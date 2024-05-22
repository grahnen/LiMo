#!/bin/env python3
import glob
import sys
import re

history_root = "../oolin/histories/simple/"

call_regex = re.compile(r' *\[(\d+)\] call ([a-zA-Z]+) ?\(?([a-z0-9A-Z\?<,>]*)\)?')
ret_regex = re.compile(r' *\[(\d+)\] return ?([a-z0-9A-Z]*)')

def get_thr(thrs):
    i = 1
    while i in thrs.values():
        i += 1
    return i

def hist2tex(history, condense = True):
    res = ""
    threads = {}
    for line in history:
        crm = call_regex.match(line)
        rrm = ret_regex.match(line)
        if crm != None:
            th = get_thr(threads) if condense else crm.group(1)
            threads[int(crm.group(1))] = th
            res +="\\call{" + str(th) + "}{\\" + crm.group(2)
            if crm.group(3):
                res += "{" + crm.group(3) + "}"
            else:
                res += "{}"

            res += "}\n"
        elif rrm != None:
            th = threads[int(rrm.group(1))] if condense else rrm.group(1)
            
            threads.pop(int(rrm.group(1)))
            res += "\\ret"
            if rrm.group(2):
                res += "[" + rrm.group(2) + "]"
            res += "{" + str(th) + "}\n"
    return res


def h2t(filename):
    with open(filename) as f:
        d = f.readlines()
    return hist2tex(d)

if __name__=="__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} [filename]")
        exit(-1)

    tex = h2t(sys.argv[1])
    sys.stdout.write("\\begin{timeline}")
    sys.stdout.write(tex)
    sys.stdout.write("\\end{timeline}")
