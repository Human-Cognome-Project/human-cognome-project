# system boundaries

Read before changing component responsibilities, runtime languages, privacy or cross-component interfaces. Component-local work must preserve the applicable boundaries.

[Choose another path](README.md) · [Bounded work](AGENT-START.md)

## Architecture invariants

- The modified Taichi **runtime/compiler substrate**, HCP **physics engine**, and HCP **engine harness** are distinct concepts.
- The canonical engine path is native **C++**. Taichi's Python frontend is upstream/vendor tooling, not the HCP runtime surface.
- Project Python is authorized only for bulk database population scripts during bootstrap. Their source preparation and validation stay within that population workflow. Offline operation alone does not authorize Python. Do not implement reusable selection/assembly components, physics, database/WAL kernels, endpoint/network infrastructure, or analyst behavior in Python.
- The **engine harness is the control interface for the physics engine**. It is not the DB manager, WAL manager, messaging system, topology system or thread manager.
- **Analyst functions are conceptually designed within the overall architecture.** Their concrete implementation structure must be engineered and calibrated against engine results on real data structures. That engineering is deferred; the overall architecture is not. Preserve the conceptual design rather than treating the analyst as an architectural blank slate or claiming its functions are already implemented. See [architecture clarification](../../engine/ARCHITECTURE.md#analyst-design-and-engineering-status).
- Warm is the shared tool/part box; hot is the analyst-owned host assembly workbench; GPU memory is the execution surface. Private/local inputs enter only through the analyst-owned active input boundary. The cache manager must not access private DB codes/data, including for content inspection or filtering.
- Database/cache/record/WAL kernels exist to support the future analyst and keep its working surfaces current.
- Kernel components communicate through active inbox/outbox endpoints and should remain location-blind: local versus remote placement is a topology/bridge concern.
- The future configuration routine resolves endpoint paths; the future thread manager handles lower-frequency/system-facing bridge activation.
- `kernels/database/`, `kernels/wal/`, and `network/endpoint/` are peers. Do not collapse WAL or shared endpoint infrastructure back under the database/cache hierarchy.


## Do not reintroduce

- Do not infer conceptual scale from raw SNode depth or secondary compression nesting.
- Do not implement the superseded English `01` / per-language database layout.
- Do not substitute a shared core character for a language-letter endpoint: the linguistic endpoint has its own identity and ordered core parents.
- Do not materialize reverse DB child-reference listings as live particles. They support navigation/assembly; ordered parent occurrences and separately prepared fields determine live participation.
- Do not extend Python beyond early bulk database bootstrap population and its preparation/validation. Offline operation alone is no exception; operational selection and assembly are native.
- Do not substitute invented analyst behaviour for the conceptual design or bypass engineering/calibration against engine results. Held #123 proposals remain outside accepted implementation contracts.
- Do not restore universal fixed diameter, beads-on-string geometry, positional parent offsets or rotation.
- Do not reset/drop persistent databases as routine validation.

<a id="current-model-realignment-2026-10-09"></a>

