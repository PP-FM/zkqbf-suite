from array import array
from dataclasses import dataclass
import sys
import faulthandler; faulthandler.enable()

@dataclass
class QResolutionTuple:
    index: int
    reduced_clause: set
    support : array
    removed: array = 0
    pivot: array = 0
    raw_clause: set = 0

    def print(self):
        if self.raw_clause:
            print("index: ", self.index, " raw_clause: ", " ".join([str(i) for i in self.raw_clause]), " reduced_clause: ", " ".join([str(i) for i in self.reduced_clause]), " support: ", " ".join([str(i) for i in self.support]), " pivot: ", " ".join([str(i) for i in self.pivot]), " removed: ", " ".join([str(i) for i in self.removed]), " end: ", 0)
        else:
            print("index: ", self.index, " raw_clause: ", " ".join([str(i) for i in self.reduced_clause]), " reduced_clause: ", " ".join([str(i) for i in self.reduced_clause]), " support: ", " ".join([str(i) for i in self.support]), " pivot: ", " ".join([str(i) for i in self.pivot]), " removed: ", " ".join([str(i) for i in self.removed]), " end: ", 0) 


def forall_reduce():
    return 


def remove_ref_to_dup_cls(proof_cpy, variables, quantifiers):
    proof_with_remapped_supports = []
    clauses_to_remove = []
    for i in range(len(proof_cpy)):
        clause = proof_cpy[i]
        proof_with_remapped_supports.append(clause)
        if len(clause.support) == 2:
            pass
        elif len(clause.support) == 1:
            if len(clause.removed) == 0:
                clauses_to_remove.append(i)
                for j in range(i+1, len(proof_cpy)):
                    for k in range(len(proof_cpy[j].support)):
                        if clause.index == proof_cpy[j].support[k]:
                            proof_cpy[j].support[k] = clause.support[0]
    return proof_with_remapped_supports, clauses_to_remove


def remove_and_remap_proof(proof: list[QResolutionTuple], clauses_to_remove):
    removed_proof = []
    i = 0
    for clause in proof:
        if clause.index != clauses_to_remove[i]:
            removed_proof.append(clause)
        else:
            if i == (len(clauses_to_remove) - 1):
                continue
            else:
                i = i+1
    counter = 0
    for i in range(len(removed_proof)):
        temp_index = removed_proof[i].index
        removed_proof[i].index = counter
        for j in range(i+1, len(removed_proof)):
            for k in range(len(removed_proof[j].support)):
                if removed_proof[j].support[k] == temp_index:
                    removed_proof[j].support[k] = counter
        counter = counter + 1
    return removed_proof


def preprocess(proof: list[QResolutionTuple], variables, quantifiers):
    input_formula = []
    preprocessed_proof = []
    proof_cpy = proof.copy()
    proof_with_remapped_supports, clauses_to_remove = remove_ref_to_dup_cls(proof_cpy, variables, quantifiers)
    removed_proof = remove_and_remap_proof(proof_with_remapped_supports, clauses_to_remove)
    return removed_proof


### Note: This tool assumes that there is no forall-reduction
### before qresolution is performed.
def qresolution(clsa, clsb, variables, quantifiers):
    clsb_c = set()
    for lit in clsb:
        clsb_c.add(-lit)
    p = clsa.intersection(clsb_c)
    if len(p) != 1:
        print(clsa, clsb)
        print(p)
        raise Exception("Tautology present in resolvent. Qres is faulty.")
    assert (len(p) == 1)
    p = list(p)
    index = variables.index(abs(p[0]))
    assert (quantifiers[index] == 'e')
    res = clsa.union(clsb)
    #  print(res)
    pivot = p[0]
    res.remove(pivot)
    res.remove(-pivot)
    assert (pivot != 0)
    return res, pivot


