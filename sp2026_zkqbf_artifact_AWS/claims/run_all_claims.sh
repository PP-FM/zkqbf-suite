#!/bin/bash
# =========================================================================
# run-claims.sh
# Run all claim*/run.sh scripts with the given zkqbf-suite directory
# =========================================================================

usage() {
    cat <<EOF
Usage: $(basename "$0") <zkqbf-suite-dir>

This script loops through all directories matching "claim*/"
and executes each directory's ./run.sh with the provided zkqbf-suite directory.

Options:
  -h, --help    Show this help message and exit

Example:
  $(basename "$0") /path/to/zkqbf-suite
EOF
    exit 0
}

# ---- Argument parsing ----
if [ $# -ne 1 ]; then
    echo "Error: Expected exactly one argument."
    usage
fi

if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    usage
fi

ZKQBF_DIR="$(realpath "$1")"

# ---- Run loop ----
for dir in claim*/; do
    if [ -d "$dir" ] && [ -x "$dir/run.sh" ]; then
        echo "[INFO] Entering $dir"
        (
            cd "$dir" || exit 1
            echo "[INFO] Running ./run.sh with $ZKQBF_DIR"
            ./run.sh "$ZKQBF_DIR"
        )
    else
        echo "[WARN] Skipping $dir (no run.sh or not executable)"
    fi
done

echo "[DONE] Finished running all claim*/run.sh scripts."
echo "Results and intermediate files are in their respective claim{1..4}/ directories. These claims are for single instances."
echo "Plots are in claim{5..8}/plots. These claims are for multiple instances to highlight the trends."