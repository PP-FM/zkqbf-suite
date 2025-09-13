#!/bin/bash
# This script runs caqe on each instance in each subdirectory in <input_dir>.
# In each subdirectory, it finds the one file (with a .qdimacs extension),
# runs <caqe_dir>/./caqe on it
# with a 120-second timeout. The output is saved to the corresponding files.

if [ "$#" -ne 8 ]; then
    echo "Usage: $0 <input_dir> <zkqbf_dir> <caqe_dir> <abc_dir> <aiger_dir> <picosat_dir> <port_num> <ip_address>"
    echo " (1) <input_dir> should be the path to the directory containing the subdirectories."
    echo "     each subdirectory should contain exactly one file with a .qdimacs extension."
    echo " (2) <zkqbf_dir> should be the path to the zkqbf directory."
    echo " (3) <caqe_dir> should be the path to the caqe directory."
    echo " (4) <abc_dir> should be the path to the abc directory."
    echo " (5) <aiger_dir> should be the path to the aiger (toolset: https://github.com/arminbiere/aiger) directory."
    echo " (6) <picosat_dir> should be the path to the picosat directory."
    echo " (7) <port_num> should be the port number for the protocol."
    echo " (8) <ip_address> should be the IP address for the protocol."
    exit 1
fi

echo "---------------------------------------------------------------------------------------------"
echo "Moving benchmarks to their own folders..."
./move_benchmarks_to_own_folders.sh "$1"
echo "---------------------------------------------------------------------------------------------"
echo "Running caqe on each instance, getting the certificates and minimizing them using abc..."
echo "---------------------------------------------------------------------------------------------"
./run_caqe_to_get_certs.sh "$1" "$2" "$3" "$4" "$5"
echo "---------------------------------------------------------------------------------------------"
echo "Sorting the benchmarks into true and false..."
echo "---------------------------------------------------------------------------------------------"
./sort_benches_into_true_and_false.sh "$1"
echo "---------------------------------------------------------------------------------------------"
echo "Getting the skolemization for the instances in the True directory, getting picosat proofs,"
echo " and merging them with the skolemization..."
echo "---------------------------------------------------------------------------------------------"
./convert_true_and_pico.sh "$1/True" $6 $2
echo "---------------------------------------------------------------------------------------------"
echo "All benchmark related files for the True instances shoulld be in the True directory."
echo "---------------------------------------------------------------------------------------------"
echo "Running zkskolem-validation as prover on the True instances..."
echo "---------------------------------------------------------------------------------------------"
./run_zkskolem_as_prover.sh "$1/True" $2 $7 $8 &
echo "---------------------------------------------------------------------------------------------"
echo "Running zkskolem-validation as verifier on the True instances..."
echo "---------------------------------------------------------------------------------------------"
./run_zkskolem_as_verifier.sh "$1/True" $2 $7 $8
