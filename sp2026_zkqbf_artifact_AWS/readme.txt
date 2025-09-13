Artifact Appendix: Towards Practical Zero-Knowledge Proof for PSPACE

This repository contains the code and data artifact for the paper 
"Towards Practical Zero-Knowledge Proof for PSPACE."

Our protocols implement two distinct approaches for proving knowledge 
about quantified Boolean formulas (QBFs) in zero-knowledge:
- **ZKQRES**: a protocol based on validating quantified-resolution (Q-Res) proofs.
- **ZKWS**: a protocol based on validating winning strategies (Skolem/Herbrand functions).

Both approaches enable efficient zero-knowledge proofs for PSPACE-complete 
statements once reduced to QBF evaluation.

----------------------------------------------------------------------
A.1 Description & Requirements

- The artifact is implemented using the EMP-toolkit as the backend 
  for interactive zero-knowledge proofs, and integrates the open-source 
  ZKUNSAT implementation.
- QBF solvers employed: DepQBF and QRPcheck (for Q-Res proofs), 
  and CAQE (for Skolem/Herbrand strategies).
- Certificates (proofs or strategies) are preprocessed into 
  `.prf.unfold` files which serve as the direct input to our protocols.
- Benchmarks are taken from the QBFEVAL’07 and QBFEVAL’23 suites, 
  as well as real-world instances from:
  * Partial Equivalence Checking (PEC)
  * Conformant Planning (C-PLAN)
  * Black-Box Checking (BBC)

Hardware requirements:
- The evaluation was originally run on AWS i4i.16xlarge instances 
  (64 vCPUs, 512 GB RAM, 50 Gbps inter-instance bandwidth). 

- High-memory machines are recommended for large benchmarks.

----------------------------------------------------------------------
A.2 Instructions

**1. Using Preprocessed Proofs**
- The folder `preprocessed_proof/` contains `.prf.unfold` files 
  for selected QBF instances.
- For *True instances*: switch to branches `zkherbrand-validation` or `zkqcube`.
- For *False instances*: switch to branches `zksklom-validation` or `zkqrp`.
- Install the required tools to specified directory via running install.sh in advance
- Running the protocol via run.sh, which requires only the selected `.prf.unfold` file.

**2. Using Raw QBF Instances**
- Install additional solvers and preprocessors:
- The installation script for additional tools is install_raw.sh
- Running the protocol via run_raw.sh, which requires only the selected .qdimacs file.


This generates properly formatted certificates compatible with our protocols.

**3. AWS VM Access**
- A preconfigured AWS virtual machine is available for evaluation.
- To request SSH access, please send an email to hengyu2@illinois.edu 
with subject line beginning with: `[SP 2026 AEC]`.

----------------------------------------------------------------------
A.3 Benchmarks & Reproduction of Results

- **QBFEVAL**: Out of 351 instances, ZKQRES verified 341 within ~1 hour; 
~75% were verified within 100 seconds. ZKWS verified 298 out of 322 
Herbrand instances within half an hour.
- **Real-world applications**: PEC, C-PLAN, and BBC instances were encoded 
as QBFs and successfully verified. Verification times ranged from ~200s 
(BBC) to ~1200s (C-PLAN).
- Optimizations: The implementation uses batching and hierarchical 
clause encoding to reduce runtime by ~50%.

This artifact enables reproduction of all the experiments described in 
Section 6 of the paper, and we separated them into 2 folders, one contains the smaller instances can be fully reproduced and some larger instances will take more that 3 hours to run and might run out of the memory.

Also, we are providing the plotting function to reproduce our evaluation diagram in section 6.