# Timing model — operation-by-operation accounting

Reference harness: src/main.c (fork head f89d643, unchanged).
Per-search timed span (main.c:374-377):
    bfs_start = MPI_Wtime();
    run_bfs(root, &pred[0]);
    bfs_stop  = MPI_Wtime();
    bfs_times[root_idx] = bfs_stop - bfs_start;
Spec §9.1: "immediately prior to visiting the search root ... until the output has
been written to memory." The BFS output is the parent array pred[] (spec §6.2), so
everything that WRITES pred[] must lie inside this span. Declared compliance for
the quotient kernel below.

## Classification of every quotient operation

A. PREPROCESSING (untimed for the searches; performed once in Kernel 1):
   1. graph_generation (edge tuples)             — untimed by rule (spec §3 "data
       generator and retrieval operations need not be timed"); recorded separately.
   2. Kernel 1: convert_graph_to_oned_csr        — inside construction_time
   3. Kernel 1: row sorting (required to make neighborhoods comparable/lookupable)
                                                 — inside construction_time
   4. Kernel 1: fingerprint prefilter build      — inside construction_time
   5. Kernel 1: EXACT equivalence classification — inside construction_time
   6. Kernel 1: quotient CSR construction        — inside construction_time
   7. Kernel 1: per-search scratch allocation    — inside construction_time
   => Items 2-7 form the officially reported construction_time. The quotient is a
      derived index READ by all 64 searches and never modified again (spec §4.1).
      A "quotient:" stderr line reports the construction split (CSR/sort/fp/
      classify/quotientCSR) as permitted additional diagnostic output.

B. UNTIMED host work between searches:
   1. BFS root sampling (deterministic seeds 2,3)   — untimed by rule (§5)
   2. clean_pred + ONE warm-up run_bfs(root[0])      — harness (main.c:350-351)
      The warm-up payload for the official run is empty: with QUOTIENT_DUMP unset
      the warm-up performs a normal BFS only. (QUOTIENT_DUMP=1 additionally runs
      the Q B_G == B_Q Q evidence BFS inside the warm-up; used only for
      semantic-evidence artifacts, never in submission-mode runs.)
   3. warm-up validate_result on root[0]             — harness (main.c:362-365)

C. TIMED PER SEARCH (the only thing compared; exactly run_bfs):
   1. reset of per-search class arrays (vis/level/parent) [O(|V_Q|)]
   2. quotient BFS: level-synchronized scan over class CSR (read-only)
   3. reconstruction pass writing pred[] (incl. isolated/unvisited vertices), with
      EXPLICIT per-member parent-edge verification via binary search over the
      sorted original CSR row (complete-bipartite theorem doubles as the check)
   => reconstruction is UNCONDITIONALLY inside the timed span because pred[] is
      the Kernel-2 output ("stop when output has been written to memory").
      No part of reconstruction is deferred to validation or reporting.

D. UNTIMED after each timed search:
   1. get_edge_count_for_teps (TEPS numerator m)
   2. official validate_result (touched only for its timing stats)
   => validation is outside the search timing span (spec §8 "run (but do not time)").

E. REPORTING (untimed): main.c prints SCALE, edgefactor, NBFS, construction_time,
   bfs time/nedge/TEPS quartiles+harmonic mean, validate stats. Our binary keeps
   the identical official block and adds only diagnostic stderr lines.

## Parity with the reference kernel inside the timed span
The official reference run_bfs performs, per search: init of dist/parent state,
top-down/bottom-up scans, and writes pred[]. Our run_bfs performs per search:
init of class state, quotient scans, reconstruction writing pred[]. Both write the
complete parent array within their span; neither moves validation inside.

## TEPS relation to measured time
m (edge_count) is counted by the same get_edge_count_for_teps semantics and is
observationally IDENTICAL (match at the quartile level with the reference runs,
e.g., min_nedge equal at SCALE 12/18/19/20). Hence with the same m in both runs,
  TEPS_quotient / TEPS_reference == time_reference / time_quotient
exactly (derived identity: TEPS = m/t). No multiply of ratios is used anywhere.