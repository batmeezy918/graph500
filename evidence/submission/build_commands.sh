#!/bin/bash
# Open MPI 5.0.7 wrapper; GCC 14.2.0; from repo src/
export CFLAGS_BASE="-Drestrict=__restrict__ -O3 -DGRAPH_GENERATOR_MPI -DREUSE_CSR_FOR_VALIDATION -I../aml -fcommon"
# reference (UNMODIFIED)
mpicc $CFLAGS_BASE -o graph500_reference_bfs bfs_reference.c csr_reference.c \
      main.c utils.c validate.c ../aml/aml.c \
      ../generator/graph_generator.c ../generator/make_graph.c \
      ../generator/splittable_mrg.c ../generator/utils.c -lm
# official-mode quotient twin
mpicc $CFLAGS_BASE -o graph500_quotient_twin_bfs bfs_quotient_twin.c csr_reference.c \
      main.c utils.c validate.c ../aml/aml.c \
      ../generator/graph_generator.c ../generator/make_graph.c \
      ../generator/splittable_mrg.c ../generator/utils.c -lm
