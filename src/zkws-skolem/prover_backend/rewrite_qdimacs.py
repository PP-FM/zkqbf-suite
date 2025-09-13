import sys

def get_old_names_to_new_names(qdimacs_file, old_names_to_new_names):
    count = 1
    qbf = open(qdimacs_file, 'r')
    lines = qbf.readlines()
    for line in lines:
        words = line.split()
        if words[0] == 'a':
            for i in range(1, len(words)-1): # last word is '0'
                old_names_to_new_names[int(words[i])] = count
                old_names_to_new_names[-int(words[i])] = -count
                count += 1
        elif words[0] == 'e':
            for i in range(1, len(words) -1): # last word is '0'
                old_names_to_new_names[int(words[i])] = count
                old_names_to_new_names[-int(words[i])] = -count
                count += 1
    
    return old_names_to_new_names

def rewrite(qdimacs_file, old_names_to_new_names):
    qbf = open(qdimacs_file, 'r')
    lines = qbf.readlines()
    for line in lines:
        words = line.split()
        if words[0] == 'p':
            print(line[:-1])
        elif words[0] == 'a' or words[0] == 'e':
            print(words[0], *[old_names_to_new_names[int(words[i])] for i in range(1, len(words)-1)], '0')
        elif words[0] == 'c':
            continue
        else:
            print(*[old_names_to_new_names[int(words[i])] for i in range(0, len(words)-1)], '0')


# This code is to rename the variables in the qdimacs file
qdimacs_file = sys.argv[1]
old_names_to_new_names = {}
old_names_to_new_names = get_old_names_to_new_names(qdimacs_file, old_names_to_new_names)
rewrite(qdimacs_file, old_names_to_new_names)