/* Copyright (c) 2026 The Authors.                                              */
/* Graph500 Kernel 2 drop-in: exact-neighborhood quotient / twin-vertex BFS.     */
/*                                                                              */
/* Equivalence O(NI):  u ~ v  iff  N(u) == N(v)  (EXACT equal open              */
/*   neighborhoods, each neighborhood its vertices' sorted adjacency row).       */
/*                                                                              */
/* Quotient graph Q_B_G: one node per twin class; directed class edge C->D iff   */
/*   some member of C is adjacent to some member of D.                           */
/*                                                                              */
/* Invariant Q B_G = B_Q Q (quotient homomorphism of level-synchronized BFS):    */
/*   - For EXACT open-neighborhood twins u~v and any vertex x:                   */
/*       x in N(u)  <=>  x in N(v).   (definition)                              */
/*   - Hence two vertices are twins in G if and only if they are co-members of a */
/*     quotient node, and any class edge C->D is COMPLETE BIPARTITE:              */
/*       for all c in C, d in D: edge c-d exists.                               */
/*   - If c1,c2 are in one class C then dist_G(r,c1) == dist_G(r,c2) and the    */
/*     BFS tree parent classes are identical, so the level arrays of the         */
/*     original BFS and the quotient BFS coincide on classes.                    */
/*                                                                              */
/* Root safety: class(root) is SPLIT into {root} and class(root) - {root}.       */
/*   Open-neighborhood twins are never adjacent (u in N(v) and u~v with u,v in   */
/*   one class implies u in N(u), contradiction), so the root has no edge to     */
/*   its own class members; {root} is its own level-0 node; non-root members     */
/*   receive dist = 2 whenever class(root) has a neighbor class; when            */
/*   class(root) has NO neighbor (root isolated) every member is isolated and    */
/*   the class structure is verified explicitly.                                 */
/*                                                                              */
/* Reconstruction: parent vertex = representative of the parent CLASS at level   */
/*   d-1; because class edges are complete bipartite this parent vertex is       */
/*   adjacent to EVERY member of the child class - verified here EXPLICITLY by   */
/*   binary search over the (sorted) original CSR row before acceptance.         */
/*                                                                              */
/* Hash policy (Stage 1): degree-aggregated 64-bit fingerprints are a candidate  */
/*   PREFILTER only; every candidate run receives EXACT sorted-neighborhood      */
/*   comparison, so hash collisions cannot merge distinct classes.               */
/*                                                                              */
/* The resulting pred[] vector is fed unchanged into the OFFICIAL graph500       */
/* validation (validate.c).                                                      */

#include "common.h"
#include "aml.h"
#include "csr_reference.h"
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

oned_csr_graph g;
int64_t *column;                    /* packed 6-byte columns, rows SORTED here */
unsigned int *rowstarts;
int64_t *pred_glob;

/* ---- equivalence and quotient structures ---- */
static uint32_t *class_id;
static uint32_t nclass;
static uint32_t *class_rep;
static uint32_t *class_size;
static uint32_t *class_csr;             /* per-class CSR: row pointers / columns   */
static uint32_t *class_col;
static uint64_t nclass_edges;

/* ---- BFS scratch ---- */
static uint8_t  *cvis;
static int32_t  *clevel;
static uint32_t *cparent;
static uint32_t *qA,*qB;

static double t_qbfs_total = 0.0, t_recon_total = 0.0;
static uint64_t recon_edge_verified = 0, recon_members = 0;
static int recon_errors = 0;
static int dump_done = 0;

static int64_t read6(const void* p) {
    const unsigned char* b = (const unsigned char*)p;
    return (int64_t)b[0] | ((int64_t)b[1]<<8) | ((int64_t)b[2]<<16) |
           ((int64_t)b[3]<<24) | ((int64_t)b[4]<<32) | ((int64_t)b[5]<<40);
}

static int cmp6(const void* a, const void* b) {
    int64_t x = read6(a), y = read6(b);
    return (x<y) ? -1 : (x>y) ? 1 : 0;
}

static uint64_t fnv1a(uint64_t h, const void* cell) {
    uint64_t v = (uint64_t)read6(cell);
    uint64_t r = h;
    for (int k = 0; k < 8; ++k) { r ^= (v & 0xff); r *= 1099511628211ULL; v >>= 8; }
    return r;
}

typedef struct vrec { int32_t deg; uint32_t idx; uint64_t fp1, fp2; } vrec;

