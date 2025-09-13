#!/bin/bash
# This script runs caqe on each instance in each subdirectory in <input_dir>.
# In each subdirectory, it finds the one file (with a .qdimacs extension),
# runs <caqe_dir>/./caqe on it
# with a 120-second timeout. The output is saved to the corresponding files.

if [ "$#" -ne 5 ]; then
    echo "Usage: $0 <input_dir> <zkqbf_dir> <caqe_dir> <abc_dir> <aiger_dir>"
    echo " (1) <input_dir> should be the path to the directory containing the subdirectories."
    echo "     each subdirectory should contain exactly one file with a .qdimacs extension."
    echo " (2) <zkqbf_dir> should be the path to the zkqbf directory."
    echo " (3) <caqe_dir> should be the path to the caqe directory."
    echo " (4) <abc_dir> should be the path to the abc directory."
    echo " (5) <aiger_dir> should be the path to the aiger (toolset: https://github.com/arminbiere/aiger) directory."
    exit 1
fi

# Loop over each subdirectory in crafted-qbfeval20
for dir in $1/*/; do
  # Find the single file in the subdirectory
  file=$(find "$dir" -maxdepth 1 -type f | head -n 1)
  
  # If no file was found, skip to the next directory
  if [ -z "$file" ]; then
    echo "Warning: No file found in directory $dir" >&2
    continue
  fi

  # Extract the base filename (e.g., CR.qdimacs)
  base=$(basename "$file")

  # Replace the .qdimacs extension with .prf
  renamedfile="${base%.qdimacs}_renamed.qdimacs"
  aagfile="${base%.qdimacs}_renamed.aag"
  aigfile="${base%.qdimacs}_renamed.aig"
  minaigerfile="${base%.qdimacs}_renamed_min.aig"
  minaagfile="${base%.qdimacs}_renamed_min.aag"
  certfile="${base%.qdimacs}_renamed_min.cert"
  qmafile="${base%.qdimacs}_renamed.qma"
  cnffile="${base%.qdimacs}_renamed.cnf"

  # Full path for the proof file
  renamed_output="$dir$renamedfile"
  aag_output="$dir$aagfile"
  aig_output="$dir$aigfile"
  min_aiger_output="$dir$minaigerfile"
  min_aag_output="$dir$minaagfile"
  cert_output="$dir$certfile"
  qma_output="$dir$qmafile"
  cnf_output="$dir$cnffile"

  # Run ../depqbf with a 30-second timeout and save the output to the proof file
  timeout 120 python3 $2/prover_backend/rewrite_qdimacs.py "$file" > "$renamed_output"
  timeout 300 $3/./caqe -c "$renamed_output" > "$aag_output"
  exit_status=$?
  # Check if the command timed out (exit status 124) or produced no output
  if [ $exit_status -eq 124 ] || [ ! -s "$aag_output" ]; then
    echo "caqe timed out or produced no output for $file; skipping to next input."
    continue  # Skips to the next iteration in the loop
  fi
  timeout 120 $5/./aigtoaig "$aag_output" "$aig_output"
  timeout 300 $4/abc -c "read $aig_output; strash; dc2; write $min_aiger_output;"
  rm -f "$aig_output"
  timeout 120 $5/./aigtoaig "$min_aiger_output" "$min_aag_output"
  rm -f "$min_aiger_output"
done
