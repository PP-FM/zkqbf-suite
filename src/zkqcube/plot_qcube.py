#!/usr/bin/env python3
"""
Parse zkQRP *.result files under ROOT/*/ (recursively) and plot metrics.

Extracted per *.result:
  - ncls        -> Clauses
  - DEGREE      -> Proof width
  - t           -> Running time (seconds)
  - communication:<bytes>  -> Communication (KB)

Derived:
  - proof_size = ncls * DEGREE
  - communication_kb = communication / 1024

Outputs:
  - CSV   : <OUT_DIR>/zkqrp_metrics.csv
  - Plots : six PNGs in <OUT_DIR> (one scatter per figure, log–log)

Usage:
  python parse_and_plot.py [--root benchmark/benchmarks_qrp_every5ish/False] [--out outdir]
"""

from __future__ import annotations
import argparse
import re
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


def parse_args() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--root", default="benchmark/benchmarks_qcube/True",
                   help="ROOT containing instance subfolders (DEFAULT: %(default)s)")
    p.add_argument("--out", default="plots", help="Output directory for CSV/PNGs")
    return p.parse_args()


# ----------- parsing helpers -----------
def last_int(pattern: str, text: str) -> int | None:
    m = list(re.finditer(pattern, text))
    return int(m[-1].group(1)) if m else None

def last_float(pattern: str, text: str) -> float | None:
    m = list(re.finditer(pattern, text))
    return float(m[-1].group(1)) if m else None


def parse_result_file(fp: Path) -> dict | None:
    """
    Return a dict with parsed fields or None if essentials are missing.
    """
    text = fp.read_text(errors="ignore")

    ncls  = last_int(r"\bncls\s+(\d+)\b", text)
    deg   = last_int(r"\bDEGREE\s+(\d+)\b", text)
    tsec  = last_float(r"\bt\s+([0-9]+(?:\.[0-9]+)?)\b", text)
    commB = last_int(r"communication:(\d+)", text)

    if None in (ncls, deg, tsec, commB):
        return None

    inst_dir = fp.parent.name  # * (instance folder)
    return {
        "instance": inst_dir,
        "file": str(fp),
        "ncls": ncls,
        "degree": deg,
        "proof_size": ncls * deg,
        "time_s": tsec,
        "communication_kb": commB / 1024.0,
    }


# ----------- plotting -----------
def scatter_log(df: pd.DataFrame, x: str, y: str, xlabel: str, ylabel: str,
                title: str, out_path: Path) -> None:
    if df.empty:
        return
    plt.figure()
    plt.scatter(df[x], df[y], s=12)     # no seaborn; one chart per figure; no explicit colors
    plt.xscale("log"); plt.yscale("log")
    plt.xlabel(xlabel); plt.ylabel(ylabel); plt.title(title)
    plt.grid(True, which="both", linestyle="--", linewidth=0.5)
    plt.tight_layout()
    plt.savefig(out_path, dpi=220)
    plt.close()


def main() -> None:
    args = parse_args()
    root = Path(args.root)
    out_dir = Path(args.out); out_dir.mkdir(parents=True, exist_ok=True)

    if not root.exists():
        raise SystemExit(f"[ERR] ROOT not found: {root.resolve()}")

    rows = []
    # Traverse ALL folders under ROOT (recursively) and pick every *.result
    for fp in root.rglob("*prover_zkqrp.result"):
        rec = parse_result_file(fp)
        if rec:
            rows.append(rec)

    df = pd.DataFrame(rows).sort_values(["instance", "file"]).reset_index(drop=True)
    csv_path = out_dir / "zkqrp_metrics.csv"
    df.to_csv(csv_path, index=False)
    print(f"[OK ] parsed rows: {len(df)}  -> {csv_path}")

    # 6 figures (log–log)
    scatter_log(df, "ncls", "communication_kb",
                "Clauses", "Communication (KB)",
                "Communication vs Clauses", out_dir / "comm_vs_clauses.png")
    scatter_log(df, "degree", "communication_kb",
                "Proof width", "Communication (KB)",
                "Communication vs Proof width", out_dir / "comm_vs_width.png")
    scatter_log(df, "proof_size", "communication_kb",
                "Proof size", "Communication (KB)",
                "Communication vs Proof size", out_dir / "comm_vs_size.png")

    scatter_log(df, "ncls", "time_s",
                "Clauses", "Running Time (s)",
                "Running Time vs Clauses", out_dir / "time_vs_clauses.png")
    scatter_log(df, "degree", "time_s",
                "Proof width", "Running Time (s)",
                "Running Time vs Proof width", out_dir / "time_vs_width.png")
    scatter_log(df, "proof_size", "time_s",
                "Proof size", "Running Time (s)",
                "Running Time vs Proof size", out_dir / "time_vs_size.png")

    print(f"[OK ] plots written to: {out_dir.resolve()}")


if __name__ == "__main__":
    main()
