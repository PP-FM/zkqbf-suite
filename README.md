# ZKQBF

## Dependencies

To run the protocol local Ubuntu system. Ensure that you have the following:

* [emp-tool](https://github.com/emp-toolkit/emp-tool)
* [emp-ot](https://github.com/emp-toolkit/emp-ot)
* [emp-zk](https://github.com/emp-toolkit/emp-zk)
* [NTL](https://libntl.org/)

Additionally, if you only have the QBF formula as a PCNF form QDIMACS file, ensure that you have:

* [DepQBF-V1](https://lonsing.github.io/depqbf/)
* [CAQE-2](https://finkbeiner.groups.cispa.de/tools/caqe/)
* [picosat](https://fmv.jku.at/picosat/) (Please enable trace when configuring)
* [abc](https://github.com/berkeley-abc/abc)
* [aiger](https://github.com/arminbiere/aiger)


Note: The pysat library must be available before invoking zkqcube preprocessing step.
You can get pysat by running the following:
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install python-sat
```
After installing pysat on .venv, for all future zkqcube preprocessing, ensure that you have activated .venv.
This is already done in the prepare_for_aws.sh script (Do not run this unless you are setting this up on an
AWS instance for the first time). 

## Intructions to run protocols

In this section we will present multiple workflows for our protocol. 

### You have the QBF formula in PCNF form in the QDIMACS format.

Suppose the QDIMACS file is 'formula.qdimacs' The simplest way to verify this in ZK is to run the following:

First run: 

```bash
mkdir -p <formula_dir>
mv formula.qdimacs <formula_dir>
mkdir -p <bench_dir>
mv <formula_dir> <bench_dir>
```

Then, depending on the protocol you want to use, run one of the following:

**a) ZKQRES or ZKQCUBE Validation**: To perform a ZKQRES or ZKQCUBE Validation (ZKQRES for false QBFs, or ZKCUBE for true QBFs), from the zkqbf-suite directory,

Run

```bash
source <venv>/bin/activate
cd src/<zkqres/zkqcube>
cmake .
make
mkdir -p data
./run_everything.sh "<bench_dir> . <path_to_depqbf_v1> <path_to_qrpcheck> <port_number> <IP_address>"
```

It is okay to skip venv activation if you are not using ZKQCUBE.

**b) ZKWS-Herbrand Validation**: To perform a ZKWS-Herbrand Validation (Only for false QBFs), from the zkqbf-suite directory, run

```bash
cd src/zkws-herbrand
cmake .
make
cd prover_backend/caqe_preprocessing
mkdir -p data
./run_everything.sh "<bench_dir> <zkws-herbrand_dir> <caqe-2_dir> <abc_dir> <aiger_dir> <picosat_dir> <port_number> <IP_address>"
```

where `<port_number>` and `<IP_address>` are to establish communication between prover and verifier.
If you are running this locally (you are both prover and verifier), you can use `<port_number>=8000` 
(or any other port that is free) and `<IP_address>=127.0.0.1` (localhost).

