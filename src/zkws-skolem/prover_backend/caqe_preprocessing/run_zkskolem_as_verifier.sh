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
  file=$(find "$dir" -maxdepth 1 -type f -name "*_renamed.qdimacs"| head -n 1)
  
  # If no file was found, skip to the next directory
  if [ -z "$file" ]; then
    echo "Warning: No file found in directory $dir" >&2
    continue
  fi

  # Extract the base filename (e.g., CR.qdimacs)
  base=$(basename "$file")

  # Replace the _renamed.qdimacs extension with _verifier.qdimacs
  verifierinput="${base%.qdimacs}_verifier.qdimacs"

  # Full path for the verifier input file
  verifier_input="$dir$verifierinput"

  # Run zkherbrand-validation as verifier with a 1000-second timeout and save the output to the _verifier.result file
  $2/./test 2 $3 $4 "$verifier_input" > "${file%.qdimacs}_verifier.result"
done
