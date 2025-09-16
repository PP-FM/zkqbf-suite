# Artifact Appendix: *Towards Practical Zero-Knowledge Proof for PSPACE*

This repository contains the code and data artifact for the paper **“Towards Practical Zero-Knowledge Proof for PSPACE.”**

Our protocols implement two complementary approaches for proving knowledge about quantified Boolean formulas (QBFs) in zero-knowledge:

- **ZKQRES** : validates quantified resolution proofs (Q-Res / Q-Cube-Res).
- **ZKWS** : validates winning strategies (Skolem / Herbrand functions).

Both approaches enable efficient interactive zero-knowledge proofs for PSPACE-complete statements by privately verifying their corresponding certificates.

## A.1 Description & Requirements

**Backends and dependencies**

- **EMP-toolkit** as the interactive ZK backend.
- **QBF solvers**
  - **DepQBF** with **QRPcheck** (for Q-Res / Q-Cube-Res proofs)
  - **CAQE-2** (for Skolem / Herbrand functions)
- **Other tools**: **ABC**, **Aiger**, **Picosat**.
- **Libraries**: **NTL** is heavily used.

It is also worth noting that the open source **ZKUNSAT** implementation is central to our implementation (We build on top of ZKUNSAT).

**Benchmarks**

- Benchmarks are taken from **QBFEVAL’07**, which we primarily used in our evaluation section on the paper.
- Results on real-world applications (Partial Equivalence Checking (PEC), Conformant Planning (C-PLAN), Black-Box Checking(BBC)) appear in the paper.

**Hardware**

- Original evaluation ran on **AWS i4i.16xlarge**  
  (64 vCPUs, 512 GiB RAM).
- We recommend **high-memory machines (>= 64 GiB RAM)**.

## A.2 Instructions

The steps below assume you are running on an AWS instance. You should see results comparable to those presented in the paper when using an **i4i.16xlarge** instance.  
**Total expected time:** ~**3 h 30 min** (details per step/claim below).

### 1) Access the AWS instance

Find the private key and IP address in the repository’s `infrastructure/` directory, then connect:

```bash
ssh -i <private-key-path> ubuntu@<IP-address>
```

If you encounter access issues, please reach out to us.  
**Time:** < 5 minutes

### 2) Upload the package to the instance

From your local machine, upload the entire folder (including `install.sh` and `claims/`) to the instance:

```bash
rsync -chvazP -e "ssh -i <private-key-path>" <path-to-this-package> ubuntu@<IP-address>:~/.
```

**Time:** < 1 minute

### 3) Install and set up dependencies

On the AWS instance:

```bash
cd <package>
chmod +x install.sh
./install.sh
```

This installs all dependencies, clones our repository, compiles components, and prepares the artifact.  
It is normal to see many warnings during this step.  
**Time:** ~15 minutes

### 4) Verify the claims

We provide **8 claims**:

- **Claims 1-4**: take a QBF in **QDIMACS** format (PCNF), generate the required intermediates (QRP proofs, Herbrand/Skolem functions, Picosat proofs, etc.), and run the protocol. Claim 1 and Claim 3 are False instances verified by ZKQRES and ZKWS, respectively; Claim 2 and Claim 4 are True instances verified by ZKQRES and ZKWS, respectively.
- **Claims 5-8**: reproduce the **trends** shown in the paper. 

Since the paper’s figures were produced on different hardware, exact numbers may vary. For the AWS setup here, see per-claim expected outputs in `claim{1..8}/expected`.

**How to run a claim** (from `claims/claim<i>`):

```bash
./run.sh <zkqbf-suite-dir-path>
```

- **Claims 1–4** write `result.txt` in `claim<i>/`.
- **Claims 5–8** write plots to `claim<i>/plots/`.

**Expected per-claim runtime**

- Claim 1: ~15 minutes  
- Claim 2: ~10 minutes  
- Claim 3: ~ 8 minutes  
- Claim 4: ~ 8 minutes  
- Claim 5: ~35 minutes  
- Claim 6: ~25 minutes  
- Claim 7: ~30 minutes  
- Claim 8: ~10 minutes

**Total across all claims:**                             ~ **141 minutes**

**Estimate time for AE to check the files and compare:** ~  **45 minutes**

> **Important (Disk Usage):**  
> Proofs for PSPACE-complete and co-NP-complete problems can be large.  
> After inspecting outputs for **Claims 1–4**, we recommend deleting intermediate directories at `claim<i>/<intermediate_files>/`.  
> For **Claims 5–6**, consider removing `claim<i>/benchmark/` after you finish.

If you would like to run any protocol on a QDIMACS file of your choice, you can replace the file in the benchmark directory of one of the claim directories (claim1–claim4), depending on the protocol you wish to test. Note that the file structure must be `claim<i>/benchmark/<subdir>/<formula.qdimacs>` (benchmark should only contain one sub-directory).

## A.3 A Note on the Benchmarks Presented

The benchmarks presented in this package are selected to be sparse in their proof sizes and ZKQRES/ZKWS protocol runtime. We do not provide all the benchmarks due to total runtime, disk-space and RAM considerations.