def check_forall_reduction(raw_cls: set, reduced_cls: set, variables, quantifiers):
    raw_existential_indices = []
    removed_indices = []
    last_rawcls_evar_index = -1
    cls_c = raw_cls.copy()
    for lit in raw_cls:
        index = variables.index(abs(lit))
        if quantifiers[index] == 'e':
            raw_existential_indices.append(index)
    raw_existential_indices.sort(reverse=True)
    if raw_existential_indices != []:
        last_rawcls_evar_index = raw_existential_indices[0]
    removed_lits = raw_cls - reduced_cls
    for lit in removed_lits:
        index = variables.index(abs(lit))
        removed_indices.append(index)
    removed_indices.sort()
    if removed_indices != []:
        if removed_indices[0] < last_rawcls_evar_index:
            raise Exception("Index of a removed literal is less than that of the last existential variable in the clause.")
    for ind in removed_indices:
        if quantifiers[ind] == 'e':
            raise Exception("Removed literal is not existential")
    return removed_lits


def extract_literals_and_quantifiers(quantifier_lines):
    literals = []
    quantifiers = []
    for line in quantifier_lines:
        x = line.split()
        for item in x[1:]:
            if item != '0':
                literals.append(int(item))
                quantifiers.append(x[0])

    return literals, quantifiers


def parse(str):
    #ResTuple = resolution()
    line = str.split(" ")
    index = int(line[0])
    reduced_clause = set();
    support = []
    i = 1
    while (line[i].strip('\n') != '0'):
        reduced_clause.add(int(line[i]))
        i = i + 1
    i = i + 1
    while (line[i].strip('\n') != '0'):
        support.append(int(line[i]))
        i = i + 1
    #print(ResTuple)
    if len(reduced_clause) == 0:
        reduced_clause.add(0)
    res = QResolutionTuple(index, reduced_clause, support)
    return res


remap = {}

def regularize ( filename, start = 0):
    resolution_list = []
    proof = open(filename, 'r')
    Lines = proof.readlines()
    counter = start
    first_line = False
    global quantifier_lines
    quantifier_lines = []

    for line in Lines:
        if not first_line:
            x = line.split()
            if x[0][0] == 'c':
                continue
            elif x[0] == "p":
                first_line = True
                continue
            else:
                raise Exception("Not in the correct format.")
        if line[0] == 'a' or line[0] == 'e':
            quantifier_lines.append(line)
            continue
        if line[0] == 'r':
            x = line.split()
            assert(x[1] == "UNSAT" or x[1] == "unsat")
            continue
        if line == "0":
            continue
        res = parse(line)
        # if len(res) == continue
        remap[res.index] = counter
        res.index = counter
        support = []
        for index in res.support:
            support.append(remap[index])
        res.support = support
        counter = counter + 1
        resolution_list.append(res)

    return resolution_list


def check_qrp (resolution_list: list[QResolutionTuple], variables, quantifiers):
    resolution_list_c = resolution_list.copy()
    length = 0
    counter = 0

    for t in resolution_list_c:
        pivot_t = []
        raw_clause = set()
        if (len(t.support) == 1):
            t.removed = check_forall_reduction(resolution_list[t.support[0]].reduced_clause.copy(), t.reduced_clause, variables, quantifiers)
        elif (len(t.support) == 2):
            tmp_cls = resolution_list[t.support[0]].reduced_clause.copy()
            raw_clause, p = qresolution(tmp_cls, resolution_list[t.support[1]].reduced_clause.copy(), variables, quantifiers)
            t.removed = check_forall_reduction(raw_clause, t.reduced_clause, variables, quantifiers)
            pivot_t.append(p)
        elif (len(t.support) > 2):
            raise Exception("Doesn't support chained proofs yet")
        if len(t.reduced_clause) > length:
            length = len(t.reduced_clause)
        if len(t.support) == 0:
            t.removed = []
        if len(t.removed) > length:
            length = len(t.removed)
        t.pivot = pivot_t
        t.raw_clause = raw_clause

    return  resolution_list_c, length


def GetChainLength(resolution_list):
    max = 0
    for e in resolution_list:
        if len(e.support) > max:
            max = len(e.support)
    return max


proofname = sys.argv[1]
#distribution = {}
resolution_raw = regularize(proofname)
#print(proofname)
chain_length =  GetChainLength(resolution_raw)
#print(chain_length)
sys.setrecursionlimit(chain_length * 4)

variables, quantifiers = extract_literals_and_quantifiers(quantifier_lines)

proof, length= check_qrp(resolution_raw, variables, quantifiers)

proof = preprocess(proof, variables, quantifiers)

for line in quantifier_lines:
    print(line)
for e in proof:
    e.print()
print("DEGREE:", length+1)