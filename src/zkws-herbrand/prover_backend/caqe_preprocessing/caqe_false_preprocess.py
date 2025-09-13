import sys

def load_quantifiers(qbffile):
    qbf = open(qbffile, 'r')
    lines = qbf.readlines()
    global a_vars
    global e_vars
    global max_var

    for line in lines:
        words = line.split()
        if len(words) == 0:
            continue
        if words[0] == 'a':
            for i in range(1, len(words) - 1): # last word is '0'
                if max_var < int(words[i]):
                    max_var = int(words[i])
                a_vars.append(int(words[i]))
        elif words[0] == 'e':
            for i in range(1, len(words) - 1): # last word is '0'
                if max_var < int(words[i]):
                    max_var = int(words[i])
                e_vars.append(int(words[i]))


def get_symbols(certfile):
    cert = open(certfile, 'r')
    lines = cert.readlines()
    global a_vars
    global e_vars
    global num_symbols
    global i_symbols
    global o_symbols

    for line in lines:
        words = line.split()
        if len(words) == 2:
            if words[0][0] == 'o':
                if words[1][0] == 'o':
                    o_symbols[int(words[0][1:])] = words[1][1:]
                else:
                    o_symbols[int(words[0][1:])] = words[1]
                num_symbols += 1
            elif words[0][0] == 'i':
                if words[1][0] == 'i':
                    i_symbols[int(words[0][1:])] = words[1][1:]
                else:
                    i_symbols[int(words[0][1:])] = words[1]
                num_symbols += 1
                
def format_certfile_and_print(certfile):
    cert = open(certfile, 'r')
    lines = cert.readlines()
    i_var_count = 0
    encountered_non_numeric = False
    global max_var

    words = lines[0].split()
    num_and_gates = int(words[5])
    aag_max_var = max_var + num_and_gates
    print('aag', aag_max_var, len(i_symbols), 0, len(o_symbols)-1, num_and_gates+len(o_symbols)-1)

    for i in range(1, len(i_symbols)+1):
        line = lines[i]
        words = line.split()
        if len(words) == 1:
            original_names_to_new_names[int(words[0])] = 2*int(i_symbols[i_var_count])
            print(2*int(i_symbols[i_var_count]))
            i_var_count += 1

    assert(i_var_count == len(e_vars))
    o_var_count = 0

    assert(len(o_symbols) == len(a_vars) + 1)

    for i in range(len(i_symbols)+1, len(i_symbols)+1+len(o_symbols)+1):
        line = lines[i]
        words = line.split()
        if len(words) == 1:
            try:
                temp = int(o_symbols[o_var_count])
            except:
                o_var_count += 1
                encountered_non_numeric = True
                continue
            lines_to_add.append(str(2*int(o_symbols[o_var_count])) + ' 1 ' + words[0])
            print(str(2*int(o_symbols[o_var_count])))
            o_var_count += 1

    max_var = max_var + 1
    for i in range(1, len(lines)):
        line = lines[i]
        words = line.split()
        if len(words) == 3:
            original_names_to_new_names[int(words[0])] = 2*max_var
            var = 2*max_var
            inp1 = int(words[1])
            inp2 = int(words[2])
            if inp1 % 2 == 0:
                if inp1 not in original_names_to_new_names.keys():
                    raise Exception("A variable doesn't depend on the previous variables")
                inp1 = original_names_to_new_names[inp1]
            else:
                if inp1-1 not in original_names_to_new_names.keys():
                    raise Exception("A variable doesn't depend on the previous variables")
                inp1 = 1 + original_names_to_new_names[inp1-1]
            if inp2 % 2 == 0:
                if inp2 not in original_names_to_new_names.keys():
                    raise Exception("A variable doesn't depend on the previous variables")
                inp2 = original_names_to_new_names[inp2]
            else:
                if inp2-1 not in original_names_to_new_names.keys():
                    raise Exception("A variable doesn't depend on the previous variables")
                inp2 = 1 + original_names_to_new_names[inp2-1]
            if (inp1 == inp2 + 1) or (inp2 == inp1 + 1):
                if min(inp1, inp2) % 2 == 0:
                    print(var, 0, 0)
                else:
                    print(var, inp1, inp2)
            else:
                print(var, inp1, inp2)
            max_var += 1
        elif len(words) == 2:
            continue
    
    for line in lines_to_add:
        words = line.split()
        var = int(words[0])
        inp1 = int(words[1])
        inp2 = int(words[2])
        if inp2 % 2 == 0:
            inp2 = original_names_to_new_names[inp2]
        else:
            inp2 = 1 + original_names_to_new_names[inp2-1]
        if (inp2 == 0): # Because inp1 is always 1
            print(var, 0, 0)
        else:
            print(var, inp1, inp2)

# This tool assumes that the input quantified boolean formula (QBF) is in prenex conjunctive normal form (PCNF).
# The certificate file is from CADET or derived from Manthan.
# The certificate should not have any comments.
a_vars = []
e_vars = []
i_symbols = {}
o_symbols = {}
lines_to_add = []
original_names_to_new_names = {}
original_names_to_new_names[0] = 0
max_var = 0
num_symbols = 0
certfile = sys.argv[1]
qbffile = sys.argv[2]
load_quantifiers(qbffile)
get_symbols(certfile)
format_certfile_and_print(certfile)