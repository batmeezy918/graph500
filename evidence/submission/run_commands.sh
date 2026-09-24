#!/bin/bash
export OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1
# official-mode BFS + full validation, per scale (single frozen graph per scale)
TMPFILE=/tmp/qg500/s18.bin REUSEFILE=1 ./graph500_quotient_twin_bfs 18 16
TMPFILE=/tmp/qg500/s19.bin REUSEFILE=1 ./graph500_quotient_twin_bfs 19 16
TMPFILE=/tmp/qg500/s20.bin REUSEFILE=1 ./graph500_quotient_twin_bfs 20 16
# reference comparisons (same frozen graphs)
TMPFILE=/tmp/qg500/s18.bin REUSEFILE=1 ./graph500_reference_bfs 18 16
