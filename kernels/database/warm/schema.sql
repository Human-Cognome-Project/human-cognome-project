-- Stage A only: apply once to a NEW, separate PostgreSQL 16 database.
-- No cold DDL, triggers, stored procedures, reverse-search indexes or cleanup.
CREATE SCHEMA warm_toolbox;
REVOKE ALL ON SCHEMA warm_toolbox FROM PUBLIC;
SET search_path = warm_toolbox, pg_catalog;

CREATE TABLE root_binding (
 scope uuid NOT NULL, prefix text[] COLLATE "C" NOT NULL,
 kind integer NOT NULL CHECK (kind BETWEEN 0 AND 2),
 epoch uuid NOT NULL, policy uuid NOT NULL, source_handle text NOT NULL,
 snapshot_hash text, writers uuid[] NOT NULL,
 PRIMARY KEY(scope,prefix)
);
CREATE TABLE tool_revision (
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, revision uuid NOT NULL,
 binding_prefix text[] COLLATE "C" NOT NULL, epoch uuid NOT NULL, recipe text NOT NULL,
 notation text, mass bigint CHECK (mass >= 0), author uuid NOT NULL,
 snapshot_hash text,
 PRIMARY KEY(scope,address,revision),
 FOREIGN KEY(scope,binding_prefix) REFERENCES root_binding(scope,prefix)
);
-- Direct object lock and permanent temporary identity claim. This serializes
-- create-only temporary addresses even across distinct preparation recipe keys.
CREATE TABLE object_slot (
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL,
 creator uuid NOT NULL, temporary_revision uuid,
 PRIMARY KEY(scope,address),
 FOREIGN KEY(scope,address,temporary_revision) REFERENCES tool_revision(scope,address,revision)
);
CREATE TABLE tool_head (
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, epoch uuid NOT NULL,
 recipe text NOT NULL, revision uuid,
 PRIMARY KEY(scope,address,epoch,recipe),
 FOREIGN KEY(scope,address,revision) REFERENCES tool_revision(scope,address,revision)
);
CREATE TABLE tool_parent (
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, revision uuid NOT NULL,
 ordinal bigint NOT NULL CHECK (ordinal BETWEEN 0 AND 4294967295),
 parent_scope uuid NOT NULL, parent_address text[] COLLATE "C" NOT NULL,
 parent_revision uuid NOT NULL, stored_mass bigint CHECK (stored_mass >= 0),
 contribution bigint NOT NULL CHECK (contribution > 0),
 PRIMARY KEY(scope,address,revision,ordinal),
 FOREIGN KEY(scope,address,revision) REFERENCES tool_revision(scope,address,revision),
 FOREIGN KEY(parent_scope,parent_address,parent_revision) REFERENCES tool_revision(scope,address,revision)
);
CREATE TABLE tool_member_of (
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, revision uuid NOT NULL,
 group_scope uuid NOT NULL, group_address text[] COLLATE "C" NOT NULL, group_revision uuid NOT NULL,
 PRIMARY KEY(scope,address,revision,group_scope,group_address,group_revision),
 FOREIGN KEY(scope,address,revision) REFERENCES tool_revision(scope,address,revision),
 FOREIGN KEY(group_scope,group_address,group_revision) REFERENCES tool_revision(scope,address,revision)
);
CREATE TABLE bundle_revision (
 selection uuid NOT NULL, revision uuid NOT NULL, owner uuid NOT NULL,
 creator uuid NOT NULL, recipe_version integer NOT NULL CHECK(recipe_version=1),
 PRIMARY KEY(selection,revision)
);
CREATE TABLE bundle_head (
 selection uuid PRIMARY KEY, revision uuid,
 FOREIGN KEY(selection,revision) REFERENCES bundle_revision(selection,revision)
);
CREATE TABLE bundle_definition (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, revision uuid NOT NULL,
 x double precision, y double precision, z double precision,
 PRIMARY KEY(selection,bundle_revision,scope,address),
 FOREIGN KEY(selection,bundle_revision) REFERENCES bundle_revision(selection,revision),
 FOREIGN KEY(scope,address,revision) REFERENCES tool_revision(scope,address,revision),
 CHECK ((x IS NULL AND y IS NULL AND z IS NULL) OR (x IS NOT NULL AND y IS NOT NULL AND z IS NOT NULL))
);
CREATE TABLE bundle_sphere (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL,
 ordinal integer NOT NULL CHECK(ordinal>=0), x double precision NOT NULL,
 y double precision NOT NULL, z double precision NOT NULL,
 radius double precision NOT NULL CHECK(radius>=0),
 PRIMARY KEY(selection,bundle_revision,scope,address,ordinal),
 FOREIGN KEY(selection,bundle_revision,scope,address) REFERENCES bundle_definition(selection,bundle_revision,scope,address)
);
CREATE TABLE bundle_node (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 node integer NOT NULL CHECK(node>=0), parent integer, role integer NOT NULL CHECK(role BETWEEN 0 AND 3),
 PRIMARY KEY(selection,bundle_revision,node),
 FOREIGN KEY(selection,bundle_revision) REFERENCES bundle_revision(selection,revision),
 FOREIGN KEY(selection,bundle_revision,parent) REFERENCES bundle_node(selection,bundle_revision,node),
 CHECK(parent IS NULL OR (parent>=0 AND parent<node))
);
CREATE TABLE bundle_occurrence (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 entry numeric(20,0) NOT NULL CHECK(entry BETWEEN 0 AND 18446744073709551615),
 path bigint[] NOT NULL CHECK(cardinality(path) IN (0,2)),
 scope uuid NOT NULL, address text[] COLLATE "C" NOT NULL, node integer NOT NULL,
 px double precision NOT NULL, py double precision NOT NULL, pz double precision NOT NULL,
 vx double precision NOT NULL, vy double precision NOT NULL, vz double precision NOT NULL,
 PRIMARY KEY(selection,bundle_revision,entry,path),
 FOREIGN KEY(selection,bundle_revision,scope,address) REFERENCES bundle_definition(selection,bundle_revision,scope,address),
 FOREIGN KEY(selection,bundle_revision,node) REFERENCES bundle_node(selection,bundle_revision,node)
);
CREATE TABLE bundle_selection (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, entry numeric(20,0) NOT NULL,
 aggregate_path bigint[] NOT NULL DEFAULT '{}' CHECK(cardinality(aggregate_path)=0),
 exposure integer NOT NULL CHECK(exposure IN (0,1)),
 PRIMARY KEY(selection,bundle_revision,entry),
 FOREIGN KEY(selection,bundle_revision,entry,aggregate_path) REFERENCES bundle_occurrence(selection,bundle_revision,entry,path)
);
CREATE TABLE bundle_part (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, entry numeric(20,0) NOT NULL,
 ordinal integer NOT NULL CHECK(ordinal>=0), path bigint[] NOT NULL,
 PRIMARY KEY(selection,bundle_revision,entry,ordinal),
 FOREIGN KEY(selection,bundle_revision,entry) REFERENCES bundle_selection(selection,bundle_revision,entry),
 FOREIGN KEY(selection,bundle_revision,entry,path) REFERENCES bundle_occurrence(selection,bundle_revision,entry,path)
);
CREATE TABLE bundle_contact (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, ordinal integer NOT NULL CHECK(ordinal>=0),
 a_entry numeric(20,0) NOT NULL, a_path bigint[] NOT NULL,
 b_entry numeric(20,0) NOT NULL, b_path bigint[] NOT NULL,
 PRIMARY KEY(selection,bundle_revision,ordinal),
 FOREIGN KEY(selection,bundle_revision,a_entry,a_path) REFERENCES bundle_occurrence(selection,bundle_revision,entry,path),
 FOREIGN KEY(selection,bundle_revision,b_entry,b_path) REFERENCES bundle_occurrence(selection,bundle_revision,entry,path)
);

