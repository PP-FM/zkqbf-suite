# ZKQBF

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