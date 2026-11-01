# Documentation map and authority

Start with the [fresh-instance onboarding/spec brief](onboarding/AGENT-START.md).
This catalog locates project-maintained documentation; it is not a requirement
to read every research record before a first build.

## How to use the documentation

| Kind | How to interpret it |
|---|---|
| Current task/status | AGENTS current state and the core continuation point to the accepted baseline and remaining work. Recheck live PRs before acting. |
| Accepted contract/plan | Governs its stated scope and acceptance criteria. An imperative in a completed stage is not an instruction to repeat it. Later explicitly accepted corrections supersede older intent. |
| Implementation/component reference | Read with current source and its profile/backend limits. A bounded component does not prove full operations. |
| Source-pinned evidence | Describes that source and execution environment. Preserve results, failures, checksums and distinctions between local and hosted runs. |
| Dated discussion/recovery/research | Preserves derivation and provenance. Old “current”, “next” and machine-local paths are historical. |
| Upstream/vendor docs | Describe Taichi or dependencies, not HCP operating policy or authorization to use their Python frontend. |

The [2026-11-01 review](documentation-review-2026-11-01.md) records the status
reconciliation and validation scope. Keep queue changes in AGENTS and the owning
plan, link them from entry points, and preserve measurements with their evidence.
Do not append another competing “next task” to every historical document.

## Evidence, upstream and archive collections

- [Engine test evidence](../engine/tests/) contains per-slice source pins,
  commands, logs and compiler artifacts. It remains historical evidence even
  when a later implementation supersedes the behavior.
- [PostgreSQL snapshot manifests](../data/postgres/snapshots/) describe retained
  data and provenance. Installation and new population are separate operations.
- [Taichi fork provenance](../engine/taichi/HCP-FORK.md),
  [HCP delta](../engine/taichi/HCP-DELTA.md) and
  [startup configuration](../engine/taichi/modernization/INSTALL-CONFIGURATION.md)
  explain the native dependency. The remaining upstream docs and modernization
  investigation records are references, not fresh-instance setup instructions.
- [Database review records](../kernels/database/review/), [repository reviews](../review/),
  [research](../research/) and [archives](../archive/) retain their original scope.
  They are not rewritten to manufacture current acceptance.
- Corpus text, licenses, generated captures, SQL exports and vendored legal/API
  material are data/reference artifacts, not an alternate onboarding queue.

## Project documentation inventory

Grouped by location and use. Titles such as PLAN, NOTES or HANDOFF do not alone
establish current authority; check their status/provenance and accepted successors.

### Entry points and governance

- [.github/ISSUE_TEMPLATE/agent-suitable.md](../.github/ISSUE_TEMPLATE/agent-suitable.md)
- [.github/ISSUE_TEMPLATE/good-first-issue.md](../.github/ISSUE_TEMPLATE/good-first-issue.md)
- [AGENTS.md](../AGENTS.md)
- [CONTRIBUTING.md](../CONTRIBUTING.md)
- [MANIFESTO.md](../MANIFESTO.md)
- [README.md](../README.md)
- [REORGANIZATION.md](../REORGANIZATION.md)
- [ROADMAP.md](../ROADMAP.md)
- [charter.md](../charter.md)
- [covenant.md](../covenant.md)
- [docs/onboarding/AGENT-START.md](onboarding/AGENT-START.md)
- [docs/onboarding/FIRST-RUN.md](onboarding/FIRST-RUN.md)
- [docs/onboarding/README.md](onboarding/README.md)

### Project contracts, plans and model references

