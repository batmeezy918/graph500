# Research baseline — FROZEN HISTORICAL EVIDENCE (pre-submission)

Status: historical research record. Do NOT overwrite. Not the new submission claim.

Experiment commit:         a07c5f4 (src/bfs_quotient_twin.c, Makefile, report.py)
Upstream (fork origin):    batmeezy918/graph500 @ newreference == f89d643 (3.0.1)
Kernel:                    exact open-neighborhood equivalence u~v iff N(u)=N(v),
                           64-bit fingerprint prefilter + exact row comparison.
Validation:                unmodified official validate.c.
Environment:               Android-16 proot, aarch64 Cortex-A55, cgroup quota 1 CPU.

Recorded (from evidence/graph500/baseline/*, evidence/graph500/quotient/*, report.py):

  SCALE | V/Q     | compression | largest class | Q B_G = B_Q Q   | official validation | S_kernel | S_composed | S_cumulative
  ------+---------+-------------+---------------+------------------+--------------------+----------+------------+--------------
   12   |    3153 | 1.299x      | 743 (isolated)| 4095/4096, 0 mm  | PASS 64/64         |    -     |     -      |      -
   18   |  153797 | 1.7045x     | 88453 (is.)   | 262144/262144,0  | PASS 64/64         |  2.971x  |   1.907x   |   1.033x
   19   |  293739 | 1.7849x     | 188996 (is.)  | 524288/524288,0  | PASS 64/64         |  3.842x  |   2.506x   |   1.062x
   20   |  560820 | 1.8697x     | 402453 (is.)  | derived (no rerun)| PASS 64/64        |  2.376x  |   1.682x   |   1.021x

Reference (official binary, same frozen graphs/exclusive CPU):
  SCALE 18 harmonic TEPS 8.16728e6 (mean_time 0.513448 s)
  SCALE 19 harmonic TEPS 5.60031e6 (mean_time 1.497700 s)
  SCALE 20 harmonic TEPS 6.57104e6 (mean_time 2.552990 s)

Quotient (research binary):
  SCALE 18 harmonic TEPS 1.58535e7   S_composed 1.907x  (qbfs+recon mean/root ~0.27 s)
  SCALE 19 harmonic TEPS 1.42328e7   S_composed 2.506x
  SCALE 20 harmonic TEPS 1.11964e7   S_composed 1.682x

Interpretation for the submission audit:
  - The historical composite ratios (S_kernel/S_composed/S_cumulative) are diagnostic
    (RESEARCH). The metric that would be submitted is the official TEPS line.
  - min_nedge equality (4193472/8387584/16775818) confirms identical traversed-edge
    accounting (source of TEPS numerator) between reference and quotient.
  - The user-designated baseline table above is preserved intact as the frozen prior.

FILES
  evidence/graph500/baseline/official_bfs_scale{12,18,19,20}.log  (reference runs)
  evidence/graph500/baseline/official_sssp_scale18.log            (reference SSSP)
  evidence/graph500/quotient/quotient_bfs_scale{12,18,19,20}.log  (quotient runs)
  evidence/report.py                                              (aggregation)