#!/bin/bash
# This script runs caqe on each instance in each subdirectory in <input_dir>.
# In each subdirectory, it finds the one file (with a .qdimacs extension),
# runs <caqe_dir>/./caqe on it
# with a 120-second timeout. The output is saved to the corresponding files.

if [ "$#" -ne 4 ]; then
    echo "Usage: $0 <input_dir> <zkqbf_dir> <port_num> <ip_address>"
    echo " (1) <input_dir> should be the path to the directory containing the subdirectories."
    echo "     each subdirectory should contain exactly one file with a .qdimacs extension."
    echo " (2) <zkqbf_dir> should be the path to the zkqbf directory."
    echo " (3) <port_num> should be the port number for the protocol."
    echo " (4) <ip_address> should be the IP address for the protocol."
    exit 1
fi

# Loop over each subdirectory in crafted-qbfeval20
for dir in $1/*/; do
  # Find the single file in the subdirectory
  file=$(find "$dir" -maxdepth 1 -type f -name "*_renamed_min.aag"| head -n 1)
  
  # If no file was found, skip to the next directory
  if [ -z "$file" ]; then
    echo "Warning: No file found in directory $dir" >&2
    continue
  fi

  # Extract the base filename (e.g., CR.qdimacs)
  base=$(basename "$file")

  # Replace the .qdimacs extension with .prf
  renamedqdimacsfile="${base%_min.aag}.qdimacs"
  prffile="${base%_min.aag}.prf"
  zkskolemfile="${base%_min.aag}.zkskolem"
  verifierinput="${base%_min.aag}_verifier.qdimacs"

  # Full path for the proof file
  renamed_qdimacs="$dir$renamedqdimacsfile"
  prf_output="$dir$prffile"
  zkskolem_output="$dir$zkskolemfile"
  verifier_input="$dir$verifierinput"

  # Run ../depqbf with a 30-second timeout and save the output to the proof file
  $2/./test 1 $3 $4 "$verifier_input" "$zkskolem_output" "${prf_output}.unfold" > "${renamed_qdimacs%.qdimacs}_prover.result"
done
