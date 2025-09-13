import sys

cert_file = sys.argv[1]
cert = open(cert_file, 'r')
lines = cert.readlines()
for line in lines:
    words = line.split()
    if words[0] == 'aag':
        print(words[1])