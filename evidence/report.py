#!/usr/bin/env python3
"""Fold official baseline + quotient-twin runs into the required tables and
the GRAPH500 closure report. Claims are taken only from files that actually
exist; no number is invented."""
import json, os, re, statistics, sys, hashlib, datetime

ROOT = "/root/g500_newreference"
EV = os.path.join(ROOT, "evidence", "graph500")
BASE = os.path.join(EV, "baseline")
QOUT = os.path.join(EV, "quotient")

RIGHT = "Run label                                                 Value"
def row(a, b): print(f"{a:<52} {b}")


def parse_official(path):
    d = {}
    if not os.path.exists(path): return None
    txt = open(path).read()
    for key in ["SCALE", "edgefactor"]:
        m = re.search(rf"^{key}:\s+(\S+)", txt, re.M)
        d[key.lower()] = m.group(1) if m else None
    for key in ["NBFS", "num_mpi_processes"]:
        m = re.search(rf"^{key}:\s+(\S+)", txt, re.M)
        d[key.lower()] = int(m.group(1)) if m else None
    for pref in ["bfs  ", "sssp "]:
        for key in ["min_time","firstquartile_time","median_time","thirdquartile_time",
                    "max_time","mean_time","stddev_time",
                    "min_TEPS","firstquartile_TEPS","median_TEPS","thirdquartile_TEPS",
                    "max_TEPS","harmonic_mean_TEPS","harmonic_stddev_TEPS",
                    "min_validate","firstquartile_validate","median_validate",
                    "thirdquartile_validate","max_validate","mean_validate","stddev_validate"]:
            m = re.search(rf"^{pref}{key}:\s*!?\s*([-0-9.eE+]+)", txt, re.M)
            if m: d[f"{pref.strip()}_{key}"] = float(m.group(1))
    m = re.search(r"^min_nedge:\s+(\S+)", txt, re.M)
    d["min_nedge"] = int(m.group(1)) if m else None
    m = re.search(r"^graph_generation:\s+([-0-9.eE+]+)", txt, re.M)
    d["graph_generation_s"] = float(m.group(1)) if m else None
    m = re.search(r"^construction_time:\s+([-0-9.eE+]+)", txt, re.M)
    d["construction_time_s"] = float(m.group(1)) if m else None
    return d


def parse_quotient(path):
    """Per-root times from stderr lines: [quotient] root R qbfs=.. recon=..
    and quotient construction stats from the 'quotient:' line."""
    if not os.path.exists(path): return None
    txt = open(path).read()
    qbfs, recon = [], []
    for m in re.finditer(r"\[quotient\] root \d+ qbfs=([0-9.eE+-]+) recon=([0-9.eE+-]+)", txt):
        qbfs.append(float(m.group(1))); recon.append(float(m.group(2)))
    r = {}
    m = re.search(r"quotient: V=(\d+) E=(\d+) V/Q=(\d+) E/Q=(\d+) \|V\|/\|V/Q\|=([.0-9]+) largest_class=(\d+) isolated=(\d+)", txt)
    if m:
        r["V"] = int(m.group(1)); r["E_adj"] = int(m.group(2)); r["V_over_Q"] = int(m.group(3))
        r["E_over_Q"] = int(m.group(4)); r["compression"] = float(m.group(5))
        r["largest_class"] = int(m.group(6)); r["isolated"] = int(m.group(7))
    m = re.search(r"times: CSR ([.0-9]+) sort ([.0-9]+) fp ([.0-9]+) classify ([.0-9]+) quotientCSR ([.0-9]+) \(total ([.0-9]+)\)", txt)
    if m:
        r["T_CSR_s"] = float(m.group(1)); r["T_sort_s"] = float(m.group(2))
        r["T_fp_s"] = float(m.group(3)); r["T_classify_s"] = float(m.group(4))
        r["T_quotientCSR_s"] = float(m.group(5)); r["T_total_build_s"] = float(m.group(6))
    r["qbfs_per_root"] = qbfs
    r["recon_per_root"] = recon
    r["T_quotient_bfs_s"] = sum(qbfs)
    r["T_reconstruction_s"] = sum(recon)
    r["nroots"] = len(qbfs)
    r["qbfs_mean"] = statistics.mean(qbfs) if qbfs else None
    m = re.search(r"matched (\d+)/(\d+) non-root vertices .*mismatches=(\d+)", txt)
    r["qbg_check"] = (int(m.group(1)), int(m.group(2)), int(m.group(3))) if m else None
    r["stages_failed"] = bool(re.search(r"STAGE\d+ FAIL", txt))
    r["validation"] = "no results printed" if "No results printed" in txt else (
        "passed" if re.search(r"harmonic_mean_TEPS", txt) else "UNKNOWN")
    return r


