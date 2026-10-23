-- psql migration, PostgreSQL 16. Run only against the explicitly chosen warm DB.
-- Existing recipe bodies/starts, cold stores and heads are not rewritten.
\set ON_ERROR_STOP on
BEGIN;
SET LOCAL lock_timeout = '5s';
-- Serialize migration/retry and publication of bundle headers. A busy reader or
-- writer may cause a timeout: rollback and retry, never expose partial DDL.
LOCK TABLE warm_toolbox.bundle_revision IN ACCESS EXCLUSIVE MODE;
SELECT to_regclass('warm_toolbox.recipe_schema') IS NULL AS upgrade_needed \gset
\if :upgrade_needed
CREATE TABLE warm_toolbox.recipe_schema (
 singleton boolean PRIMARY KEY CHECK(singleton),
 version integer NOT NULL CHECK(version=2)
);
ALTER TABLE warm_toolbox.bundle_revision
 DROP CONSTRAINT bundle_revision_recipe_version_check,
 ADD CONSTRAINT bundle_revision_recipe_version_check CHECK(recipe_version IN (1,2)),
 ADD CONSTRAINT bundle_revision_version_key UNIQUE(selection,revision,recipe_version);

CREATE TABLE warm_toolbox.bundle_occurrence_v2 (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 recipe_version integer NOT NULL DEFAULT 2 CHECK(recipe_version=2),
 entry numeric(20,0) NOT NULL CHECK(entry BETWEEN 0 AND 18446744073709551615),
 path bigint[] NOT NULL CHECK(cardinality(path) IN (0,2)),
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, node integer NOT NULL,
 PRIMARY KEY(selection,bundle_revision,entry,path),
 FOREIGN KEY(selection,bundle_revision,recipe_version)
   REFERENCES warm_toolbox.bundle_revision(selection,revision,recipe_version),
 FOREIGN KEY(selection,bundle_revision,scope,address)
   REFERENCES warm_toolbox.bundle_definition(selection,bundle_revision,scope,address),
 FOREIGN KEY(selection,bundle_revision,node)
   REFERENCES warm_toolbox.bundle_node(selection,bundle_revision,node)
);
CREATE TABLE warm_toolbox.bundle_selection_v2 (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, entry numeric(20,0) NOT NULL,
 aggregate_path bigint[] NOT NULL DEFAULT '{}' CHECK(cardinality(aggregate_path)=0),
 exposure integer NOT NULL CHECK(exposure IN (0,1)),
 PRIMARY KEY(selection,bundle_revision,entry),
 FOREIGN KEY(selection,bundle_revision,entry,aggregate_path)
   REFERENCES warm_toolbox.bundle_occurrence_v2(selection,bundle_revision,entry,path)
);
CREATE TABLE warm_toolbox.bundle_part_v2 (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, entry numeric(20,0) NOT NULL,
 ordinal integer NOT NULL CHECK(ordinal>=0), path bigint[] NOT NULL,
 PRIMARY KEY(selection,bundle_revision,entry,ordinal),
 FOREIGN KEY(selection,bundle_revision,entry)
   REFERENCES warm_toolbox.bundle_selection_v2(selection,bundle_revision,entry),
 FOREIGN KEY(selection,bundle_revision,entry,path)
   REFERENCES warm_toolbox.bundle_occurrence_v2(selection,bundle_revision,entry,path)
);
CREATE TABLE warm_toolbox.bundle_contact_v2 (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, ordinal integer NOT NULL CHECK(ordinal>=0),
 a_entry numeric(20,0) NOT NULL, a_path bigint[] NOT NULL,
 b_entry numeric(20,0) NOT NULL, b_path bigint[] NOT NULL,
 PRIMARY KEY(selection,bundle_revision,ordinal),
 FOREIGN KEY(selection,bundle_revision,a_entry,a_path)
   REFERENCES warm_toolbox.bundle_occurrence_v2(selection,bundle_revision,entry,path),
 FOREIGN KEY(selection,bundle_revision,b_entry,b_path)
   REFERENCES warm_toolbox.bundle_occurrence_v2(selection,bundle_revision,entry,path)
);
CREATE TABLE warm_toolbox.bundle_focus_v2 (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 recipe_version integer NOT NULL DEFAULT 2 CHECK(recipe_version=2),
 ordinal integer NOT NULL CHECK(ordinal>=0), kind integer NOT NULL CHECK(kind BETWEEN 0 AND 2),
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL,
 PRIMARY KEY(selection,bundle_revision,ordinal),
 UNIQUE(selection,bundle_revision,kind,scope,address),
 FOREIGN KEY(selection,bundle_revision,recipe_version)
   REFERENCES warm_toolbox.bundle_revision(selection,revision,recipe_version),
 FOREIGN KEY(selection,bundle_revision,scope,address)
   REFERENCES warm_toolbox.bundle_definition(selection,bundle_revision,scope,address)
);
REVOKE ALL ON warm_toolbox.recipe_schema, warm_toolbox.bundle_occurrence_v2,
 warm_toolbox.bundle_selection_v2, warm_toolbox.bundle_part_v2,
 warm_toolbox.bundle_contact_v2, warm_toolbox.bundle_focus_v2 FROM PUBLIC;
INSERT INTO warm_toolbox.recipe_schema VALUES(true,2);
\else
SELECT count(*)=1 AND min(version)=2 AS supported_schema FROM warm_toolbox.recipe_schema \gset
\if :supported_schema
\else
\echo 'Unsupported warm recipe schema; migration refused.'
\quit 3
\endif
\endif
COMMIT;
