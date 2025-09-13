import sys

qdimacs_file = sys.argv[1]
max_var = int(sys.argv[2])  #This is the maximum variable number in the AIGER file of the herbrand function
qdimacs = open(qdimacs_file, 'r')
qdimacs_lines = qdimacs.readlines()
for line in qdimacs_lines:
    if line.startswith('c'):
        continue
    elif line.startswith('p'):
        words = line.split()
        assert(len(words) == 4)
        if words[3][-1] == '\n':
            words[3] = words[3][:-1]
        print('p cnf', max_var+1, int(words[3]) + 1)
    elif line[0] != '\n':
        print(line[:-1])
print(max_var+1, 0)