def summarize(scales):
    rows = []
    for s in scales:
        base = parse_official(os.path.join(BASE, f"official_bfs_scale{s}.log"))
        quo = parse_quotient(os.path.join(QOUT, f"quotient_bfs_scale{s}.log"))
        if base is None or quo is None:
            print(f"-- scale {s}: missing logs (base={base is not None}, quo={quo is not None})")
            continue
        Tb = base["bfs_mean_time"] * base["nbfs"]
        Tq = quo["T_quotient_bfs_s"]
        Tr = quo["T_reconstruction_s"]
        S_kernel = Tb / Tq if Tq else None
        S_composed = Tb / (Tq + Tr) if (Tq + Tr) else None
        Tb_full = (base["construction_time_s"] + Tb + base["bfs_mean_validate"] * base["nbfs"])
        To_full = (quo["T_total_build_s"] + Tq + Tr + base["bfs_mean_validate"] * base["nbfs"])
        S_cumulative = Tb_full / To_full if To_full else None
        rows.append({
            "scale": s, "base": base, "quo": quo,
            "T_baseline_BFS": Tb, "T_quotient_BFS": Tq, "T_reconstruction": Tr,
            "S_kernel": S_kernel, "S_composed": S_composed,
            "T_baseline_full": Tb_full, "T_optimized_full": To_full,
            "S_cumulative": S_cumulative})
    return rows


if __name__ == "__main__":
    scales = [int(a) for a in sys.argv[1:]] or [18]
    row("Graph500 quotient-closure summary", "")
    row("timestamp", datetime.datetime.now(datetime.timezone.utc).isoformat())
    row("repo", "batmeezy918/graph500 @ f89d643 (newreference)")
    for r in summarize(scales):
        s = r["scale"]; b = r["base"]; q = r["quo"]
        print("=" * 72)
        print(f"SCALE {s}  (edgefactor 16, NBFS {b['nbfs']}, procs {b['num_mpi_processes']})")
        print("=" * 72)
        row("official baseline graph_generation_s", f"{b['graph_generation_s']:.6f}")
        row("official baseline construction_time_s", f"{b['construction_time_s']:.6f}")
        row("official baseline bfs mean_time_s", f"{b['bfs_mean_time']:.6f}")
        row("official baseline bfs harmonic_mean_TEPS", f"{b['bfs_harmonic_mean_TEPS']:.6g}")
        row("min_nedge", f"{b['min_nedge']}")
        print("-" * 72)
        row("quote V (vertices)", f"{q['V']}")
        row("quote E (directed adjacency incl. symm) ", f"{q['E_adj']}")
        row("quote V/Q (twin classes)", f"{q['V_over_Q']}")
        row("quote E/Q (distinct class->class)", f"{q['E_over_Q']}")
        row("quote |V|/|V/Q| (compression)", f"{q['compression']}")
        row("quote largest class", f"{q['largest_class']}")
        row("quote isolated vertices (one twin class)", f"{q['isolated']}")
        print("-" * 72)
        row("T_baseline_BFS (64 roots, mean*NBFS)", f"{r['T_baseline_BFS']:.6f}")
        row("T_quotient_BFS (64 roots)", f"{r['T_quotient_BFS']:.6f}")
        row("T_reconstruction (64 roots)", f"{r['T_reconstruction']:.6f}")
        row("S_kernel  = T_baseline_BFS / T_quotient_BFS", f"{r['S_kernel']:.6f}x")
        row("S_composed = T_baseline_BFS / (T_q + T_recon)", f"{r['S_composed']:.6f}x")
        row("T_baseline_full (constr+bfs+validate)", f"{r['T_baseline_full']:.6f}")
        row("T_optimized_full (build+q+recon+validate)", f"{r['T_optimized_full']:.6f}")
        row("S_cumulative = T_full_base / T_full_opt", f"{r['S_cumulative']:.6f}x" if r['S_cumulative'] else "n/a")
        row("Q B_G == B_Q Q (non-root vertices)", f"{q['qbg_check']}")
        row("STAGE failure lines", "yes" if q["stages_failed"] else "no")
        row("official validation", q["validation"])
        print("=" * 72)