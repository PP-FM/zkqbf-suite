import sys

def format_certfile_and_print(certfile):
    cert = open(certfile, 'r')
    lines = cert.readlines()

    for line in lines:
        words = line.split()
        if len(words) == 3:
            var = int(words[0])
            inp1 = int(words[1])
            inp2 = int(words[2])
            if (inp1 == inp2 + 1) or (inp2 == inp1 + 1):
                if (min(inp1, inp2) % 2) == 0:
                    print(var, 0, 0)
                else:
                    print(line[:-1])
            else:
                print(line[:-1])
        else:
            print(line[:-1])


certfile = sys.argv[1]
format_certfile_and_print(certfile)