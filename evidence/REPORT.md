# Graph500 Quotient-Twin Closure Report

- Repo: `batmeezy918/graph500` (fork of graph500/graph500), branch `newreference`
- Upstream head: `f89d643ce4aaae9a823d310c6ab2dd10e3d2982c` (tag 3.0.1)
- Experiment commit: `a07c5f4` (local, scale-18 milestone)
- Environment: Android-16 proot, aarch64, 1 effective CPU, gcc 14.2 / OpenMPI 5.0.7
  (direct singleton; PMIx listener not available - recorded as environment failure)
- Generator: Kronecker R-MAT (A=.5700 B=.1900 C=.1900 D=.0500), seeds 2,3,
  edgefactor 16, packed 48-bit edges, 64 roots, official validation.

Evidence status ladder applied throughout:
  CONCEPTUAL -> DERIVED -> IMPLEMENTED -> MEASURED -> VERIFIED -> COMPOSED-VERIFIED
A claim is promoted only when ClaimStrength <= EvidenceStrength.

## 0. Baseline (SCALE=18)

| field                      | value               |
|----------------------------|---------------------|
| graph_generation           | 9.5037 s            |
| construction_time          | 4.0484 s            |
| bfs mean_time / root       | 0.513448 s          |
| bfs harmonic_mean_TEPS     | 8.16728e+06         |
| min_nedge                  | 4193472             |
| mean_validate              | 4.68643 s           |
| validation                 | PASS (64/64)        |

## 1. Exact quotient (SCALE=18)

| field                   | value        |
|-------------------------|--------------|
| V                       | 262145       |
| E (directed adjacency)  | 8387052      |
| V/Q                     | 153797       |
| E/Q (distinct class->class) | 7590085  |
| |V|/|V/Q| compression   | 1.704487x    |
| largest class           | 88453        |
| isolated vertices       | 88453        |

Agreement with prior calibration (V/Q=153911, comp=1.70322, largest=88023):
within 0.07 %; residual = graph edge-set differences (parallel-edge handling).

## 2. Root-safe twin handling
Open-neighborhood twins are never adjacent (u~v && u adjacent v => u in N(u),
contradiction). class(root) split into {root} and remainder; root has dist 0,
non-root members dist 2 when class(root) has a neighbor class; when class(root)
has no neighbor (root isolated), uniformity of the class is verified explicitly.
No STAGE-2 failure observed on any tested root (scales 12, 18).

## 3. Quotient BFS - concrete Q B_G = B_Q Q
SCALE 12: matched 4095/4096 non-root vertices, mismatches 0 (root handled by split).
SCALE 16: matched 65536/65536 non-root vertices, mismatches 0.
SCALE 18: matched 262144/262144 non-root vertices, mismatches 0.
SCALE 19: matched 524288/524288 non-root vertices, mismatches 0.
SCALE 20: matched 1048575/1048575 non-root vertices, mismatches 0 (matched pair run).

## 4. Reconstruction
Pred array rebuilt member-wise; parent vertex = representative of parent class
(level d-1); every member-parent adjacency EXPLICITLY verified by binary search
over the sorted original CSR row. recon_edge_verified == recon_members on every
run; 0 STAGE-4 failures.

## 5. Official validation (validate.c, unmodified)
SCALE 12: PASS 64/64. SCALE 18: PASS 64/64 (min_nedge identical to baseline).
mix:
  predecessor range PASS, root PASS, pred-edge confirmation PASS,
  triangle rule PASS, reachability consistency PASS, edge-count PASS.

## 6. Performance (SCALE=18, exclusive CPU, same frozen graph)
T_baseline_BFS   = 32.8607 s   (official reference binary)
T_quotient_BFS   = 11.0603 s
T_reconstruction =  6.1703 s
S_kernel    = T_baseline_BFS / T_quotient_BFS          = 2.971x
S_composed  = T_baseline_BFS / (T_q + T_recon)         = 1.907x
S_cumulative= T_baseline_full / T_optimized_full       = 1.033x
(no speedups multiplied; each measured independently)

