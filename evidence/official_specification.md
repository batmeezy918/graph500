# Official Graph500 Specification — record for submission audit

Source URLs (retrieved 2026-09-24):
- Benchmark specification v2.0: https://graph500.org/?page_id=12
- Rules / submissions: https://graph500.org/?page_id=182
- Repository README (newreference): https://github.com/graph500/graph500/blob/newreference/src/README
- Homepage (deadlines / lists): https://graph500.org/

## Kernel definitions (paraphrased from spec v2.0)
- Kernel 1 — Graph Construction: "The first kernel may transform the edge list to
  any data structures ... that are used for the remaining kernels." "The graph may
  be represented in any manner, but it may not be modified by or between subsequent
  kernels. Space may be reserved in the data structure for marking or locking."
  "The process of constructing the graph data structure ... must be timed."
  => Kernel 2 may READ the Kernel-1 structures; a one-time derived index built by
  Kernel 1 is permitted; per-search marking/locking space is the only mutable space.
- Kernel 2 — BFS: "A BFS of a graph starts with a single source vertex ... produces
  a correct BFS tree as output. We ... do not constrain the choice of BFS algorithm
  itself, as long as it produces a correct BFS tree."
  "You may not search from multiple search keys concurrently. No information can be
  passed between different invocations of this kernel."
  "The routine must return an array containing valid breadth-first search parent
  information (per vertex). The parent of the search key is itself, and the parent
  of any vertex not included in the tree is -1."
  => the parent array IS the Kernel-2 output.
- Kernel 3 — SSSP: separate kernel, cannot use Kernel-2 data. (Not part of this
  submission; included as official fields only.)

## Graph generation
- edgefactor = 16 (iconsists of ratio M/N = (# edges)/(# vertices)).
- N = 2^SCALE, M = edgefactor * N.
- Kronecker / R-MAT generator, initiators A=.57, B=.19, C=.19, D=.05.
- Vertex labels >= 48 bits; BFS-only runs need not store weights.
- Multiple edges, self-loops, isolated vertices "may be ignored in the subsequent
  kernels but must be included in the edge list provided to the first kernel."
- "If stored to disk, the data may be retrieved before starting kernel 1. The data
  generator and retrieval operations need not be timed."

## 64 searches
- "randomly sampled from the vertices ... sample only from vertices that are
  connected to some other vertex. Their degrees, not counting self-loops, must be
  at least one." NBFS = 64 for non-trivial graphs. This sampling step is UNTIMED.
- Neighborhoods are open (self-loops ignored).

## Validation (sec. 8)
"run (but do not time) a function that ensures that the discovered parent tree and
distance vector are correct":
  (1) the tree is a tree and does not contain cycles;
  (2) each tree edge connects vertices whose BFS levels differ by exactly one;
  (3) every edge in the input list has endpoint BFS levels that differ by at most
      one, or both endpoints are not in the tree;
  (4) the tree spans an entire connected component;
  (5) a node and its parent are joined by an edge of the original graph.
"at full scale ... the exact details depend on the pseudo-random number generator
and BFS algorithm used" so validation is soft-checking of these properties. We run
the untouched official validate.c which implements exactly these checks.

## Timing (sec. 9.1)
"Start the time for a search immediately prior to visiting the search root. Stop
the time for that search when the output has been written to memory. Do not time
any I/O outside of the search routine. The spirit of the benchmark is to gauge the
performance of a single search."
=> In the reference harness (main.c lines 374-376) each search is timed exactly as
   MPI_Wtime() around run_bfs(root,&pred); the parent array is the "output written
   to memory", so it MUST be produced inside the timed span. Validation, I/O, and
   Kernel-1 construction are outside the per-search timed span.

## TEPS (sec. 9.2)
TEPS(n) = m / timeK2(n), m = undirected edges in the traversed component
=self-loop tuples within component + half non-self-loop tuples within component.
The reference harness computes m via get_edge_count_for_teps() after each search
and prints TEPS = edge_count / time; the harmonic mean is used to compare rates.
"Because TEPS is a rate, the rates are compared using harmonic means."

## Required output fields (sec 9.3)
SCALE, edgefactor, NBFS, construction_time, bfs_min/firstquartile/median/
thirdquartile/max_time, bfs_mean/stddev_time, bfs_*_nedge, bfs_*_TEPS,
bfs_harmonic_mean_TEPS, bfs_harmonic_stddev_TEPS, and the equivalent sssp_* fields.
"Additional fields are permitted." Kernel-3 fields may be zeroed when only one
kernel is run.

## Submission
- Submissions are accepted through http://www.graph500.org (Submissions page).
- Lists are published twice a year. Latest published list: June 2026 BFS
  (homepage "Top Ten from June 2026 BFS").
- Homepage banner quotes "Submission deadline for next BFS & SSSP is June 1st 2026"
  — that window is past as of the run date (2026-09-24); the next eligible window
  must be consulted on graph500.org before filing.

## Compliance notes for this submission
- Our quotient structures are built ONCE in Kernel 1 (inside make_graph_data_structure,
  included in the timed construction_time) and only READ by the 64 searches; the
  original CSR is not modified. Per-search marking space (visited/level/parent
  arrays) is the only mutable space per search. => consistent with Kernel 1 rules.
- The 64 searches run strictly one at a time (no concurrent search keys), matching
  the "may not search from multiple search keys concurrently" restriction in the
  submission-mode runs. A two-worker run is documented separately as a RESEARCH
  diagnostic and never presented as the official procedure.
- Reconstruction runs inside run_bfs: the writer of the parent array, entirely
  within the timed span; zero work is deferred to validation.
- Validation is the unmodified official validate.c, run outside the timing window.