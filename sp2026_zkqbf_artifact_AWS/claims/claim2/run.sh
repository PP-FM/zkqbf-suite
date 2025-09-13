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
cd "$ZKQBF_DIR/src/zkqcube/"
sudo ./run_everything.sh "$ORIG_DIR/benchmark" . "$BIN" "$BIN" 8000 127.0.0.1
cp "$ORIG_DIR/benchmark/False"/*/*_renamed_prover_zkqrp.result "$ORIG_DIR/result.txt"
echo "Experiment completed. Results are in "$ORIG_DIR"/result" 
# --------------------------------