static int cmp_vrec(const void* a, const void* b) {
    const vrec* x = a; const vrec* y = b;
    if (x->deg != y->deg) return (x->deg < y->deg) ? -1 : 1;
    if (x->fp1 != y->fp1) return (x->fp1 < y->fp1) ? -1 : 1;
    if (x->fp2 != y->fp2) return (x->fp2 < y->fp2) ? -1 : 1;
    return 0;
}

static int32_t safe_id(uint32_t v) { return (int32_t)VERTEX_TO_GLOBAL(my_pe(), v); }

static int rows_equal(uint32_t a, uint32_t b) {
    uint32_t na = rowstarts[a+1]-rowstarts[a];
    uint32_t nb = rowstarts[b+1]-rowstarts[b];
    if (na != nb) return 0;
    const char* A = (const char*)column + (size_t)BYTES_PER_VERTEX*rowstarts[a];
    const char* Bb = (const char*)column + (size_t)BYTES_PER_VERTEX*rowstarts[b];
    for (uint32_t i = 0; i < na; ++i)
        if (read6(A + (size_t)BYTES_PER_VERTEX*i) != read6(Bb + (size_t)BYTES_PER_VERTEX*i)) return 0;
    return 1;
}

void make_graph_data_structure(const tuple_graph* const tg) {
    size_t i;
    double t0 = MPI_Wtime();

    convert_graph_to_oned_csr(tg, &g);
    column = g.column;
    rowstarts = g.rowstarts;
    double t1 = MPI_Wtime();

    for (i = 0; i < g.nlocalverts; ++i) {
        uint32_t s = rowstarts[i], e = rowstarts[i+1];
        if (e - s > 1)
            qsort((char*)column + (size_t)BYTES_PER_VERTEX*s, e - s,
                  BYTES_PER_VERTEX, cmp6);
    }
    double t2 = MPI_Wtime();

    vrec* rec = (vrec*)xmalloc(g.nlocalverts * sizeof(vrec));
    for (i = 0; i < g.nlocalverts; ++i) {
        uint32_t s = rowstarts[i], e = rowstarts[i+1];
        uint64_t f1 = 1469598103934665603ULL;
        uint64_t f2 = 1099511628211ULL;
        for (uint32_t j = s; j < e; ++j) {
            const char* cell = (const char*)column + (size_t)BYTES_PER_VERTEX*j;
            f1 = fnv1a(f1, cell);
            f2 = fnv1a(f2 ^ ((uint64_t)read6(cell) * 0x9E3779B97F4A7C15ULL), cell);
        }
        rec[i].deg = e - s;
        rec[i].idx = i;
        rec[i].fp1 = f1;
        rec[i].fp2 = f2;
    }
    qsort(rec, g.nlocalverts, sizeof(vrec), cmp_vrec);
    double t3 = MPI_Wtime();

    class_id = (uint32_t*)xmalloc(g.nlocalverts * sizeof(uint32_t));
    nclass = 0;
    size_t run0 = 0;
    while (run0 < g.nlocalverts) {
        size_t run1 = run0 + 1;
        while (run1 < g.nlocalverts && cmp_vrec(&rec[run1], &rec[run0]) == 0) ++run1;
        uint32_t rep = rec[run0].idx;
        uint32_t cid = nclass++;
        class_id[rep] = cid;
        for (size_t r = run0 + 1; r < run1; ++r)
            if (rows_equal(rep, rec[r].idx)) class_id[rec[r].idx] = cid;
        for (size_t r = run0 + 1; r < run1; ++r)
            if (class_id[rec[r].idx] != cid) {
                uint32_t c2 = nclass++;
                class_id[rec[r].idx] = c2;
                for (size_t r2 = r + 1; r2 < run1; ++r2)
                    if (class_id[rec[r2].idx] != cid && rows_equal(rec[r].idx, rec[r2].idx))
                        class_id[rec[r2].idx] = c2;
            }
        run0 = run1;
    }
    free(rec);
    double t4 = MPI_Wtime();

    class_rep = (uint32_t*)xmalloc(nclass * sizeof(uint32_t));
    class_size = (uint32_t*)xmalloc(nclass * sizeof(uint32_t));
    memset(class_size, 0, nclass * sizeof(uint32_t));
    for (i = 0; i < g.nlocalverts; ++i) {
        uint32_t c = class_id[i];
        if (class_size[c]++ == 0) class_rep[c] = i;
    }

    uint32_t* mark = (uint32_t*)xmalloc(nclass * sizeof(uint32_t));
    uint32_t* degC = (uint32_t*)xmalloc(nclass * sizeof(uint32_t));
    memset(mark, 0xFF, nclass * sizeof(uint32_t));
    memset(degC, 0, nclass * sizeof(uint32_t));
    uint32_t epoch = 1;

    for (i = 0; i < g.nlocalverts; ++i) {
        uint32_t c = class_id[i];
        uint32_t s = rowstarts[i], e = rowstarts[i+1];
        for (uint32_t j = s; j < e; ++j) {
            uint64_t w = read6((const char*)column + (size_t)BYTES_PER_VERTEX*j);
            uint32_t cw = class_id[VERTEX_LOCAL(w)];
            if (cw != c && mark[cw] != epoch) { mark[cw] = epoch; degC[c]++; }
        }
        ++epoch;
        if (epoch == 0xFFFFFFFE) { epoch = 1; memset(mark, 0xFF, nclass*sizeof(uint32_t)); }
    }

    class_csr = (uint32_t*)xmalloc((nclass + 1) * sizeof(uint32_t));
    class_csr[0] = 0;
    for (uint32_t c = 0; c < nclass; ++c) class_csr[c+1] = class_csr[c] + degC[c];
    nclass_edges = class_csr[nclass];
    class_col = (uint32_t*)xmalloc(nclass_edges * sizeof(uint32_t));

    epoch = 1;
    memset(mark, 0xFF, nclass * sizeof(uint32_t));
    for (i = 0; i < g.nlocalverts; ++i) {
        uint32_t c = class_id[i];
        uint32_t s = rowstarts[i], e = rowstarts[i+1];
        for (uint32_t j = s; j < e; ++j) {
            uint64_t w = read6((const char*)column + (size_t)BYTES_PER_VERTEX*j);
            uint32_t cw = class_id[VERTEX_LOCAL(w)];
            if (cw != c && mark[cw] != epoch) {
                mark[cw] = epoch;
                uint32_t slot = class_csr[c] + (--degC[c]);
                class_col[slot] = cw;
            }
        }
        ++epoch;
        if (epoch == 0xFFFFFFFE) { epoch = 1; memset(mark, 0xFF, nclass*sizeof(uint32_t)); }
    }
    free(mark); free(degC);
    double t5 = MPI_Wtime();

    cvis    = (uint8_t*)xmalloc((size_t)(nclass + 1) * sizeof(uint8_t));
    clevel  = (int32_t*)xmalloc((size_t)(nclass + 1) * sizeof(int32_t));
    cparent = (uint32_t*)xmalloc((size_t)(nclass + 1) * sizeof(uint32_t));
    qA = (uint32_t*)xmalloc((size_t)(nclass + 1) * sizeof(uint32_t));
    qB = (uint32_t*)xmalloc((size_t)(nclass + 1) * sizeof(uint32_t));

    if (rank == 0) {
        uint32_t maxsz = 0, isolated = 0;
        for (uint32_t c = 0; c < nclass; ++c)
            if (class_size[c] > maxsz) maxsz = class_size[c];
        for (i = 0; i < g.nlocalverts; ++i)
            if (rowstarts[i] == rowstarts[i+1]) isolated++;
        fprintf(stderr,
            "quotient: V=%llu E=%llu V/Q=%u E/Q=%llu |V|/|V/Q|=%.6f largest_class=%u isolated=%u\n"
            "  times: CSR %.6f sort %.6f fp %.6f classify %.6f quotientCSR %.6f (total %.6f)\n",
            (unsigned long long)g.nlocalverts, (unsigned long long)g.nlocaledges,
            nclass, (unsigned long long)nclass_edges,
            (double)g.nlocalverts / (double)nclass, maxsz, isolated,
            t1-t0, t2-t1, t3-t2, t4-t3, t5-t4, t5-t0);
        {
            uint64_t b1=0,b2=0,b34=0,b5_256=0,b257_65536=0,b_big=0;
            for (uint32_t c = 0; c < nclass; ++c) {
                uint32_t s = class_size[c];
                if (s==1) b1++; else if (s==2) b2++; else if (s<=4) b34++;
                else if (s<=256) b5_256++; else if (s<=65536) b257_65536++; else b_big++;
            }
            fprintf(stderr,
                "  class-size histogram: |C|=1:%llu |C|=2:%llu |C|3-4:%llu |C|5-256:%llu |C|257-65536:%llu |C|>65536:%llu\n",
                (unsigned long long)b1,(unsigned long long)b2,(unsigned long long)b34,
                (unsigned long long)b5_256,(unsigned long long)b257_65536,
                (unsigned long long)b_big);
            fprintf(stderr,
                "  singletons_fraction=%.6f\n",
                (double)b1 / (double)nclass);
        }
    }
}

