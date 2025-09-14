#!/usr/bin/env bash
# Runs zkQRP protocol (prover + verifier) for each instance under BENCH_DIR.
# Assumes each subdir has "*_renamed.zkqrp" (falls back to any "*.zkqrp").
# Usage: ./run_protocol_only.sh <zkqres_dir> [BENCH_DIR]
#   <zkqres_dir> : directory containing the 'test' binary
#   [BENCH_DIR]  : default ./benchmark/False

set -euo pipefail

ZKQRES_DIR="${1:?Usage: $0 <zkqres_dir> [BENCH_DIR]}"
BENCH_DIR="${2:-./benchmark/benchmarks_qcube/True}"
TEST_BIN="$ZKQRES_DIR/test"
PORT=8000
IP=127.0.0.1

[[ -x "$TEST_BIN" ]] || { echo "Executable not found: $TEST_BIN" >&2; exit 1; }
[[ -d "$BENCH_DIR" ]] || { echo "Benchmark dir not found: $BENCH_DIR" >&2; exit 1; }

shopt -s nullglob
for dir in "$BENCH_DIR"/*/; do
  zkqrp="$(find "$dir" -maxdepth 1 -type f -name '*_renamed.zkqrp' | head -n 1 || true)"
  renamed="$(find "$dir" -maxdepth 1 -type f -name '*_renamed.qdimacs' | head -n 1 || true)"
  [[ -n "$zkqrp" ]] || zkqrp="$(find "$dir" -maxdepth 1 -type f -name '*.zkqrp' | head -n 1 || true)"
  if [[ -z "$zkqrp" ]]; then
    echo "[SKIP] No .zkqrp in $dir"
    continue
  fi

  base="$(basename "$zkqrp")"
  stem="${base%.zkqrp}"
  prover_out="$dir/${stem}_prover_zkqrp.result"
  verifier_out="$dir/${stem}_verifier_zkqrp.result"

  if [[ -s "$prover_out" && -s "$verifier_out" ]]; then
    echo "[SKIP] Results exist for $(basename "$dir")"
    sudo rm -f "$prover_out" "$verifier_out"
  fi

  echo "--------------------------------------------------------------------------------"
  echo "[DIR ] $(basename "$dir")"
  echo "[FILE] $base"
  echo "[CMD ] prover: $TEST_BIN 1 $PORT $IP $renamed $zkqrp"
  "$TEST_BIN" 1 "$PORT" "$IP" "$renamed" "$zkqrp" >"$prover_out" 2>&1 &
  echo "[CMD ] verifier: $TEST_BIN 2 $PORT $IP"
  "$TEST_BIN" 2 "$PORT" "$IP" $renamed >"$verifier_out" 2>&1; 

  echo "[DONE] $(basename "$dir")"
done
shopt -u nullglob

echo "All done."

mkdir -p plots

python $1/plot_qcube.py --root /home/ubuntu/zkqbf-suite/sp2026_zkqbf_artifact_AWS/claims/claim6/benchmark/benchmarks_qcube/True --out /home/ubuntu/zkqbf-suite/sp2026_zkqbf_artifact_AWS/claims/claim6/plots
