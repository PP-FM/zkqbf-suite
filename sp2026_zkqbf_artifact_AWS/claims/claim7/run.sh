
set -euo pipefail

if [[ "$#" -ne 1 ]]; then
  echo "Usage: $0 <zkqbf-suite_dir>" >&2
  exit 1
fi

INPUT_DIR=./benchmark/benchmarks_herbrand_for_every5s/False
ZKQBF_DIR="$1/src/zkws-herbrand"
PORT=8000
IP=127.0.0.1
TEST_BIN="$ZKQBF_DIR/test"
TIMEOUT=120  # seconds (kept as a hint; not enforced unless you wrap with 'timeout')

[[ -d "$INPUT_DIR" ]] || { echo "Input dir not found: $INPUT_DIR" >&2; exit 1; }
[[ -x "$TEST_BIN"  ]] || { echo "Executable not found: $TEST_BIN" >&2; exit 1; }

shopt -s nullglob
for dir in "$INPUT_DIR"/*/; do
  aag="$(find "$dir" -maxdepth 1 -type f -name '*_renamed_min.aag' | head -n 1 || true)"
  if [[ -z "$aag" ]]; then
    echo "[SKIP] No *_renamed_min.aag in $(basename "$dir")"
  fi

  base="$(basename "$aag")"                 # e.g., FOO_renamed_min.aag
  stem="${base%_min.aag}"                   # e.g., FOO_renamed
  renamed_qdimacs="${stem}.qdimacs"         # e.g., FOO_renamed.qdimacs
  prf="${stem}.prf"                         # e.g., FOO_renamed.prf
  zkherb="${stem}.zkherb"                   # e.g., FOO_renamed.zkherb
  verifier_qdimacs="${stem}_verifier.qdimacs"

  # Full paths
  renamed_qdimacs_path="$dir$renamed_qdimacs"
  prf_path="$dir$prf"
  zkherb_path="$dir$zkherb"
  verifier_qdimacs_path="$dir$verifier_qdimacs"

  prover_out="${renamed_qdimacs_path%.qdimacs}_prover.result"
  verifier_out="${renamed_qdimacs_path%.qdimacs}_verifier.result"

  # If both results exist and are non-empty, refresh (consistent with the reference style)
  if [[ -s "$prover_out" && -s "$verifier_out" ]]; then
    echo "[SKIP] Results exist for $(basename "$dir"); refreshing..."
    rm -f -- "$prover_out" "$verifier_out"
  fi

  echo "[DIR ] $(basename "$dir")"
  # echo "[FILE] $base"
  # echo "[DERI] renamed_qdimacs=$renamed_qdimacs  prf=$prf  zkherb=$zkherb  verifier=$verifier_qdimacs"

  mkdir -p data
  # echo "[CMD ] prover: $TEST_BIN 1 $PORT $IP \"$verifier_qdimacs_path\" \"$zkherb_path\" \"${prf_path}.unfold\""
  "$TEST_BIN" 1 "$PORT" "$IP" "$verifier_qdimacs_path" "$zkherb_path" "${prf_path}.unfold" >"$prover_out" 2>&1 &

  # echo "[CMD ] verifier: $TEST_BIN 2 $PORT $IP \"$verifier_qdimacs_path\""
  "$TEST_BIN" 2 "$PORT" "$IP" "$verifier_qdimacs_path" >"$verifier_out" 2>&1

  echo "[DONE] $(basename "$dir")"
done
shopt -u nullglob

echo "All done."

mkdir -p plots

$1/.venv/bin/python3 $ZKQBF_DIR/plot_herbrand.py --root /home/ubuntu/zkqbf-suite/sp2026_zkqbf_artifact_AWS/claims/claim7/benchmark/benchmarks_herbrand_for_every5s/False --out /home/ubuntu/zkqbf-suite/sp2026_zkqbf_artifact_AWS/claims/claim7/plots