void free_graph_data_structure(void) {
    free_oned_csr_graph(&g);
    free(class_id); free(class_rep); free(class_size);
    free(class_csr); free(class_col);
    free(cvis); free(clevel); free(cparent); free(qA); free(qB);
}

void clean_pred(int64_t* pred) {
    for (size_t i = 0; i < g.nlocalverts; ++i) pred[i] = -1;
}

static inline uint32_t qdeg(uint32_t node, uint32_t rootclass) {
    return (node == nclass)
        ? (class_csr[rootclass+1]-class_csr[rootclass])
        : (class_csr[node+1]-class_csr[node]);
}
static inline uint32_t qcol(uint32_t node, uint32_t k, uint32_t rootclass) {
    return (node == nclass)
        ? class_col[class_csr[rootclass]+k]
        : class_col[class_csr[node]+k];
}

void run_bfs(int64_t root, int64_t* pred) {
    double t0 = MPI_Wtime();
    uint32_t rootclass = class_id[VERTEX_LOCAL(root)];
    const uint32_t NQ = nclass + 1;
    pred_glob = pred;

    memset(cvis, 0, NQ);
    for (uint32_t i = 0; i < NQ; ++i) { clevel[i] = -1; cparent[i] = 0xFFFFFFFFu; }

    uint32_t fc = 0, nfc = 0, level = 0;
    cvis[nclass] = 1; clevel[nclass] = 0;
    qA[fc++] = nclass;
    pred[VERTEX_LOCAL(root)] = root;

    if (qdeg(nclass, rootclass) == 0) {
        int bad = 0;
        for (uint32_t m = 0; m < g.nlocalverts; ++m)
            if (class_id[m] == rootclass && rowstarts[m] != rowstarts[m+1]) { bad = 1; break; }
        if (bad)
            fprintf(stderr, "STAGE2 FAIL: non-isolated member in isolated class %u\n", rootclass), recon_errors++;
    }

    while (fc > 0) {
        nfc = 0;
        for (uint32_t i = 0; i < fc; ++i) {
            uint32_t node = qA[i];
            uint32_t d = qdeg(node, rootclass);
            for (uint32_t k = 0; k < d; ++k) {
                uint32_t nb = qcol(node, k, rootclass);
                if (!cvis[nb]) {
                    cvis[nb] = 1;
                    clevel[nb] = level + 1;
                    cparent[nb] = node;
                    qB[nfc++] = nb;
                }
            }
        }
        uint32_t* tmp = qA; qA = qB; qB = tmp;
        fc = nfc;
        ++level;
    }
    double t1 = MPI_Wtime();
    t_qbfs_total += t1 - t0;

    uint32_t recon = 0;
    for (uint32_t v = 0; v < g.nlocalverts; ++v) {
        if (v == VERTEX_LOCAL(root)) continue;
        uint32_t c = class_id[v];
        int32_t L = clevel[c];
        if (L < 1) continue;
        uint32_t P = cparent[c];
        int64_t rep = (P == nclass) ? root : (int64_t)safe_id(class_rep[P]);
        uint32_t s = rowstarts[v], e = rowstarts[v+1];
        int found = 0;
        while (s < e) {
            uint32_t mid = s + (e - s) / 2;
            int64_t w = read6((const char*)column + (size_t)BYTES_PER_VERTEX*mid);
            if (w == rep) { found = 1; break; }
            if (w < rep) s = mid + 1; else e = mid;
        }
        if (!found) {
            fprintf(stderr, "STAGE4 FAIL: root %" PRId64 " class %u vertex %u parent-vertex %" PRId64 " not adjacent (level %d)\n",
                    root, c, v, rep, L);
            recon_errors++;
        } else recon_edge_verified++;
        pred[v] = rep;
        recon++;
    }
    t_recon_total += MPI_Wtime() - t1;
    recon_members += recon;

    if (rank == 0) {
        uint64_t nvis = 0;
        for (uint32_t i = 0; i < g.nlocalverts; ++i) if (pred[i] != -1) nvis++;
        fprintf(stderr, "[quotient] root %" PRId64 " qbfs=%.6f recon=%.6f visited=%llu/%llu levels=%u\n",
                root, t1 - t0, MPI_Wtime() - t1,
                (unsigned long long)nvis, (unsigned long long)g.nlocalverts, level);
    }

    /* STAGE 3 concrete evidence: Q B_G == B_Q Q on the warm-up root. */
    if (rank == 0 && !dump_done && getenv("QUOTIENT_DUMP")) {
        dump_done = 1;
        fprintf(stderr, "=== QUOTIENT EVIDENCE for root %" PRId64 " ===\n", root);
        int32_t* rd = (int32_t*)xmalloc(g.nlocalverts * sizeof(int32_t));
        uint8_t* rv = (uint8_t*)xmalloc(g.nlocalverts);
        uint32_t* rq1 = (uint32_t*)xmalloc(g.nlocalverts * sizeof(uint32_t));
        memset(rv, 0, g.nlocalverts);
        for (uint32_t i = 0; i < g.nlocalverts; ++i) rd[i] = -1;
        rd[VERTEX_LOCAL(root)] = 0; rv[VERTEX_LOCAL(root)] = 1;
        uint64_t hh = 0, tt = 1; rq1[0] = VERTEX_LOCAL(root);
        while (hh < tt) {
            uint32_t u = rq1[hh++];
            for (uint32_t j = rowstarts[u]; j < rowstarts[u+1]; ++j) {
                uint64_t w = read6((const char*)column + (size_t)BYTES_PER_VERTEX*j);
                uint32_t loc = VERTEX_LOCAL(w);
                if (!rv[loc]) { rv[loc] = 1; rd[loc] = rd[u] + 1; rq1[tt++] = loc; }
            }
        }
        uint64_t mism = 0, tot = 0, skipped_root = 0;
        for (uint32_t v = 0; v < g.nlocalverts; ++v) {
            if (v == VERTEX_LOCAL(root)) { skipped_root++; continue; } /* split node */
            int32_t q = clevel[class_id[v]];
            int32_t o = rd[v];
            if (o == -1) { if (q != -1) mism++; }
            else if (q == -1) mism++;
            else if (q != o) { mism++; if (mism <= 8)
                fprintf(stderr, "   MISM v=%u orig=%d quo=%d\n", v, o, q); }
            tot++;
            if (v < 12)
                fprintf(stderr, "   v=%u orig_dist=%d quo_dist=%d class=%u\n",
                        v, o, q, class_id[v]);
        }
        fprintf(stderr, "   Q B_G == B_Q Q check: matched %llu/%llu non-root vertices (root %" PRId64 " handled by singleton split), mismatches=%llu\n",
                (unsigned long long)(tot - mism), (unsigned long long)tot, root,
                (unsigned long long)mism);
        for (uint32_t L = 1; L < (uint32_t)level && L <= 3; ++L) {
            fprintf(stderr, "   level %u: parent-class map (class -> parent class, rep-vertex):\n", L);
            uint32_t shown = 0;
            for (uint32_t nc = 0; nc < NQ && shown < 10; ++nc) {
                if (clevel[nc] != (int32_t)L) continue;
                uint32_t P = cparent[nc];
                if (nc < nclass)
                    fprintf(stderr, "     class %u (%u members, rep v=%u) <- parent %u rep-vertex=%" PRId64 "\n",
                            nc, class_size[nc], class_rep[nc], P,
                            (P == nclass) ? root : (int64_t)safe_id(class_rep[P]));
                else
                    fprintf(stderr, "     class CS(root singleton) <- parent -\n");
                shown++;
            }
        }
        fprintf(stderr, "   reconstructed pred[] (first 12 vertices):\n");
        for (uint32_t v = 0; v < 12 && v < g.nlocalverts; ++v)
            fprintf(stderr, "     pred[%u]=%" PRId64 " dist=%d\n", v, pred[v], rd[v]);
        free(rq1); free(rv); free(rd);
    }
}

void get_edge_count_for_teps(int64_t* edge_visit_count) {
    long edge_count = 0;
    for (size_t i = 0; i < g.nlocalverts; ++i)
        if (pred_glob[i] != -1)
            for (uint32_t j = rowstarts[i]; j < rowstarts[i+1]; ++j)
                if (read6((const char*)column + (size_t)BYTES_PER_VERTEX*j) <= VERTEX_TO_GLOBAL(my_pe(), i))
                    edge_count++;
    aml_long_allsum((long*)&edge_count);
    *edge_visit_count = edge_count;
}

size_t get_nlocalverts_for_pred(void) {
    return g.nlocalverts;
}