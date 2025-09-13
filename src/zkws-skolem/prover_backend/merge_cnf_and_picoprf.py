import sys

def merge_files_and_print(proofname, cnfname):
    proof = open(proofname, 'r')
    cnf = open(cnfname, 'r')
    prooflines = proof.readlines()
    cnflines = cnf.readlines()
    counter = 1

    for line in cnflines:
        words = line.split()
        if words[0] == 'p':
            continue
        else:
            print(counter, line[:-1], 0)
            counter += 1
    
    for line in prooflines:
        words = line.split()
        index = int(words[0])
        if index <= len(cnflines) - 1:
            continue
        else:
            print(line[:-1])


proofname = sys.argv[2]
cnfname = sys.argv[1]
merge_files_and_print(proofname, cnfname)