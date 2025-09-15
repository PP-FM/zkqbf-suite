#!/usr/bin/env bash
set -euo pipefail

usage() {
    echo "Usage: $0 <path-to-zkqbf-suite>"
    echo
    echo "Arguments:"
    echo "  <path-to-zkqbf-suite>   Directory containing zkqbf-suite"
    echo "  -h, --help              Show this help message and exit"
}

# Ensure exactly one argument is passed
if [ "$#" -ne 1 ]; then
    usage
    exit 1
fi

# Handle help flag
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    usage
    exit 0
fi

ZKQBF_DIR="$1"

# Check that the directory exists
if [ ! -d "$ZKQBF_DIR" ]; then
    echo "Error: Directory '$ZKQBF_DIR' does not exist."
    exit 1
fi

echo "Using zkqbf-suite directory: $ZKQBF_DIR"

# Navigate to zkqcube and run experiment
ORIG_DIR=$(pwd)
BASE=~/libs
BIN=$BASE/bin
source "$ZKQBF_DIR/.venv/bin/activate"
cd "$ZKQBF_DIR/src/zkws-skolem/prover_backend/caqe_preprocessing"
sudo ./run_everything.sh "$ORIG_DIR/benchmark" ../.. "$BIN" "$BIN" "$BIN" "$BIN" 8000 127.0.0.1
cp "$ORIG_DIR/benchmark/True"/*/*_renamed_prover.result "$ORIG_DIR/result.txt"
echo "Experiment completed. Results are in "$ORIG_DIR"/result" 
mkdir -p "$ORIG_DIR/intermediate_files"
cp -R "$ORIG_DIR/benchmark/True"/* "$ORIG_DIR/intermediate_files"
sudo mv "$ORIG_DIR/benchmark/True/"* "$ORIG_DIR/benchmark"
sudo rm -rf "$ORIG_DIR/benchmark/False"
sudo rm -rf "$ORIG_DIR/benchmark/True"
rm -f "$ORIG_DIR/benchmark/"*/*_renamed*
echo "Intermediate files are in "$ORIG_DIR"/intermediate_files"
# --------------------------------