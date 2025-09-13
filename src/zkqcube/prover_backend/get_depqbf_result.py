import sys

prooffile = sys.argv[1]
proof = open(prooffile, "r")
Lines = proof.readlines()
if Lines == []:
    sys.exit(2)
if "SAT" in Lines[-1]:
    sys.exit(0)
if "sat" in Lines[-1]:
    sys.exit(0)
sys.exit(2)