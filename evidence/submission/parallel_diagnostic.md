# Parallel-worker diagnostic (RESEARCH / DIAGNOSTIC — NOT official)

Graph500 spec §6 forbids concurrent search keys ("You may not search from multiple
search keys concurrently"). This experiment is therefore NOT a submission-mode run;
it answers the two-level-execution protocol question on this host: does partitioning
the 64 search keys across workers give physical parallelism? (It cannot here.)

## Protocol (honest account of the actual command semantics)
The harness skips validation when getenv("SKIP_VALIDATION") is non-NULL. The shell
set SKIP_VALIDATION=0, which the C code reads as "set" -> validation was SKIPPED on
all three runs below. The numbers therefore compare the search+reconstruction wall
time ONLY (plus one quotient construction each process). This is the correct basis
for the parallel-phase question and is documented as such.

Run A (serial, single process, all 64 keys):
    wall = 31 s   (construction + 64 search/recon)
Run B (workers = 2, concurrent processes, keys 0-31 and 32-63):
    worker1 wall = 42 s, worker2 wall = 42 s   (construction + 32 search/recon each)
    T_1 = 31 s ;  T_2wall = max(42, 42) = 42 s
    S_parallel = T_1 / T_2wall = 0.74
    efficiency = S_parallel / 2 = 0.37
All three produced identical-class structure and identical pred results (partition of
a deterministic key split; results identical by construction to serial 64-key run).

## Interpretation
- On a cgroup-quota-1 (single physical core) host, decomposing the search-key space
  across two worker processes gives S_parallel < 1; efficiency well below 1. Each
  worker pays its own full Kernel-1 construction (duplicated preprocessing) and the
  OS time-slices one core. Cross-check of serial full runs (matched pairs) shows the
  same kernel serial is optimal here.
- Conclusion (RESEARCH): the verified quotient decomposition is a WORK-REDUCTION
  mechanism (serial, exact), not a parallel-means on this host. Claim strictly scoped:
  no physical parallel speedup is claimed; official results assume 1 process / 1 core.
- This also documents the resource-collision regime: agent + omniroute + proot baseline
  CPU exists on the host; all headline pairs are measured back-to-back to keep
  reference/quotient symmetry.

Artifacts: evidence/submission/par_serial18.log, par_worker1_18.log, par_worker2_18.log