- [docs/README.md](README.md)
- [docs/address-encoding-transition.md](address-encoding-transition.md)
- [docs/architecture.md](architecture.md)
- [docs/bonding-notes.md](bonding-notes.md)
- [docs/c3-cpu-capture-handoff.md](c3-cpu-capture-handoff.md)
- [docs/coherent-dynamics-gpu-snapshots.md](coherent-dynamics-gpu-snapshots.md)
- [docs/core-local-study-continuation-plan.md](core-local-study-continuation-plan.md)
- [docs/core-operation-readiness-plan.md](core-operation-readiness-plan.md)
- [docs/cumulative-publication-integration-plan.md](cumulative-publication-integration-plan.md)
- [docs/data-protocol.md](data-protocol.md)
- [docs/engine-object-implementation-plan.md](engine-object-implementation-plan.md)
- [docs/engine-object-support-corrections.md](engine-object-support-corrections.md)
- [docs/field-physics-and-tick-notes.md](field-physics-and-tick-notes.md)
- [docs/first-stage-data-object-model.md](first-stage-data-object-model.md)
- [docs/instance-local-data.md](instance-local-data.md)
- [docs/language-component-unicode-plan.md](language-component-unicode-plan.md)
- [docs/letter-population-writer.md](letter-population-writer.md)
- [docs/linguistic-bootstrap-population-plan.md](linguistic-bootstrap-population-plan.md)
- [docs/live-hierarchy-handoff-plan.md](live-hierarchy-handoff-plan.md)
- [docs/live-hierarchy-refresh-plan.md](live-hierarchy-refresh-plan.md)
- [docs/live-numerical-profile-plan.md](live-numerical-profile-plan.md)
- [docs/model-realignment-plan.md](model-realignment-plan.md)
- [docs/napier-system-discussion.md](napier-system-discussion.md)
- [docs/napier-system-guide.md](napier-system-guide.md)
- [docs/native-admission-contact-transaction-plan.md](native-admission-contact-transaction-plan.md)
- [docs/native-precision-truncation-plan.md](native-precision-truncation-plan.md)
- [docs/parent-structure-notes.md](parent-structure-notes.md)
- [docs/particle-geometry-notes.md](particle-geometry-notes.md)
- [docs/physics-basis.md](physics-basis.md)
- [docs/pooled-space-live-insertion-plan.md](pooled-space-live-insertion-plan.md)
- [docs/pre-d-hot-workbench-design.md](pre-d-hot-workbench-design.md)
- [docs/precision-exposure-and-concept-catalogue.md](precision-exposure-and-concept-catalogue.md)
- [docs/prepared-capacity-foundation-plan.md](prepared-capacity-foundation-plan.md)
- [docs/prepared-coherent-update-plan.md](prepared-coherent-update-plan.md)
- [docs/prepared-execution-backing-plan.md](prepared-execution-backing-plan.md)
- [docs/production-hierarchy-response-plan.md](production-hierarchy-response-plan.md)
- [docs/storage-and-working-split.md](storage-and-working-split.md)
- [docs/upward-centroid-integration-plan.md](upward-centroid-integration-plan.md)
- [docs/viewport-purpose-and-plan.md](viewport-purpose-and-plan.md)
- [docs/warm-knowledge-cache-layout.md](warm-knowledge-cache-layout.md)
- [docs/warm-toolbox-implementation-plan.md](warm-toolbox-implementation-plan.md)
- [docs/weighted-centroid-accounting-plan.md](weighted-centroid-accounting-plan.md)

### Engine contracts, build guidance and historical records

