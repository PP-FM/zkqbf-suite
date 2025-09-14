from __future__ import annotations
import argparse
import re
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


def args_parse() -> argparse.Namespace:
    p = argparse.ArgumentParser()
    p.add_argument("--root",
                   default="benchmark/benchmarks_herbrand_for_every5s/False",
                   help="Root directory containing instance subfolders (DEFAULT: %(default)s)")
    p.add_argument("--out", default="plots_herbrand",
                   help="Output folder for CSV and figures")
    return p.parse_args()


# ---------- helpers ----------
def last_int(pat: str, text: str) -> int | None:
    m = list(re.finditer(pat, text, flags=re.MULTILINE))
    return int(m[-1].group(1)) if m else None

def last_float(pat: str, text: str) -> float | None:
    m = list(re.finditer(pat, text, flags=re.MULTILINE))
    return float(m[-1].group(1)) if m else None

def herbrand_inputs_after_header(text: str) -> int | None:
    """
    Find the '----Herbrand Function----' header, then read the first
    'number of input variables: <int>' that follows it.
    """
    hdr = re.search(r"^-{2,}\s*Herbrand Function\s*-{2,}\s*$",
                    text, flags=re.MULTILINE | re.IGNORECASE)
    if not hdr:
        # Sometimes the header may be on a single line without dashes count variations
        hdr = re.search(r"Herbrand Function", text, flags=re.IGNORECASE)
    if not hdr:
        return None
    tail = text[hdr.end():]
    m = re.search(r"number of input variables:\s*(\d+)", tail)
    return int(m.group(1)) if m else None

def get_formula_size_from_cnf(dir_path: Path) -> int | None:
    """
    Look for a '*.cnf' in dir_path and parse its first 'p cnf <vars> <clauses>' line.
    """
    for cnf in sorted(dir_path.glob("*.cnf")):
        try:
            with cnf.open("r", errors="ignore") as fh:
                for line in fh:
                    line = line.strip()
                    if not line or line.startswith("c"):
                        continue
                    m = re.match(r"p\s+cnf\s+(\d+)\s+(\d+)", line)
                    if m:
                        return int(m.group(2))  # clauses
                    # if first non-comment line isn't header, break to try next file
                    break
        except OSError:
            continue
    return None

def plot_scatter_log(df: pd.DataFrame, x: str, y: str,
                     xlabel: str, ylabel: str, title: str, out_png: Path) -> None:
    if df.empty:
        return
    plt.figure()
    plt.scatter(df[x], df[y], s=12)         # matplotlib only; single chart per figure; no explicit colors
    plt.xscale("log"); plt.yscale("log")
    plt.xlabel(xlabel); plt.ylabel(ylabel); plt.title(title)
    plt.grid(True, which="both", linestyle="--", linewidth=0.5)
    plt.tight_layout()
    plt.savefig(out_png, dpi=220)
    plt.close()


def parse_result(fp: Path) -> dict | None:
    """
    Parse a single *_prover.result file into a dict of metrics. Returns None on missing essentials.
    """
    text = fp.read_text(errors="ignore")

    # essentials
    clauses = last_int(r"total number of clauses:\s*(\d+)\b", text)
    proof_width = last_int(r"DEGREE\s+for\s+ZKUNSAT:\s*(\d+)\b", text)
    time_s = last_float(r"\bt\s+([0-9]+(?:\.[0-9]+)?)\b", text)
    commB = last_int(r"communication:(\d+)", text)

    herbrand_inputs = herbrand_inputs_after_header(text)
    formula_size = get_formula_size_from_cnf(fp.parent)

    if None in (clauses, proof_width, time_s, commB, herbrand_inputs):
        return None

    return {
        "instance": fp.parent.name,
        "file": str(fp),
        "clauses": clauses,
        "proof_width": proof_width,
        "proof_size": clauses * proof_width,
        "formula_size": formula_size,                # may be None if no cnf header found
        "herbrand_function_size": herbrand_inputs,
        "time_s": time_s,
        "communication_kb": commB / 1024.0,
    }


def main() -> None:
    args = args_parse()
    root = Path(args.root)
    out = Path(args.out)
    out.mkdir(parents=True, exist_ok=True)

    if not root.exists():
        raise SystemExit(f"[ERR] Root not found: {root.resolve()}")

    rows = []
    for fp in root.rglob("*_prover.result"):
        rec = parse_result(fp)
        if rec:
            rows.append(rec)

    df = pd.DataFrame(rows).sort_values(["instance", "file"]).reset_index(drop=True)
    csv_path = out / "herbrand_metrics.csv"
    df.to_csv(csv_path, index=False)
    print(f"[OK ] parsed rows: {len(df)} -> {csv_path}")

    # 10 charts (log–log), one per figure
    # Communication vs {Clauses, Proof width, Proof size, Formula size, Herbrand size}
    plot_scatter_log(df, "clauses", "communication_kb",
                     "Clauses", "Communication (KB)",
                     "Communication vs Clauses", out / "comm_vs_clauses.png")
    plot_scatter_log(df, "proof_width", "communication_kb",
                     "Proof width", "Communication (KB)",
                     "Communication vs Proof width", out / "comm_vs_width.png")
    plot_scatter_log(df, "proof_size", "communication_kb",
                     "Proof size", "Communication (KB)",
                     "Communication vs Proof size", out / "comm_vs_size.png")
    if "formula_size" in df and df["formula_size"].notna().any():
        plot_scatter_log(df.dropna(subset=["formula_size"]), "formula_size", "communication_kb",
                         "Formula size", "Communication (KB)",
                         "Communication vs Formula size", out / "comm_vs_formula.png")
    plot_scatter_log(df, "herbrand_function_size", "communication_kb",
                     "Herbrand function size", "Communication (KB)",
                     "Communication vs Herbrand size", out / "comm_vs_herbrand.png")

    # Running Time vs {Clauses, Proof width, Proof size, Formula size, Herbrand size}
    plot_scatter_log(df, "clauses", "time_s",
                     "Clauses", "Running Time (s)",
                     "Running Time vs Clauses", out / "time_vs_clauses.png")
    plot_scatter_log(df, "proof_width", "time_s",
                     "Proof width", "Running Time (s)",
                     "Running Time vs Proof width", out / "time_vs_width.png")
    plot_scatter_log(df, "proof_size", "time_s",
                     "Proof size", "Running Time (s)",
                     "Running Time vs Proof size", out / "time_vs_size.png")
    if "formula_size" in df and df["formula_size"].notna().any():
        plot_scatter_log(df.dropna(subset=["formula_size"]), "formula_size", "time_s",
                         "Formula size", "Running Time (s)",
                         "Running Time vs Formula size", out / "time_vs_formula.png")
    plot_scatter_log(df, "herbrand_function_size", "time_s",
                     "Herbrand function size", "Running Time (s)",
                     "Running Time vs Herbrand size", out / "time_vs_herbrand.png")

    print(f"[OK ] figures saved under: {out.resolve()}")


if __name__ == "__main__":
    main()
