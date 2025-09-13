import sys

certfile = sys.argv[1]
cert = open(certfile, 'r')
lines = cert.readlines()
if lines == []:
    sys.exit(2)
if "UNSAT" in lines[-1]:
    sys.exit(0)
elif "SAT" in lines[-1]:
    sys.exit(1)