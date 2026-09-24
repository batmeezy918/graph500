# READY FOR THE NEXT GRAPH500 SUBMISSION WINDOW

Current state on run date (2026-09-24): the homepage lists the latest results as
"June 2026 BFS/SSSP" and quotes "Submission deadline for next BFS & SSSP is June 1st
2026" — i.e. the last named window has passed. Graph500 lists are published twice a
year (a June and a November list). A new window (nominally Nov-2026 / SC or June-2027)
has NOT yet been officially announced at fetch time.

THIS PACKAGE IS NOT BEING SUBMITTED NOW; it is staged so that when a window opens the
exact remaining actions are:

## Blocking condition
1. Submission portal for an OPEN (future) window not yet announced → cannot file.
2. No personal authorization to submit identity/contact data → human approval step.

## Exact remaining actions when a window opens
1. Re-confirm graph500.org "Submissions" page (?page_id=182) for the current form,
   fields (SCALE, GTEPS, machine info, nodes/cores, installation, contact, and the
   run output), deadline, and accepted formats.
2. Re-run the headline measurements in this package ON THE OFFICIAL submission
   machine (optional; our probe node is a single-quota Cortex-A55 — performance is
   representative only for this node) and paste the official-format output block.
3. Provide: the SCALE selection, the `bfs harmonic_mean_TEPS` (GTEPS) value, and the
   required machine/contact metadata.
4. Include the README classification note that the submitted numbers are produced
   with the unmodified reference harness plus the documented Kernel-2 drop-in, full
   validation enabled, per the legal timing model (evidence/timing_model.md).
5. Human approval before clicking submit (identity/contact fields required).

## Files ready in this package
evidence/submission/README.md, system.txt, compiler.txt, graph500_commit.txt,
algorithm_commit.txt, build_commands.sh, run_commands.sh, timing_model.md,
official_specification.md, quotient_statistics.csv, performance_comparison.csv,
baseline/quotient/validation outputs, checksums.sha256.