- [engine/ARCHITECTURE.md](../engine/ARCHITECTURE.md)
- [engine/README.md](../engine/README.md)
- [engine/docs/ACCEPTED-MOTION.md](../engine/docs/ACCEPTED-MOTION.md)
- [engine/docs/ACTIVE-FIELD-MODEL.md](../engine/docs/ACTIVE-FIELD-MODEL.md)
- [engine/docs/ACTIVE-WORKBENCH.md](../engine/docs/ACTIVE-WORKBENCH.md)
- [engine/docs/ASSEMBLY-BOOTSTRAP.md](../engine/docs/ASSEMBLY-BOOTSTRAP.md)
- [engine/docs/BUILD-VETTING.md](../engine/docs/BUILD-VETTING.md)
- [engine/docs/CENTROID-CASCADE.md](../engine/docs/CENTROID-CASCADE.md)
- [engine/docs/CONFIGURATION.md](../engine/docs/CONFIGURATION.md)
- [engine/docs/CONTACT-SAFE-TICKS.md](../engine/docs/CONTACT-SAFE-TICKS.md)
- [engine/docs/CUDA-C3-QUALIFICATION.md](../engine/docs/CUDA-C3-QUALIFICATION.md)
- [engine/docs/CUDA-LOCAL-VALIDATION.md](../engine/docs/CUDA-LOCAL-VALIDATION.md)
- [engine/docs/CUMULATIVE-PUBLICATION.md](../engine/docs/CUMULATIVE-PUBLICATION.md)
- [engine/docs/DEVICES.md](../engine/docs/DEVICES.md)
- [engine/docs/DRIFT-AUDIT.md](../engine/docs/DRIFT-AUDIT.md)
- [engine/docs/EFFECTIVE-MOTION.md](../engine/docs/EFFECTIVE-MOTION.md)
- [engine/docs/ENGINE-NOTES.md](../engine/docs/ENGINE-NOTES.md)
- [engine/docs/HARNESS-NOTES.md](../engine/docs/HARNESS-NOTES.md)
- [engine/docs/HARNESS-STRUCTURE.md](../engine/docs/HARNESS-STRUCTURE.md)
- [engine/docs/LETTER-PROJECTION.md](../engine/docs/LETTER-PROJECTION.md)
- [engine/docs/LIVE-HIERARCHY.md](../engine/docs/LIVE-HIERARCHY.md)
- [engine/docs/LIVE-NUMERICAL-PROFILE.md](../engine/docs/LIVE-NUMERICAL-PROFILE.md)
- [engine/docs/LIVE-REFRESH.md](../engine/docs/LIVE-REFRESH.md)
- [engine/docs/MEMBERSHIP-FIELDS.md](../engine/docs/MEMBERSHIP-FIELDS.md)
- [engine/docs/NATIVE-HANDOFF.md](../engine/docs/NATIVE-HANDOFF.md)
- [engine/docs/NATIVE-PRECISION.md](../engine/docs/NATIVE-PRECISION.md)
- [engine/docs/NATIVE-SELECTION.md](../engine/docs/NATIVE-SELECTION.md)
- [engine/docs/OBJECT-SUPPORT-RESULTS.md](../engine/docs/OBJECT-SUPPORT-RESULTS.md)
- [engine/docs/OPERATIONAL-PLAN.md](../engine/docs/OPERATIONAL-PLAN.md)
- [engine/docs/PRECISION-STATE-AUDIT.md](../engine/docs/PRECISION-STATE-AUDIT.md)
- [engine/docs/PREPARED-EXECUTION.md](../engine/docs/PREPARED-EXECUTION.md)
- [engine/docs/PREPARED-GEOMETRY.md](../engine/docs/PREPARED-GEOMETRY.md)
- [engine/docs/PREPARED-SPACE.md](../engine/docs/PREPARED-SPACE.md)
- [engine/docs/PREPARED-UPDATES.md](../engine/docs/PREPARED-UPDATES.md)
- [engine/docs/README.md](../engine/docs/README.md)
- [engine/docs/RECORD-MAPPING.md](../engine/docs/RECORD-MAPPING.md)
- [engine/docs/SOURCE-COUNTS.md](../engine/docs/SOURCE-COUNTS.md)
- [engine/docs/VETTING-PLAN.md](../engine/docs/VETTING-PLAN.md)
- [engine/docs/WEIGHTED-CENTROID-CASCADE.md](../engine/docs/WEIGHTED-CENTROID-CASCADE.md)
- [engine/docs/WEIGHTED-NATIVE-HANDOFF.md](../engine/docs/WEIGHTED-NATIVE-HANDOFF.md)
- [engine/docs/api-reference/NOTES.md](../engine/docs/api-reference/NOTES.md)
- [engine/docs/taichi-snode-mechanics.md](../engine/docs/taichi-snode-mechanics.md)

### Database, WAL and network components

