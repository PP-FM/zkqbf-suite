import sys
from array import array
from dataclasses import dataclass
from math import floor

quantifier_lines = []

@dataclass
class AIGERline:
    result : int;
    support : array;

    def print(self):
        global truth_var
        for i in range(2):
            if self.support[i] == 0:
                self.support[i] = truth_var+1
            elif self.support[i] == 1:
                self.support[i] = truth_var
        print(-int(self.result/2), int(self.support[0]/2) * (1 if self.support[0]%2 == 0 else -1), 0)
        print(-int(self.result/2), int(self.support[1]/2) * (1 if self.support[1]%2 == 0 else -1), 0)
        if (self.support[0] == self.support[1]):
            print(int(self.result/2), int(self.support[0]/2) * (-1 if self.support[0]%2 == 0 else 1), 0)
        else:
            print(int(self.result/2), int(self.support[0]/2) * (-1 if self.support[0]%2 == 0 else 1), int(self.support[1]/2) * (-1 if self.support[1]%2 == 0 else 1), 0)


def parse(aigfile):
    clauses = []
    aig = open(aigfile, 'r')
    Lines = aig.readlines()
    counter = 1
    global quantifier_lines

    for str in Lines:
        line = str.split(" ")
        if (line[0] == "a") or (line[0] == "e"):
            quantifier_lines.append(str[:-1])
            continue
        elif len(line) == 3:
            support = []
            support.append(int(line[1]))
            support.append(int(line[2]))
            clauses.append(AIGERline(int(line[0]), support))
    
    return clauses


skofile = sys.argv[1]
qbffile = sys.argv[2]
if (skofile != "--zkherbval-verifier"):
    qbf = open(qbffile, 'r')
    lines = qbf.readlines()
    quantifier_lines = 0
    for line in lines:
        if line[0] == "a":
            quantifier_lines += 1
        elif line[0] == "e":
            quantifier_lines += 1
    max_var = int(sys.argv[3])
    truth_var = 2*max_var + 2
    clauses = parse(skofile)
    if lines[0][0] == "p":
        print("p cnf", int(truth_var/2), (len(clauses)*3)+1+ len(lines)-1-quantifier_lines)
        for clause in clauses:
            clause.print()
        for line in lines[1+quantifier_lines:]:
            if len(line) != 0:
                if line[0] != '\n':
                    print(line[:-1])
        print(int(truth_var/2), 0)
    else:
        print("p cnf", int(truth_var/2), (len(clauses)*3)+1+ len(lines)-quantifier_lines)
        for clause in clauses:
            clause.print()
        for line in lines[quantifier_lines:]:
            if len(line) != 0:
                if line[0] != '\n':
                    print(line[:-1])
        print(int(truth_var/2), 0)
else:
    qbf = open(qbffile, 'r')
    lines = qbf.readlines()
    quantifier_lines = 0
    for line in lines:
        if line[0] == "a":
            quantifier_lines += 1
        elif line[0] == "e":
            quantifier_lines += 1
    max_var = int(sys.argv[3])
    truth_var = 2*max_var + 2
    if lines[0][0] == "p":
        print("p cnf", int(truth_var/2), (len(lines)-1-quantifier_lines+1))
        for line in lines[1+quantifier_lines:]:
            if len(line) != 0:
                if line[0] != '\n':
                    print(line[:-1])
        print(int(truth_var/2), 0)
    else:
        print("p cnf", int(truth_var/2), (len(lines)-quantifier_lines+1))
        for line in lines[quantifier_lines:]:
            if len(line) != 0:
                if line[0] != '\n':
                    print(line[:-1])
        print(int(truth_var/2), 0)