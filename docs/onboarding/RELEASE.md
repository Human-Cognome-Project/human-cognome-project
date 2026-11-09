# Local study and release intent

Read for study design, observation or packaging work. [Choose another path](README.md).

## Intended local study and release

The purpose is to observe mathematically derived organization and its changes
when new language material is added. Small letter universes need not crystallize
or reach exact stillness. Report motion and shapes separately from invariant
failures; do not inject language-family clustering as an expected answer.
Earlier max-tick observations do not establish a runtime timeout or large-scale
performance. More bodies/fields may amortize overhead; measure before claiming it.

Local development uses the native build and NAS PostgreSQL; secrets stay outside
Git. The release snapshot means a **ready-to-load-and-run Docker setup**:
application image(s), separate retained-fixture PostgreSQL service, commands,
notation and source/image/fixture identities. Prior model states are optional.
Choose CPU/GPU image separation during packaging. Use
`docker/snapshots/<date>-<source-revision>/` for release bundles with appropriate
large-artifact handling. A command-line operator or coding agent can drive it;
a new GUI is not required. The bounded letter-physics mini-release must state
supported operations and backends and does not imply completion of #156/#145.

## Security updates before publication

Patrick confirmed on 2026-11-09 that published Docker images must include
available applicable security fixes, including Ubuntu 24.04 updates where that
distribution is used. Refresh builder and runtime bases and installed packages
before each release; updating the development machine alone does not update an
image. Apply the same requirement to the separate PostgreSQL service image.

Select a current patched base, then record its digest and the resulting package
inventory with the release's source/image identities. Deliberately refresh pinned
digests when fixes arrive; rebuilding an old digest or reusing stale package layers
does not establish current patch coverage. Follow [Docker's build guidance](https://docs.docker.com/build/building/best-practices/)
for refreshing bases and package layers. Check applicable vendor advisories and
scan final images; record the date, findings and fixes. Unresolved findings need
an explicit reviewed disposition, not a claim that an image is vulnerability-free.

Validate supported operations/backends on the patched release images before
publication and retain the results. Preserve PostgreSQL data and fixture identities
during service-image updates. Host/WSL kernel maintenance remains a separate
deployment responsibility; image updates do not establish host patch status.