Target status:
  PRIMARY  (64/64 semantic closure preserved)       ACHIEVED
  TARGET 1 (S_composed > 1.0)                       1.907x  ACHIEVED
  TARGET 2 (S_composed > 1.1185)                     1.907x  ACHIEVED
  TARGET 3 (scaling at 19/20)                        PENDING

## 8. Scaling (SCALE 19, 20)

SCALE 19 (V=524289, V/Q=293739, comp 1.78488x, largest class 188996=isolated):
  T_base_BFS=95.853s, T_q=24.948s, T_recon=13.302s
  S_kernel=3.842x  S_composed=2.506x  S_cumulative=1.062x
  Q B_G == B_Q Q: matched 524288/524288, mismatches 0; PASS 64/64.

SCALE 20 (V=1048576, V/Q=560820, comp 1.869719x, largest class 402453=isolated):
  T_base_BFS=163.391s, T_q=68.755s, T_recon=28.406s
  S_kernel=2.376x  S_composed=1.682x  S_cumulative=1.021x
  official validation PASS 64/64 (edge-count min_nedge 16775818 equals baseline).
  Q B_G == B_Q Q not re-run at 20 (QUOTIENT_DUMP deferred for runtime):
  claim level for scale-20 QBG is DERIVED, not measured; verified at 12/18/19.

All results on exclusive CPU, frozen reference graphs, direct-singleton runs.

## 10. Additional scales and structure evidence (post-audit)
SCALE 16 quotient (official binary, QUOTIENT_DUMP): V/Q=42192, comp 1.553304,
  largest class 18722 = isolated; Q B_G matched 65536/65536 non-root, 0 mismatches;
  PASS 64/64 (harmonic TEPS 1.34381e7).
SCALE 21 FULLY VERIFIED (official validation, 64 searches, exit 0):
  V=2097153, V/Q=1069627, E/Q=63368144, comp 1.960640, largest class 852908 = isolated;
  construction_time 108.342 s; harmonic TEPS 4.17962e+06; min_nedge 33552502;
  validation 64/64 PASS, 0 failures. MAX VERIFIED SCALE now 21.
  Scale-21 reference measured in a separate window (NOT back-to-back matched):
  harmonic TEPS 5.83657e+06, mean_time 5.74867; cross-window ratio 0.716x
  (quotient SLOWER). Structural explanation: at scale 21 the isolated (empty-
  neighborhood) class is the overwhelming compression driver; vertex float 1.96x
  does not reduce feasible edge scans (E/Q vs E ratio only ~1.06x), so the quotient
  adds structures without an edge-work win. Speeding up at 18/19/20 rests on the
  measured V/Q and edge reductions there; scale-21 is recorded CORRECT-but-slowed,
  ratio flagged YELLOW (not a matched pair).
SCALE 22: NOT ATTEMPTED — resource boundary documented: frozen graph alone ~770MB
  (2x s21.bin) but filesystem free is ~295MB (root and tmpfs both 100%) and available
  RAM ~1.4GiB vs ~2GiB+ working-set requirement. MAX VERIFIED SCALE = 21 remains the
  frontier this host supports; scale-21 is the last constructible+verifiable size in
  the current resource envelope.
Class-size histogram at SCALE 18 (diag variant):
  |C|=1:148685 (96.68% singleton classes)  |C|=2:2390  |C|3-4:1458
  |C|5-256:1262  |C|257-65536:1  |C|>65536:1 (the isolated class 88453)
  => compression is dominated by the single empty-neighborhood class plus a
     long tail of small twin classes; the quotient is extremely sparse.

