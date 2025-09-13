import sys
from array import array
from dataclasses import dataclass
from math import floor

truth_var = 0
max_var = 0
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
    global truth_var
    global max_var
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
        elif len(line) == 2:
            if line[0] == "Maxvar:":
                max_var = int(int(line[1])*2)
                truth_var = max_var + 2
    
    return clauses

if sys.argv[1] == "--zkskoval-input":
    qbf_aigfile = sys.argv[2]
    clauses = parse(qbf_aigfile)
    print("Original-Quantifiers:")
    for line in quantifier_lines:
        print(line)
    print("Formula-for-negQBF:")
    for clause in clauses:
        clause.print()
    print(int(truth_var/2), 0)
    print(-int((max_var)/2), 0)
else:
    skofile = sys.argv[1]
    qbf_aigfile = sys.argv[2]
    clauses = parse(skofile)
    clauses.extend(parse(qbf_aigfile))
    print("p cnf", int(truth_var/2), (len(clauses)*3)+2)
    for clause in clauses:
        clause.print()
    print(int(truth_var/2), 0)
    print(-int((max_var)/2), 0)