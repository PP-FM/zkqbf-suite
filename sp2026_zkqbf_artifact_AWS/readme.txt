Artifact Appendix: Towards Practical Zero-Knowledge Proof for PSPACE

This repository contains the code and data artifact for the paper 
"Towards Practical Zero-Knowledge Proof for PSPACE."

Our protocols implement two distinct approaches for proving knowledge 
about quantified Boolean formulas (QBFs) in zero-knowledge:

- **ZKQRES**: a protocol based on validating quantified-resolution 
  (Q-Res/Q-Cube-Res) proofs.

- **ZKWS**  : a protocol based on validating winning strategies 
  (Skolem/Herbrand functions).

Both approaches enable efficient zero-knowledge proofs for PSPACE-complete 
statements through the privacy preserving verification of their certificates.

----------------------------------------------------------------------
A.1 Description & Requirements

- The artifact is implemented using the EMP-toolkit as the backend 
  for interactive zero-knowledge proofs, and integrates the open-source 
  ZKUNSAT implementation.

- QBF solvers employed: DepQBF with QRPcheck (for Q-Res/Q-Cube-Res proofs), 
  and CAQE-2 (for Skolem/Herbrand functions).

- Other tools employed: ABC, Aiger and Picosat. Our protocol also heavily
  relies on the NTL library.

- Benchmarks presented in this package are taken from the QBFEVAL'07 suite.
  This is primarily because our experimental evaluation was majorly 
  performed on this set of instances. (Results for real-world applications 
  like Partial Equivalence Checking (PEC), Conformant Planning (C-PLAN),
  and Black-Box Checking (BBC) can be found in the paper)

Hardware requirements:
- The evaluation of the protocol was originally run on AWS i4i.16xlarge 
  instances (64 vCPUs, 512 GB RAM, 50 Gbps inter-instance bandwidth). 

- High-memory (> 64 GiB) machines are recommended to invoke these protocols. 

----------------------------------------------------------------------
A.2 Instructions

The following are the instructions for the AE, or any future user to run
this package on an AWS instance. You will see results similar to the ones
presented in the paper if you have access to i4i.16xlarge instances 
(64 vCPUs, 512 GB RAM, 50 Gbps inter-instance bandwidth).

Expected Time: 3 hours 30 mins. Please find the specific breakdown below:

**1. Please find log into the AWS instance that we have provided in the 
     infrastructure directory of this package.

     You can do this by running:

     ssh -i <private-key-path> ubunut@<IP-address>

     from the terminal. If you have any issues with accessing the instance,
     please reach out to us.

     **Time: <5 minutes**

**2. To run use this package, we recommend uploading this folder (including
     "install.sh" and "claims") onto the AWS instance. This can be done 
     with the following command on your local machine:

     rsync -chvazP -e "ssh -i <private-key-path>" <path-to-this-package> ubuntu@<IP-address>:~/.

     **Time: <1 minute** 

**3. You should now be able to see the package on the AWS instance. To setup 
     the instance to run our protocols, first run:

     cd <package>
     ./install.sh

     This should install all the dependencies, clone our repo, compile and 
     prepare the artifact for use on the AWS instance.

     Note: It is normal to see a lot of warnings while running this step.

     **Time: 15 minutes** (install.sh will run for ~15 minutes)

**4. Verifying our claims:

     We have provided 8 claims, the first four claims take as input a 
     QBF formula in QDIMACS format, prepare all necessary intermediate 
     files (QRP proofs/Herbrand functions/Skolem functions/picosat proofs/
     intermediate files to run protocol)

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