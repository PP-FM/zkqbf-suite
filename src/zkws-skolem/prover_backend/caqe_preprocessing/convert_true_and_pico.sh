#!/bin/bash
# This script runs caqe on each instance in each subdirectory in <input_dir>.
# In each subdirectory, it finds the one file (with a .qdimacs extension),
# runs <caqe_dir>/./caqe on it
# with a 120-second timeout. The output is saved to the corresponding files.

if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <input_dir> <pico_dir> <zkqbf_dir>"
    echo " (1) <input_dir> should be the path to the directory containing the subdirectories."
    echo "     each subdirectory should contain exactly one file with a .qdimacs extension."
    echo " (2) <pico_dir> should be the path to the picosat directory."
    echo " (3) <zkqbf_dir> should be the path to the zkqbf directory."
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
  qmafile="${base%_min.aag}.qma"
  certfile="${base%_min.aag}.cert"
  cnffile="${base%_min.aag}.cnf"
  picoprffile="${base%_min.aag}.picoprf"
  mergedprffile="${base%_min.aag}.mergedprf"
  prffile="${base%_min.aag}.prf"
  zkskolemfile="${base%_min.aag}.zkskolem"
  verifierinput="${base%_min.aag}_verifier.qdimacs"

  # Full path for the proof file
  renamed_qdimacs="$dir$renamedqdimacsfile"
  qma_output="$dir$qmafile"
  cert_output="$dir$certfile"
  cnf_output="$dir$cnffile"
  picoprf_output="$dir$picoprffile"
  mergedprf_output="$dir$mergedprffile"
  prf_output="$dir$prffile"
  zkskolem_output="$dir$zkskolemfile"
  verifier_input="$dir$verifierinput"

  # Run ../depqbf with a 30-second timeout and save the output to the proof file
  timeout 200 python3 caqe_true_preprocess.py "$file" "$renamed_qdimacs" > "$cert_output"
  max_var=$(python3 "$3/prover_backend/get_cert_maxvar.py" "$cert_output")
  timeout 200 python3 "$3/prover_backend/qdimacsmatrix_to_aig_andlines.py" "$renamed_qdimacs" "$max_var" > "$qma_output"
  timeout 200 python3 "$3/prover_backend/combine_and_convert_aig_to_cnf.py" "$cert_output" "$qma_output" > "$cnf_output"
  timeout 200 python3 "$3/verifier_backend/combine_and_convert_aig_to_cnf.py" --zkskoval-input "$qma_output" > "$verifier_input"
  timeout 360 $2/./picosat -T "$picoprf_output" "$cnf_output" || true
  timeout 360 python3 $3/prover_backend/merge_cnf_and_picoprf.py "$cnf_output" "$picoprf_output" > "$mergedprf_output"
  rm -f "$picoprf_output"
  timeout 1000 python3 $3/prover_backend/ExtendProof.py "$mergedprf_output" > "$prf_output"
  rm -f "$mergedprf_output"
  timeout 1000 python3 $3/prover_backend/unfold_proof.py "$prf_output"
  rm -f "$prf_output"
  timeout 1000 python3 $3/prover_backend/skolemaig_for_zk.py "$renamed_qdimacs" "$cert_output" > "$zkskolem_output"
done
