import pandas as pd
import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np

mpl.style.use("default")
df_herbrand_perf = pd.read_csv("prover_herbrand_full.csv")[["name", "t", "cnf_size", "herbrand_input", "clauses", "degree"]]
df_herbrand_perf.rename(columns={"t": "t_herbrand"}, inplace=True)

df_herbrand = pd.read_csv("prover_herbrand_full.csv")
df_herbrand.rename(columns={"t": "t_herbrand"}, inplace=True)

df_qrp = pd.read_csv("qrp_stats.csv")[["name", "t"]]
df_qrp.rename(columns={"t": "t_qrp"}, inplace=True)
df = pd.merge(df_herbrand, df_qrp, on="name", how="inner")

for col in ["t_herbrand", "t_qrp", "cnf_size", "clauses", "herbrand_input"]:
    df[col] = pd.to_numeric(df[col], errors='coerce')

df = df.dropna(subset=["t_herbrand", "t_qrp", "cnf_size", "clauses", "herbrand_input"])

def plot_comparison(x_col, x_label, filename):
    df_sorted = df.sort_values(by=x_col).reset_index(drop=True)

    fig, ax = plt.subplots(figsize=(8, 5))
    ax.scatter(df_sorted[x_col], df_sorted["t_herbrand"], color='black', s=16, marker='o', label='Herbrand function')
    ax.scatter(df_sorted[x_col], df_sorted["t_qrp"], color='red', s=30, marker='x', label='QRP method')

    ax.set_xlabel(x_label)
    ax.set_ylabel("Total Running Time (t) [s]")
    ax.set_title(f"Running Time vs {x_label}")
    
    ax.set_xscale("linear")
    ax.set_yscale("log")
    ax.grid(True, linestyle='--', linewidth=0.5)
    ax.legend(loc='upper left')

    fig.savefig(filename, bbox_inches='tight', dpi=300, format='pgf')
    plt.show()
plot_comparison("cnf_size", "CNF Variable Count", "t_vs_cnf_variable_comparison.pgf")
plot_comparison("clauses", "Number of Clauses", "t_vs_clauses_comparison.pgf")
plot_comparison("herbrand_input", "Herbrand Input Size", "t_vs_herbrand_input_comparison.pgf")
plot_comparison("name", "Index of Instances", "t_vs_index_comparison.pgf")