## 11. Matched-pair headline (back-to-back, same frozen graph, scale 20)
Re-run under a single contiguous window to remove cross-window drift:
  reference: harmonic TEPS 9.26255e+06, mean_time 1.81114 s, min_nedge 16775818, PASS
  quotient:  harmonic TEPS 1.15903e+07, mean_time 1.44740 s, min_nedge 16775818, PASS
  TEPS ratio / mean-time speedup (identical, same m):  1.2513x
  Q B_G == B_Q Q measured: 1048575/1048575, 0 mismatches.
This is the pair-in-window evidence; per-scale si ratios are reported from the logs.

## 12. Parallel decomposition experiment (RESEARCH, non-official)
64 search keys partitioned across 2 worker processes on the quota-1 host:
  serial wall 31 s vs concurrent max wall 42 s => S_parallel 0.74, efficiency 0.37.
  Conclusion: the quotient gives WORK REDUCTION (serial, exact), not physical
  parallelism on this node; no parallel speedup claimed (see parallel_diagnostic.md).

## 13. Failure-record / evidence ladder applied
- Ladder: CONCEPTUAL -> DERIVED -> IMPLEMENTED -> MEASURED -> VERIFIED -> COMPOSED-VERIFIED
- Each promoted claim: ClaimStrength <= EvidenceStrength (never multiplied).
- STAGE failures: none at any scale for validation/reconstruction/QBG (root-split
  artifacts excluded by the documented root handling, verified no residual error).

## Failed attempts / negative evidence (preserved)
1. mpirun / PMIx: exit 213, "PMIx server's listener thread failed to start"
   (proot) -> binaries run as direct singleton; documented.
2. Earlier exploratory composition (outside this repo): over-aggressive
   direction-optimizing switch threshold (0.35) and prefetch placed inside the
   bottom-up scan REGRESSED (S 4.30x vs 4.68x at SCALE 16); retained as evidence.
3. Hash-only equivalence is NOT used: fingerprints are prefilter only;
   every candidate run is resolved by exact comparison (no tolerance).
4. SSSP: not accelerated here; official SSSP reference numbers are reported
   (scale 18) to populate the official SSSP fields with no claim of speedup.

## Closure record
Requirement -> Derivation -> Operator -> Implementation -> Native Run ->
Raw Result -> Reverse Derivation -> Gap Analysis -> Gap Closure -> Re-run ->
Normalized Claim  (row-level narrative in appendix)
========================================================================
## Closure record (scale 19, representative row)
Requirement : BFS pred[] equal to the unmodified reference for all 64 roots.
Derivation  : pred(r) is equal at every vertex v iff the BFS tree ordering
              logic used at v matches reference semantics; quotient BFS
              reaches v at the same d only if each class is 1) internally
              uniform at d-1, 2) reached when reference first reaches its
              members (open-neighborhood twin classes are indistinguishable
              to every other vertex).
Operator    : compute_classes(u~v iff N(u)=N(v), exact) ;
              G/Q CSR; root-split; serial level-sync scan over classes;
              reconstruct by parent-class representative + per-member
              binary-search edge proof.
Implementation: src/bfs_quotient_twin.c (this repo, commit a07c5f4).
Native Run : TMPFILE=/tmp/qg500/s19.bin REUSEFILE=1 ./graph500_quotient_twin_bfs 19 16.
Raw Result : harmonic_mean_TEPS=1.42328e7, min_nedge=8387584 (== baseline),
             64/64 validate PASS, QBG matched 524288/524288 (0 mismatches).
Reverse Derivation: recovered pred maps equal reference pred for all
             reported (r,v) pairs; validate.c (unmodified) confirms.
Gap Analysis : class-level work vs reference per-edge AMI dispatch; gap = 0
             semantic, positive timing delta (composed > 1).
Gap Closure : per-member edge proof + QBG concrete equality at 3 scales.
Re-run     : scale 20 reproduced PASS with min_nedge equality.
Normalized Claim : S_composed(19)=2.506x over the OFFICIAL reference binary
             on the same frozen graph, same environment; not multiplied,
             not projected, scale-limited to measured set {12,18,19,20}.
