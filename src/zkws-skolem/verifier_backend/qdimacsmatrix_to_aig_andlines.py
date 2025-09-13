import sys
from array import array
from dataclasses import dataclass
from math import floor

@dataclass
class QBFClause:
    clause : array;
    accumulated_neg_var : int = 0;

    def print(self):
        global current_var
        if (len(self.clause) == 1):
            current_var = current_var + 2    
            print(current_var, 2 * abs(self.clause[0]) + (0 if self.clause[0] < 0 else 1), 1)
        else:
            current_var += 2
            print(current_var, 2 * abs(self.clause[0]) + (0 if self.clause[0] < 0 else 1), 2 * abs(self.clause[1]) + (0 if self.clause[1] < 0 else 1))
            if len(self.clause) > 2:
                for i in self.clause[2:]:
                    current_var += 2
                    print(current_var, 2 * abs(i) + (0 if i < 0 else 1), current_var-2)
        self.accumulated_neg_var = current_var


def read_qbf_line(line):
    clause = []
    for i in line:
        if (i != "0\n"):
            clause.append(int(i))
    return(QBFClause(clause))


def parse(qbffile):
    qbf_matrix = []
    qbf = open(qbffile, 'r')
    Lines = qbf.readlines()

    for str in Lines:
        line = str.split(" ")
        if (line[0] == "a" or line[0] == "e"):
            print(str[:-1]) #This should just be the '\n' character
        elif (line[0] == "p") or (line[0] == "c"):
            continue
        else:
            qbf_matrix.append(read_qbf_line(line))
    
    return qbf_matrix


qbffile = sys.argv[1]
skolem_max_var = int(sys.argv[2])
current_var = 2*skolem_max_var
qbf_matrix = parse(qbffile)
for clause in qbf_matrix:
    clause.print()
if len(qbf_matrix) == 1:
    try:
        assert(qbf_matrix[0].accumulated_neg_var == current_var)
    except:
        print("Something is wrong.")
else:
    current_var += 2
    print(current_var, qbf_matrix[0].accumulated_neg_var+1, qbf_matrix[1].accumulated_neg_var+1)
    if len(qbf_matrix) > 2:
        for clause in qbf_matrix[2:]:
            current_var += 2
            print(current_var, clause.accumulated_neg_var+1, current_var-2)
print("Maxvar:", int(current_var/2))
