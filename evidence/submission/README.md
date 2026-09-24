# Graph500 SUBMISSION PACKAGE — quotient-twin BFS (Kernel 2)

Status generated: 2026-09-24. Node: single Cortex-A55 (1 effective CPU), proot Linux.
All official-format runs use the UNMODIFIED reference harness (main.c, validate.c,
csr_reference.c) with a Kernel-2 drop-in (src/bfs_quotient_twin.c).

## Category labels (used throughout)
- OFFICIAL GRAPH500 RESULT — produced by the official harness ANTI-checks; the ONLY
  numbers eligible to be reported under the Graph500 TEPS definition. In this package:
  `bfs harmonic_mean_TEPS` etc. printed by `graph500_quotient_twin_bfs <SCALE> 16`.
  NOTE: not yet accepted/published on the Graph500 list; leaderboard claims are RED.
- RESEARCH / DIAGNOSTIC RESULT — ratios (S_kernel, S_composed, S_cumulative),
  QBG evidence dumps, class statistics, parallel partition experiment.
- HISTORICAL RESULT — the frozen pre-audit research table (evidence/research_baseline.md);
  retained, never presented as the new result.

## Files
- system.txt, compiler.txt            environment + toolchain
- graph500_commit.txt                 upstream checkout (f89d643, tag 3.0.1)
- algorithm_commit.txt                implementation commit (a07c5f4, +variants)
- build_commands.sh / run_commands.sh exact commands used
- baseline_output.txt                 concatenation of official_baseline/official_bfs_scale{18,19,20}.log
- quotient_output.txt                 concatenation of quotient logs (official-format fields)
- validation_output.txt               validation PASS statements extracted from runs
- timing_model.md                     operation-by-operation accounting (see parent dir)
- quotient_statistics.csv, performance_comparison.csv
- checksums.sha256                    all artifacts

## Headline official-format numbers (MEASURED, GREEN)
SCALE 18: harmonic TEPS 1.58535e+07 (reference 8.16728e+06)  NBFS 64  validation 64/64
SCALE 19: harmonic TEPS 1.42328e+07 (reference 5.60031e+06)  NBFS 64  validation 64/64
SCALE 20: harmonic TEPS 1.11964e+07 (reference 6.57104e+06)  NBFS 64  validation 64/64
TEPS ratio == mean-time speedup exactly (identical traversed-edge count m; see
timing_model.md section E).

## Compliance assertions
- Graph: official Kronecker generator, seeds 2,3, edgefactor 16, verbatim harness.
- 64 searches, one at a time, correct parent arrays produced inside each timed window.
- Kernel-1 construction builds the quotient index once (included in construction_time),
  never modified between kernels; per-search marking space only mutable area.
- Validation: unmodified official validate.c, run outside the timed windows.
- No results multiplied; every ratio above is a single measured quotient.