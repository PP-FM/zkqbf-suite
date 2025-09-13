import sys

def get_old_names_to_new_names(qdimacs_file, old_names_to_new_names):
    count = 1
    qbf = open(qdimacs_file, 'r')
    lines = qbf.readlines()
    global new_var_to_quantifier
    global quantifier_lines
    global max_var
    for line in lines:
        words = line.split()
        if words[0] == 'a':
            quantifier_lines += 1
            for i in range(1, len(words)-1): # last word is '0'
                old_names_to_new_names[int(words[i])] = count
                old_names_to_new_names[-int(words[i])] = -count
                new_var_to_quantifier[count] = 'a'
                new_var_to_quantifier[-count] = 'a'
                count += 1
        elif words[0] == 'e':
            quantifier_lines += 1
            for i in range(1, len(words) -1): # last word is '0'
                old_names_to_new_names[int(words[i])] = count
                old_names_to_new_names[-int(words[i])] = -count
                new_var_to_quantifier[count] = 'e'
                new_var_to_quantifier[-count] = 'e'
                count += 1
    max_var = count - 1
    return old_names_to_new_names

def rewrite(qdimacs_file, old_names_to_new_names):
    qbf = open(qdimacs_file, 'r')
    lines = qbf.readlines()
    encountered_preamble = False
    global max_var
    global quantifier_lines
    global new_var_to_quantifier
    comment_lines = 0
    for line in lines:
        words = line.split()
        if words[0] == 'p':
            print(line[:-1])
            encountered_preamble = True
        elif words[0] == 'a' or words[0] == 'e':
            if not encountered_preamble:
                print ("p cnf", max_var, len(lines)-comment_lines-quantifier_lines)
                encountered_preamble = True
            print(words[0], *[old_names_to_new_names[int(words[i])] for i in range(1, len(words)-1)], '0')
        elif words[0] == 'c':
            comment_lines += 1
            continue
        else:
            new_set_of_words = get_new_words(words, old_names_to_new_names, new_var_to_quantifier)
            print(*[new_set_of_words[i] for i in range(len(new_set_of_words))], '0')


def get_new_words(words, old_names_to_new_names, new_var_to_quantifier):
    new_set_of_words = []
    abs_of_words = []
    for i in range(len(words)):
        if int(words[i]) in old_names_to_new_names:
            new_set_of_words.append(old_names_to_new_names[int(words[i])])
            abs_of_words.append(abs(old_names_to_new_names[int(words[i])]))
        elif int(words[i]) == 0:
            continue
        else:
            raise ValueError(f"Variable {words[i]} not found in quantifier lines.")
    abs_of_words.sort(reverse=True)
    found_existential = False
    need_to_remove = []
    for i in range(len(abs_of_words)):
        if new_var_to_quantifier[abs_of_words[i]] == 'e' and (not found_existential):
            found_existential = True
        elif new_var_to_quantifier[abs_of_words[i]] == 'a' and (not found_existential):
            need_to_remove.append(abs_of_words[i])
    for i in range(len(need_to_remove)):
        if (need_to_remove[i] in new_set_of_words) and ((need_to_remove[i] * -1) in new_set_of_words):
            raise ValueError(f"Variable {need_to_remove[i]} and its negation are both present in the same clause.")
        if need_to_remove[i] in new_set_of_words:
            new_set_of_words.remove(need_to_remove[i])
        if need_to_remove[i] * -1 in new_set_of_words:
            new_set_of_words.remove(need_to_remove[i] * -1)
    return new_set_of_words


# This code is to rename the variables in the qdimacs file.
# Note: Please do not have any comments after the preamble for the qdimacs!
qdimacs_file = sys.argv[1]
quantifier_lines = 0
max_var = 0
new_var_to_quantifier = {}
old_names_to_new_names = {}
old_names_to_new_names = get_old_names_to_new_names(qdimacs_file, old_names_to_new_names)
rewrite(qdimacs_file, old_names_to_new_names)