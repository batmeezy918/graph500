# Submission claim audit — GREEN / YELLOW / RED

Per protocol, only GREEN claims are stated as measured benchmark results; YELLOW
claims state their derivation; RED claims are withdrawn.

## GREEN (measured, reproducible, in this environment)
- Official-format harmonic BFS TEPS on the quotient run:
  SCALE 18: 1.58535e+07 · SCALE 19: 1.42328e+07 · SCALE 20: 1.11964e+07
  (with reference at 8.16728e+06 / 5.60031e+06 / 6.57104e+06; logs + checksums).
- 64/64 official validation PASS at SCALE 12/18/19/20 for BOTH binaries.
- min_nedge equality: 65327 / 4193472 / 8387584 / 16775818 (reference == quotient).
- Quotient structure: V/Q, E/Q, compression, largest class = isolated class
  (SCALE 12/18/19/20).
- Q B_G == B_Q Q measured equality (0 mismatches): SCALE 12 (4095/4096 non-root),
  SCALE 18 (262144/262144), SCALE 19 (524288/524288).
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
- Any SCALE-21 feasibility claim (pending) will be "probe-only", derived from a
  SKIP_VALIDATION bounded run, NOT a full submission measurement.

## RED (withdrawn / not claimed)
- No leaderboard rank, no "official TEPS acceptance", no "world record", no
  "official Graph500 result" label: the result is not published on any list.
- No claim that the two-worker partition is official: the spec forbids concurrent
  search keys; the parallel run is a RESEARCH diagnostic only.
- No claim of physical parallelism on this host before measurement (efficiency is
  measured in the diagnostic, not assumed).