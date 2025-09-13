from array import array
from dataclasses import dataclass
from dataclasses import replace
import sys
import faulthandler; faulthandler.enable()
from pysat.solvers import Glucose3


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
        raise Exception("Contradiction present in resolvent. Q-cube-res is faulty.")
    assert (len(p) == 1)
    p = list(p)
    index = variables.index(abs(p[0]))
    assert (quantifiers[index] == 'a')
    res = clsa.union(clsb)
    #  print(res)
    pivot = p[0]
    res.remove(pivot)
    res.remove(-pivot)
    assert (pivot != 0)
    return res, pivot


def check_existential_reduction(raw_cls: set, reduced_cls: set, variables, quantifiers):
    raw_forall_indices = []
    removed_indices = []
    last_rawcls_avar_index = -1
    cls_c = raw_cls.copy()
    for lit in raw_cls:
        index = variables.index(abs(lit))
        if quantifiers[index] == 'a':
            raw_forall_indices.append(index)
    raw_forall_indices.sort(reverse=True)
    if raw_forall_indices != []:
        last_rawcls_avar_index = raw_forall_indices[0]
    removed_lits = raw_cls - reduced_cls
    for lit in removed_lits:
        index = variables.index(abs(lit))
        removed_indices.append(index)
    removed_indices.sort()
    if removed_indices != []:
        if removed_indices[0] < last_rawcls_avar_index:
            raise Exception("Index of a removed literal is less than that of the last forall variable in the clause.")
    for ind in removed_indices:
        if quantifiers[ind] == 'a':
            raise Exception("Removed literal is not existentially quantified.")
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
            assert(x[1] == "SAT" or x[1] == "sat")
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
            t.removed = check_existential_reduction(resolution_list[t.support[0]].reduced_clause.copy(), t.reduced_clause, variables, quantifiers)
            #assert(len(t.support) == 2)
        elif (len(t.support) == 2):
            tmp_cls = resolution_list[t.support[0]].reduced_clause.copy()
            raw_clause, p = qresolution(tmp_cls, resolution_list[t.support[1]].reduced_clause.copy(), variables, quantifiers)
            t.removed = check_existential_reduction(raw_clause, t.reduced_clause, variables, quantifiers)
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


def reorder_proof(proof, length):
    old_index_to_new_index = {}
    counter = 0
    reordered_proof = []
    for e in proof:
        if len(e.raw_clause) > length:
            length = len(e.raw_clause)
        if len(e.support) == 0:
            old_index_to_new_index[e.index] = counter
            e.index = counter
            reordered_proof.append(e)
            counter = counter + 1
    for e in proof:
        if len(e.support) >= 1:
            for i in e.support:
                if i in old_index_to_new_index.keys():
                    e.support[e.support.index(i)] = old_index_to_new_index[i]
            old_index_to_new_index[e.index] = counter
            e.index = counter
            counter = counter + 1
            reordered_proof.append(e)
    for e in proof:
        if len(e.support) == 2:
            if e.pivot[0] in reordered_proof[e.support[0]].reduced_clause:
                continue
            if e.pivot[0] in reordered_proof[e.support[1]].reduced_clause:
                e.pivot[0] = -e.pivot[0]
    
    return reordered_proof, length


def existentially_reduce(raw_clause: set, variables, quantifiers):
    last_forall_var_index = -1
    removed = []
    for lit in raw_clause:
        index = variables.index(abs(lit))
        if quantifiers[index] == 'a':
            if last_forall_var_index < index:
                last_forall_var_index = index

    reduced_clause = raw_clause.copy()
    for lit in raw_clause:
        index = variables.index(abs(lit))
        if quantifiers[index] == 'e':
            if index > last_forall_var_index:
                reduced_clause.remove(lit)
                removed.append(lit)
    
    return reduced_clause, removed


def resolve(reduced_clause_1: set, reduced_clause_2: set, variables, quantifiers):
    clsb_c = set()
    for lit in reduced_clause_2:
        clsb_c.add(-lit)
    p = reduced_clause_1.intersection(clsb_c)
    if len(p) > 1:
        raise Exception("Contradiction present in resolvent. Q-cube-res is faulty.")
    if len(p) == 0:
        raise Exception("No pivot present in resolvent. Q-cube-res is faulty.")
    assert (len(p) == 1)
    p = list(p)
    assert (p[0] != 0)
    index = variables.index(abs(p[0]))
    assert (quantifiers[index] == 'a')
    res = reduced_clause_1.union(reduced_clause_2)
    #  print(res)
    pivot = p[0]
    res.remove(pivot)
    res.remove(-pivot)
    return res, p


