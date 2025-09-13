#!/bin/bash
# This script runs caqe on each instance in each subdirectory in <input_dir>.
# In each subdirectory, it finds the one file (with a .qdimacs extension),
# runs <caqe_dir>/./caqe on it
# with a 120-second timeout. The output is saved to the corresponding files.

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <input_dir>"
    echo " (1) <input_dir> should be the path to the directory containing the subdirectories."
    echo "     each subdirectory should contain exactly one file with a .qdimacs extension."
    echo " Note that the True and False sudirectories will be created in the <input_dir>."
    exit 1
fi

# Loop over each subdirectory in crafted-qbfeval20
for dir in $1/*/; do
  # Find the single file in the subdirectory
  file=$(find "$dir" -maxdepth 1 -type f -name "*_renamed.aag"| head -n 1)
  
  # If no file was found, skip to the next directory
  if [ -z "$file" ]; then
    echo "Warning: No file found in directory $dir" >&2
    continue
  fi

  python3 "caqe_result.py" "$file"
  exit_code=$?

  mkdir -p "$1/True"
  mkdir -p "$1/False"

  if [ $exit_code -eq 1 ]; then
    echo "True" "$file"
    mv $dir $1/True
  elif [ $exit_code -eq 0 ]; then
    echo "False" "$file"
    mv $dir $1/False
  elif [ $exit_code -eq 2 ]; then
    rm -rf "$dir"
  else
    echo "Error: Unexpected exit code $exit_code"
  fi
  
done