- [kernels/README.md](../kernels/README.md)
- [kernels/database/API.md](../kernels/database/API.md)
- [kernels/database/ENCODING-ENDPOINT-TIER-PLAN.md](../kernels/database/ENCODING-ENDPOINT-TIER-PLAN.md)
- [kernels/database/ENCODING-TABLES-PLAN.md](../kernels/database/ENCODING-TABLES-PLAN.md)
- [kernels/database/HANDOFF.md](../kernels/database/HANDOFF.md)
- [kernels/database/NOTES.md](../kernels/database/NOTES.md)
- [kernels/database/PLAN.md](../kernels/database/PLAN.md)
- [kernels/database/TIER2-PLAN.md](../kernels/database/TIER2-PLAN.md)
- [kernels/database/WORKING-SET-AND-LEDGER.md](../kernels/database/WORKING-SET-AND-LEDGER.md)
- [kernels/database/codec/README.md](../kernels/database/codec/README.md)
- [kernels/database/command/README.md](../kernels/database/command/README.md)
- [kernels/database/controller/README.md](../kernels/database/controller/README.md)
- [kernels/database/dbmanager/README.md](../kernels/database/dbmanager/README.md)
- [kernels/database/declare/README.md](../kernels/database/declare/README.md)
- [kernels/database/dispatch/README.md](../kernels/database/dispatch/README.md)
- [kernels/database/ingestion/README.md](../kernels/database/ingestion/README.md)
- [kernels/database/read/README.md](../kernels/database/read/README.md)
- [kernels/database/schema/README.md](../kernels/database/schema/README.md)
- [kernels/database/schema/tests.md](../kernels/database/schema/tests.md)
- [kernels/database/update/README.md](../kernels/database/update/README.md)
- [kernels/database/warm/README.md](../kernels/database/warm/README.md)
- [kernels/database/warm/STRUCTURAL-RECIPES.md](../kernels/database/warm/STRUCTURAL-RECIPES.md)
- [kernels/wal/README.md](../kernels/wal/README.md)
- [kernels/wal/REPORT-TO-WORK.md](../kernels/wal/REPORT-TO-WORK.md)
- [kernels/wal/USAGE.md](../kernels/wal/USAGE.md)
- [kernels/wal/WAL-IMPL-PLAN.md](../kernels/wal/WAL-IMPL-PLAN.md)
- [kernels/wal/WAL-INTEGRATION-HANDOFF.md](../kernels/wal/WAL-INTEGRATION-HANDOFF.md)
- [kernels/wal/WAL-INTEGRATION-PLAN.md](../kernels/wal/WAL-INTEGRATION-PLAN.md)
- [kernels/wal/WAL-PLAN.md](../kernels/wal/WAL-PLAN.md)
- [kernels/wal/wal_schema_tests.md](../kernels/wal/wal_schema_tests.md)
- [network/ENDPOINT-ACTIVATION-NOTES.md](../network/ENDPOINT-ACTIVATION-NOTES.md)
- [network/ENDPOINT-PRIMITIVES-PLAN.md](../network/ENDPOINT-PRIMITIVES-PLAN.md)
- [network/KERNEL-ACTIVATION.md](../network/KERNEL-ACTIVATION.md)
- [network/README.md](../network/README.md)
- [network/SWARM-NOTES.md](../network/SWARM-NOTES.md)
- [network/endpoint/README.md](../network/endpoint/README.md)

### Tools and source material

- [sources/README.md](../sources/README.md)
- [tools/legacy-extraction/README.md](../tools/legacy-extraction/README.md)
- [tools/linguistics/README.md](../tools/linguistics/README.md)
- [tools/viewport/README.md](../tools/viewport/README.md)

### Historical handoffs and reconciliation

- [docs/documentation-status-review-2026-10-23.md](documentation-status-review-2026-10-23.md)
- [docs/history/acceptance-through-201.md](history/acceptance-through-201.md)
- [docs/legacy-data-maps/README.md](legacy-data-maps/README.md)
- [docs/legacy-data-maps/database-access.md](legacy-data-maps/database-access.md)
- [docs/legacy-data-maps/kaikki-pipeline.md](legacy-data-maps/kaikki-pipeline.md)
- [docs/legacy-data-maps/shards-and-schema.md](legacy-data-maps/shards-and-schema.md)
- [docs/legacy-data-maps/tokenization-policies.md](legacy-data-maps/tokenization-policies.md)
- [engine/taichi/modernization/IMPLEMENTATION-STATUS.md](../engine/taichi/modernization/IMPLEMENTATION-STATUS.md)
- [engine/taichi/modernization/hcp-handoff/START-HERE.md](../engine/taichi/modernization/hcp-handoff/START-HERE.md)
