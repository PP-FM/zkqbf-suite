
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
  echo "Usage: $0 <zkqbf-suite_dir>" >&2
  exit 1
fi

INPUT_DIR=./benchmark/benchmarks_skolem/True
ZKQBF_DIR="$1/src/zkws-skolem"
PORT=8000
IP=127.0.0.1
TEST_BIN="$ZKQBF_DIR/test"
[[ -d "$INPUT_DIR" ]] || { echo "Input dir not found: $INPUT_DIR. Unzipping" >&2; tar -xf "./benchmark/benchmarks_skolem.tar.xz" -C "./benchmark/"; }
[[ -x "$TEST_BIN"  ]] || { echo "Executable not found: $TEST_BIN" >&2; exit 1; }

shopt -s nullglob
for dir in "$INPUT_DIR"/*/; do
  aag="$(find "$dir" -maxdepth 1 -type f -name '*_renamed_min.aag' | head -n 1 || true)"
  if [[ -z "$aag" ]]; then
    echo "[SKIP] No *_renamed_min.aag in $(basename "$dir")"
    continue
  fi

  base="$(basename "$aag")"                 # e.g., FOO_renamed_min.aag
  stem="${base%_min.aag}"                   # e.g., FOO_renamed
  renamed_qdimacs="${stem}.qdimacs"         # e.g., FOO_renamed.qdimacs
  prf="${stem}.prf"                         # e.g., FOO_renamed.prf
  zkskolem="${stem}.zkskolem"                   # e.g., FOO_renamed.zkskolem
  verifier_qdimacs="${stem}_verifier.qdimacs"

  # Full paths
  renamed_qdimacs_path="$dir$renamed_qdimacs"
  prf_path="$dir$prf"
  zkskolem_path="$dir$zkskolem"
  verifier_qdimacs_path="$dir$verifier_qdimacs"

  prover_out="${renamed_qdimacs_path%.qdimacs}_prover.result"
  verifier_out="${verifier_qdimacs_path%.qdimacs}_verifier.result"

  # If both results exist and are non-empty, refresh (consistent with the reference style)
  if [[ -s "$prover_out" && -s "$verifier_out" ]]; then
    continue
    # echo "[SKIP] Results exist for $(basename "$dir"); refreshing..."
    rm -f -- "$prover_out" "$verifier_out"
  fi

  echo "--------------------------------------------------------------------------------"
  echo "[DIR ] $(basename "$dir")"
  # echo "[FILE] $base"
  # echo "[DERI] renamed_qdimacs=$renamed_qdimacs  prf=$prf  zkskolem=$zkskolem  verifier=$verifier_qdimacs"

  mkdir -p data
  # echo "[CMD ] prover: $TEST_BIN 1 $PORT $IP \"$verifier_qdimacs_path\" \"$zkskolem_path\" \"${prf_path}.unfold\""
  "$TEST_BIN" 1 "$PORT" "$IP" "$verifier_qdimacs_path" "$zkskolem_path" "${prf_path}.unfold" >"$prover_out" 2>&1 &

  # echo "[CMD ] verifier: $TEST_BIN 2 $PORT $IP \"$verifier_qdimacs_path\""
  "$TEST_BIN" 2 "$PORT" "$IP" "$verifier_qdimacs_path" >"$verifier_out" 2>&1

  echo "[DONE] $(basename "$dir")"
done
shopt -u nullglob

echo "All done."

mkdir -p plots

$1/.venv/bin/python3 $ZKQBF_DIR/plot_skolem.py --root $INPUT_DIR --out ./plots