def shrink_proof(proof: QResolutionTuple, variables, quantifiers):
    counter = 0
    shrinked_proof = []
    initial_cubes = []
    initial_cube_indices = []
    support_remap = {}
    to_be_removed = []
    already_visited = []
    for e in proof:
        if len(e.support) == 0:
            initial_cubes.append(e)
            initial_cube_indices.append(e.index)
        if len(e.support) == 1:
            new_cube = replace(e)
            if e.support[0] in initial_cube_indices:
                if e.support[0] not in already_visited:
                    already_visited.append(e.support[0])
                    new_cube.support = []
                    new_cube.raw_clause, _ = existentially_reduce(proof[e.support[0]].reduced_clause, variables, quantifiers)
                    new_cube.reduced_clause = new_cube.raw_clause
                    new_cube.removed = []
                    new_cube.index = counter
                    support_remap[e.support[0]] = counter
                    support_remap[e.index] = counter
                    new_cube.raw_clause = proof[e.support[0]].reduced_clause
                    shrinked_proof.append(new_cube)
                    counter = counter + 1
                else:
                    support_remap[e.index] = support_remap[e.support[0]]
            else:
                #TODO: Work on this
                support_remap[e.index] = support_remap[e.support[0]]
        if len(e.support) == 2:
            new_resolved_cube, pivot = resolve(shrinked_proof[support_remap[e.support[0]]].reduced_clause, shrinked_proof[support_remap[e.support[1]]].reduced_clause, variables, quantifiers)
            new_cube = replace(e)
            new_cube.raw_clause = new_resolved_cube
            new_cube.reduced_clause, new_cube.removed = existentially_reduce(new_resolved_cube, variables, quantifiers)
            new_cube.pivot = pivot
            new_cube.index = counter
            support_remap[e.index] = counter
            new_cube.support = [support_remap[e.support[0]], support_remap[e.support[1]]]
            shrinked_proof.append(new_cube)
            counter = counter + 1
    
    return shrinked_proof


def get_qbf_clauses(qbf_file):
    qbf_clauses = []
    with open(qbf_file, 'r') as f:
        lines = f.readlines()
        for line in lines:
            if line == "" or line == "\n":
                continue
            if line[0] == 'p':
                continue
            if line[0] == 'c':
                continue
            if line[0] == 'e' or line[0] == 'a':
                continue
            if line.strip() == "0":
                continue
            temp = line.split()
            tautology = False
            for x in temp[:-1]:
                if str(-int(x)) in temp:
                    tautology = True
                    break
            if not tautology:
                qbf_clauses.append([int(x) for x in temp[:-1]])

    return qbf_clauses


def complete_initial_cubes(qbf_clauses, shrinked_proof, variables, quantifiers):
    for e in shrinked_proof:
        if len(e.support) == 0:
            complete_initial_cube = []
            qbf_clauses_copy = qbf_clauses.copy()
            for clause in qbf_clauses:
                intersection = set(clause).intersection(e.raw_clause)
                if intersection != set():
                    qbf_clauses_copy.remove(clause)
                else:
                    qbf_clauses_copy.remove(clause)
                    neg_raw_cube = {-x for x in e.raw_clause}
                    clause_set = set(clause)
                    new_clause_set = clause_set - neg_raw_cube
                    for lit in new_clause_set:
                        index = variables.index(abs(lit))
                        if quantifiers[index] == 'a':
                            new_clause_set.remove(lit)
                    qbf_clauses_copy.append(list(new_clause_set))
            if qbf_clauses_copy == []:
                continue
            else:
                g = Glucose3()
                for clause in qbf_clauses_copy:
                    g.add_clause(clause)
                if g.solve(assumptions=list(e.raw_clause)):
                    complete_initial_cube = (e.raw_clause).union(set(g.get_model()))
                    e.raw_clause = set(complete_initial_cube)
    
    return 


proofname = sys.argv[1]
qbf_file = sys.argv[2]
#distribution = {}
qbf_clauses = get_qbf_clauses(qbf_file)
resolution_raw = regularize(proofname)
#print(proofname)
chain_length =  GetChainLength(resolution_raw)
#print(chain_length)
sys.setrecursionlimit(chain_length * 4)

variables, quantifiers = extract_literals_and_quantifiers(quantifier_lines)

proof, length= check_qrp(resolution_raw, variables, quantifiers)

for line in quantifier_lines:
    if line[0] == 'a':
        print("e", line[2:])
    else:
        print("a", line[2:])

#reordered_proof, length = reorder_proof(proof, length)
shrinked_proof = shrink_proof(proof, variables, quantifiers)
complete_initial_cubes(qbf_clauses, shrinked_proof, variables, quantifiers)
shrinked_proof, length = reorder_proof(shrinked_proof, length)
for e in shrinked_proof:
    e.removed = list(e.raw_clause - e.reduced_clause)
    e.print()
print("DEGREE:", length+3)
