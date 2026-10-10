# Engine corrections for first-stage data objects

Status: **ACCEPTED in PR #124; runtime implementation not started.**
Execution details: [implementation plan](engine-object-implementation-plan.md).

This proposal translates the agreed physics simplifications into corrections
needed by the objects that will be completed in the first data-modeling stage.
It follows the current [model realignment plan](model-realignment-plan.md),
with the delivery focus clarified by Patrick in this session: correct the
engine, verify the changes, then proceed directly to completing first-stage
data modeling. Verification is an implementation acceptance check, not a new
research phase to re-prove the agreed model mechanics.

## Approval and delivery

This proposal PR is the approval gate before runtime changes. After approval,
implement the corrections in focused commits and publish the implementation
PR with its verification results. Direct adversarial review by separate agents
is optional; the named coder/adversary workflow in the database plans belongs
to Patrick's local CI units.

DECLARE and PR #111 record-integrity corrections remain a separate workstream.
This proposal authorizes no persistent database writes or schema changes.
When the engine acceptance checks succeed, the next task is completion of the
first data-modeling stage and its intended object insertion, rather than
additional general physics qualification.

## Corrections

### 1. Proportional whole-particle response — #118

Remove internal application offsets from both the force and centroid paths.
Evaluate each participating contribution at the particle's actual centre,
while applying its resulting force to the whole particle through the existing
mass-based integration. Preserve proportional participation, exact structural
counts at the object boundary, ordered composition, and repeated occurrences.

Update the edge payload, staging callers, tests and documentation together.
Remove the obsolete off-centre-response assertions and descriptions; replace
them with checks of the corrected whole-particle behavior. Preserve actual
positions of separately exposed components. Add no rotation or rotary alignment.

### 2. Object extent and ordinary contact — #120, #119

Carry a prepared radius per represented particle rather than assuming every
object has one universal diameter. Radius, structural mass and operative
distance scale remain distinct. Compute enclosure during object preparation
and reuse it until composition, geometry or scale changes.

The proposed bounded enclosure rule for review is the maximum of
`distance(component centre, object centroid) + component radius` over the
prepared components, all expressed in the same frame. This encloses the
declared component geometry without a summed-volume approximation. It does
not choose a packing layout or place components: the layout must be specified
with the first-stage object definitions. Internal enclosure coordinates are
preparation data, not force-application offsets. Reconcile the single/double
and triple/quadruple examples in #120 before fixing their layout or exact radii.

Use the sum of the two prepared radii as the ordinary spherical-contact
distance on the supported contact relationships. Separate this body-contact
response from corona spacing and the defined-bond stretch hold. The corrected
first-stage path runs without corona/alignment spacing regimes or the special
shear hold; cohesion comes from the participating attraction fields. Preserve
the existing finite penetration response initially, expressed against the
prepared body-contact distance, and qualify it with the corrected fixtures.
Do not silently replace the ledgered contact coverage with an all-pairs pass.

Keep the existing main commit and its settings as a reproducible reference;
successful corrected-path checks need not retain the old mechanisms as a
second supported runtime mode.

### 3. Data-object staging and distance assumptions — #106, #115

Use explicit object and occurrence records to stage the first fixed study.
Retain reusable cold identity separately from live particle/slot identity;
repeated uses of a definition have distinct mutable state. Parent-value fields
must preserve connections across ordinal positions, including repeated parents.
Do not double-count an aggregate and its exposed parts.

The designated primary study structure carries conceptual scale. Attached
secondary compression inherits that scale at 1:1 through nested dedupe.
Use 64 as the initial conceptual-distance choice where a primary transition
is expressed, and prepare positions, radii and centroids in one operative frame.
Do not derive scale from raw SNode depth, cold address depth or compression depth,
rescale structural mass, or add a compensating force multiplier.

This first fixed-study correction does not require completing the general
zoom/refocus/recomposition machinery under review in #123. Preserve the future
seam without adopting its unresolved publication contract. Seed consistent
centroids before the first force pass and after any supported relationship
publication. Treat unused mass-zero slots as inert without divisions by zero
if the first-stage staging uses a preallocated pool.

### 4. Integration and obsolete runtime elements

Audit numerical guards and integration inputs affected by the payload and
extent changes. Retain the current integration/braking behavior unless a
correction exposes a specific failure; any necessary adjustment must identify
the failing case and resulting behavior. Do not add new force laws, arbitrary
stabilizing constraints or activation machinery to obtain a passing result.

The suspended universal-attraction kernel is not active physics. Its total-mass
calculation still feeds radial damping, so any cleanup must preserve that
dependency. Broad damping redesign and selective work controls in #91 are not
automatic prerequisites for supporting these objects.

## Acceptance and handoff

Run the existing native CPU smoke, field and index-cap suites, updating the
field assertions that describe withdrawn behavior. Add focused regressions for
the corrected contribution, enclosure, contact and staging paths, including
repeated contributions and any supported free slots. Exercise representative
first-stage object relationships through the native engine, checking finite
state, valid centroid initialization and the intended contact/cohesion behavior.

Use whatever bounded fixtures best exercise the changes; they are verification
inputs, not an alternative data plan. Record the settings and observed outcomes.
Run available backend checks appropriate to the actual environment and record
unavailable hardware without extending this into another qualification project.

Once these implementation checks pass, publish the results and proceed to
completion of the first-stage data modeling. A failure should lead to diagnosis
and a bounded correction, not silent disabling of checks or re-opening agreed
model principles. Persistent data insertion follows its own reviewed scope.
