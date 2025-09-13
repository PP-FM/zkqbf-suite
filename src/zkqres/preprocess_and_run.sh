#!/bin/bash
# This script processes each subdirectory.
# In each subdirectory, it finds the one file (with a .qdimacs extension),
# converts its name to have a .prf extension, and runs ../depqbf on it
# with a 30-second timeout. The output is saved to the corresponding .prf file.

if [ $# -ne 6 ]; then
  echo "Usage: $0 <path_to_benchmarks_dir> <zkqbf_dir> <path_to_depqbf> <path_to_qrpcheck> <port_number> <IP_address>"
  echo " - Note: We run ./depqbf with --trace --dep-man=simple. This is the way qbfcert calls depqbf (v1)."
  echo "         If you are using a later version of depqbf, please consult depqbf's documentation"
  echo "         for the correct command line options, and change this script accordingly."
  echo " - <port_number> and <IP_address> are for zkQRPcheck."
  exit 1
fi

# Create the False directory if it doesn't exist
mkdir -p $1/False
for dir in $1/*/; do
  if [ "$dir" = "$1//False" ]; then
    continue  # Skip the False directory
  fi

  # Find the single file in the subdirectory
  file=$(find "$dir" -maxdepth 1 -type f -name "*_renamed.qdimacs"| head -n 1)
  
  # If no file was found, skip to the next directory
  if [ -z "$file" ]; then
    echo "No renamed file found in directory $dir" >&2
    echo "Looking for *.qdimacs instead" >&2
    file=$(find "$dir" -maxdepth 1 -type f -name "*.qdimacs"| head -n 1)
    if [ -z "$file" ]; then
      echo "No .qdimacs file found in directory $dir" >&2
      continue 
    else
      # Extract the base filename (e.g., CR.qdimacs)
      base=$(basename "$file")

      # Replace the .qdimacs extension with the appropriate extensions
      inputfile="${base%.qdimacs}.qdimacs"
      renamedfile="${base%.qdimacs}_renamed.qdimacs"
      prooffile="${base%.qdimacs}_renamed.qrp"
      trimmedfile="${base%.qdimacs}_renamed.trimmed"
      zkqrpfile="${base%.qdimacs}_renamed.zkqrp"
      proverfile="${base%.qdimacs}_renamed_prover_zkqrp.result"
      verifierfile="${base%.qdimacs}_renamed_verifier_zkqrp.result"

      # Full path for intermediate files
      input="$dir$inputfile"
      renamed="$dir$renamedfile"
      output="$dir$prooffile"
      trimmed="$dir$trimmedfile"
      zkqrp="$dir$zkqrpfile"
      prover="$dir$proverfile"
      verifier="$dir$verifierfile"

      # Run ../depqbf with a 10-minute timeout
      echo "---------------------------------------------------------------------------------------------"
      echo "Renaming $input to $renamed"
      echo "---------------------------------------------------------------------------------------------"
      timeout 60 python3 $2/prover_backend/rewrite_qdimacs.py "$input" > "$renamed"
      echo "---------------------------------------------------------------------------------------------"
      echo "Running depqbf on $renamed"
      echo "---------------------------------------------------------------------------------------------"
      timeout 600 $3/depqbf --trace --dep-man=simple "$renamed" > "$output"
      exit_status=$?
      if [ $exit_status -eq 124 ] || [ ! -s "$output" ]; then
        echo "depqbf timed out or produced no output for $renamed; skipping to next input."
        rm -f "$output"  # Remove the empty output file
        continue 
      fi
      echo "---------------------------------------------------------------------------------------------"
      echo "Running qrpcheck on $output"
      echo "---------------------------------------------------------------------------------------------"
      timeout 60 python3 $2/prover_backend/get_depqbf_result.py "$output"
      exit_code=$?
      if [ $exit_code -ne 0 ]; then
        echo "depqbf failed to produce a valid UNSAT proof for $renamed; skipping to next input."
        rm -f "$output"  # Remove the empty output file
        continue 
      fi
      timeout 600 $4/qrpcheck -p qrp $output > $trimmed
      rm -f $output
      echo "---------------------------------------------------------------------------------------------"
      echo "Running ExtendQRPProof on $trimmed"
      echo "---------------------------------------------------------------------------------------------"
      timeout 600 python3 $2/prover_backend/ExtendQRPProof.py $trimmed > $zkqrp
      #rm -f $trimmed
      echo "---------------------------------------------------------------------------------------------"
      echo "Running zkQRPcheck on $zkqrp as prover in the background"
      echo "---------------------------------------------------------------------------------------------"
      $2/./test 1 $5 $6 $zkqrp > $prover &
      echo "---------------------------------------------------------------------------------------------"
      echo "Running zkQRPcheck on $zkqrp as verifier"
      echo "---------------------------------------------------------------------------------------------"
      $2/./test 2 $5 $6 > $verifier
      echo "---------------------------------------------------------------------------------------------"
      echo "Done! Moving $dir to $1/False"
      echo "---------------------------------------------------------------------------------------------"
      mv $dir $1/False
    fi
    continue
  fi

  # Extract the base filename (e.g., CR.qdimacs)
  base=$(basename "$file")

  # Replace the .qdimacs extension with the appropriate extensions
  renamedfile="${base%.qdimacs}.qdimacs"
  prooffile="${base%.qdimacs}.qrp"
  trimmedfile="${base%.qdimacs}.trimmed"
  zkqrpfile="${base%.qdimacs}.zkqrp"
  proverfile="${base%.qdimacs}_prover_zkqrp.result"
  verifierfile="${base%.qdimacs}_verifier_zkqrp.result"

  # Full path for intermediate files
  renamed="$dir$renamedfile"
  output="$dir$prooffile"
  trimmed="$dir$trimmedfile"
  zkqrp="$dir$zkqrpfile"
  prover="$dir$proverfile"
  verifier="$dir$verifierfile"

  # Run ../depqbf with a 10-minute timeout
  echo "---------------------------------------------------------------------------------------------"
  echo "Running depqbf on $renamed"
  echo "---------------------------------------------------------------------------------------------"
  timeout 600 $3/depqbf --trace --dep-man=simple "$renamed" > "$output"
  exit_status=$?
  if [ $exit_status -eq 124 ] || [ ! -s "$output" ]; then
    echo "depqbf timed out or produced no output for $renamed; skipping to next input."
    rm -f "$output"  # Remove the empty output file
    continue 
  fi
  echo "---------------------------------------------------------------------------------------------"
  echo "Running qrpcheck on $output"
  echo "---------------------------------------------------------------------------------------------"
  timeout 60 python3 $2/prover_backend/get_depqbf_result.py "$output"
  exit_code=$?
  if [ $exit_code -ne 0 ]; then
    echo "depqbf failed to produce a valid UNSAT proof for $renamed; skipping to next input."
    rm -f "$output"  # Remove the empty output file
    continue 
  fi
  timeout 600 $4/qrpcheck -p qrp $output > $trimmed
  rm -f $output
  echo "---------------------------------------------------------------------------------------------"
  echo "Running ExtendQRPProof on $trimmed"
  echo "---------------------------------------------------------------------------------------------"
  timeout 600 python3 $2/prover_backend/ExtendQRPProof.py $trimmed > $zkqrp
  #rm -f $trimmed
  echo "---------------------------------------------------------------------------------------------"
  echo "Running zkQRPcheck on $zkqrp as prover in the background"
  echo "---------------------------------------------------------------------------------------------"
  $2/./test 1 $5 $6 $zkqrp > $prover &
  echo "---------------------------------------------------------------------------------------------"
  echo "Running zkQRPcheck on $zkqrp as verifier"
  echo "---------------------------------------------------------------------------------------------"
  $2/./test 2 $5 $6 > $verifier
  echo "---------------------------------------------------------------------------------------------"
  echo "Done! Moving $dir to $1/False"
  echo "---------------------------------------------------------------------------------------------"
  mv $dir $1/False
done