CREATE TABLE bundle_child (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 parent_scope uuid NOT NULL, parent_address text[] COLLATE "C" NOT NULL,
 child_scope uuid NOT NULL, child_address text[] COLLATE "C" NOT NULL,
 PRIMARY KEY(selection,bundle_revision,parent_scope,parent_address,child_scope,child_address),
 FOREIGN KEY(selection,bundle_revision,parent_scope,parent_address) REFERENCES bundle_definition(selection,bundle_revision,scope,address),
 FOREIGN KEY(selection,bundle_revision,child_scope,child_address) REFERENCES bundle_definition(selection,bundle_revision,scope,address)
);

CREATE TABLE bundle_member_of (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 member_scope uuid NOT NULL, member_address text[] COLLATE "C" NOT NULL,
 group_scope uuid NOT NULL, group_address text[] COLLATE "C" NOT NULL,
 PRIMARY KEY(selection,bundle_revision,member_scope,member_address,group_scope,group_address),
 FOREIGN KEY(selection,bundle_revision,member_scope,member_address) REFERENCES bundle_definition(selection,bundle_revision,scope,address),
 FOREIGN KEY(selection,bundle_revision,group_scope,group_address) REFERENCES bundle_definition(selection,bundle_revision,scope,address)
);

CREATE TABLE bundle_members (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 group_scope uuid NOT NULL, group_address text[] COLLATE "C" NOT NULL,
 member_scope uuid NOT NULL, member_address text[] COLLATE "C" NOT NULL,
 PRIMARY KEY(selection,bundle_revision,group_scope,group_address,member_scope,member_address),
 FOREIGN KEY(selection,bundle_revision,group_scope,group_address) REFERENCES bundle_definition(selection,bundle_revision,scope,address),
 FOREIGN KEY(selection,bundle_revision,member_scope,member_address) REFERENCES bundle_definition(selection,bundle_revision,scope,address)
);

CREATE TABLE epoch_use (
 scope uuid NOT NULL, prefix text[] COLLATE "C" NOT NULL, epoch uuid NOT NULL,
 selection uuid NOT NULL, bundle_revision uuid NOT NULL,
 PRIMARY KEY(scope,prefix,epoch,selection,bundle_revision),
 FOREIGN KEY(scope,prefix) REFERENCES root_binding(scope,prefix),
 FOREIGN KEY(selection,bundle_revision) REFERENCES bundle_revision(selection,revision)
);
CREATE TABLE bundle_lease (
 selection uuid NOT NULL, bundle_revision uuid NOT NULL, consumer uuid NOT NULL,
 actor uuid NOT NULL, instance uuid NOT NULL,
 PRIMARY KEY(selection,bundle_revision,consumer),
 FOREIGN KEY(selection,bundle_revision) REFERENCES bundle_revision(selection,revision)
);
REVOKE ALL ON ALL TABLES IN SCHEMA warm_toolbox FROM PUBLIC;
