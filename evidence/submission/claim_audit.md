# Submission claim audit — GREEN / YELLOW / RED

Per protocol, only GREEN claims are stated as measured benchmark results; YELLOW
claims state their derivation; RED claims are withdrawn.

## GREEN (measured, reproducible, in this environment)
- Official-format harmonic BFS TEPS on the quotient run:
  SCALE 18: 1.58535e+07 · SCALE 19: 1.42328e+07 · SCALE 20: 1.11964e+07
  SCALE 20 matched pair: quotient 1.15903e+07 vs ref 9.26255e+06 (ratio 1.2513x)
  SCALE 21: quotient 4.17962e+06 (VERIFIED 64/64)
  (reference at 18/19/20: 8.16728e+06 / 5.60031e+06 / 6.57104e+06; 21: 5.83657e+06).
- 64/64 official validation PASS at SCALE 12/16/18/19/20/21 for BOTH binaries,
  scale 21 = extended MAX VERIFIED SCALE (full validation, exit 0).
- min_nedge equality: 65327 / 4193472 / 8387584 / 16775818 / 33552502
  (reference == quotient).
- Quotient structure: V/Q, E/Q, compression, largest class = isolated class
  (SCALE 12/16/18/19/20/21).
- Q B_G == B_Q Q measured equality (0 mismatches): SCALE 12 (4095/4096 non-root),
  SCALE 16 (65536/65536), SCALE 18 (262144/262144), SCALE 19 (524288/524288),
  SCALE 20 (1048575/1048575, matched pair).
- Reconstruction: per-member parent-edge verification performed for every assigned
  parent (benchmarked; recon_edge_verified == recon_members on every run).

## YELLOW (derived, each derivation explicitly stated)
- "TEPS ratio == time speedup": derives from identical traversed-edge count m
  (measured min_nedge equality) and the harmonic relationship TEPS = m / mean_time
  (nedge quartiles identical, so harmonic mean TEPS ratio = mean-time ratio). This is
  an arithmetic identity on measured quantities, not a compound claim; no speedups are
  multiplied (each S_* is a single measured ratio).
- "Largest twin class is the isolated class": shown by direct measurement at every
  scale (largest_class == isolated count) and derives from the empty-neighborhood
  being exactly-one equivalence class.
- "Quotient carries no extra captured work": derives from timing_model.md (quotient
  built once in Kernel 1; searches read-only).
- SCALE-21 cross-window TEPS ratio 0.716x (quotient slower): DIMENSION-flagged
  YELLOW — the two 21 runs were NOT back-to-back matched; and the structural reason
  (isolated-class-dominated compression ~1.06x edge float at 21) makes the reversal
  plausible but not asserted as a benchmark result. The scale-21 quotient's own
  correctness (64/64) is GREEN; its relative performance is YELLOW.

## RED (withdrawn / not claimed)
- No leaderboard rank, no "official TEPS acceptance", no "world record", no
  "official Graph500 result" label: the result is not published on any list.
- No claim that the two-worker partition is official: the spec forbids concurrent
  search keys; the parallel run is a RESEARCH diagnostic only.
- No claim of physical parallelism on this host before measurement (efficiency is
  measured in the diagnostic, not assumed: S_parallel 0.74, eff 0.37 measured).
- No claim that the quotient improves BFS at SCALE 21 (measured cross-window ratio
  0.716x, quotient slower; not claimed as a win).