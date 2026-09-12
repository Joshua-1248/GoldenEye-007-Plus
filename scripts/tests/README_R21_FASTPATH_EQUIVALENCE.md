# R21 fast-path equivalence test note

The R20 community-source archive supplied as the R21 baseline contained
`scripts/test_fastpaths.sh`, but did **not** contain its referenced source file:

`scripts/tests/fastpath_equivalence.c`

R21 therefore cannot honestly reproduce that host-side equivalence test from
the supplied baseline. The wrapper now exits with status **77 (SKIP)** and an
explicit explanation when the source is absent, instead of failing at the host
compiler or being misreported as a passing test.

This does not affect the N64 ROM build. R21's physical-source audit,
post-link physical ROM audit, fixed compressed-C slot guard, and R21 static
regression audit are separate checks.
