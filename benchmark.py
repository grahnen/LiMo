#!/bin/env python3

import os
import subprocess as sp
import time
import sys
import json
import glob
from dataclasses import dataclass

from pathlib import Path

try:
    from tqdm import tqdm
    def tqwrite(x):
        tqdm.write(x)
    def tqfor(x):
        return tqdm(x)
except:
    def tqfor(x):
        for i in x:
            yield i
    def tqwrite(x):
        print(x)

import re

@dataclass
class Test:
    path : str
    threads : int
    ops_per_thread : int
    done = False
    limo_result : float = 0
    violin_result : float = 0

    def run(self):
        self.limo_result, l_res = limousine(fn)
        self.violin_result, v_res = violin(fn)
        if(l_res != v_res):
            print(f"MISMATCH in {path}")

if len(sys.argv) < 3:
    print(f"Usage: {sys.argv[0]} [directory] [output]")
    exit(0)

PATH_TO_VIOLIN = "../violin/bin/logchecker.rb"

p = re.compile('[^\d]*(\d+)-(\d+)-(\d+)r(\d+).hist')

outputs_li = []
outputs_vi = []

thread_re = re.compile('Threads: (\d+)')
evt_re = re.compile('Events: (\d+)')
adt_re = re.compile('ADT: ([a-z]+)')
lin_re = re.compile('OK')
simpl_re = re.compile('Simplification: (\d+)')
lintime_re = re.compile('Linearization: (\d+)')


TIMEOUT = 60.0

violin_lin = re.compile('VIOLATION:\s+false')

def limousine(fn):
    start = time.perf_counter()
    try:
        res = sp.run(["./bin/linearize", fn], stdout=sp.PIPE, timeout=TIMEOUT)
    except sp.TimeoutExpired:
        #print("Timeout reached")
        return None, None
    stop = time.perf_counter()
    outputs_li.append((fn, str(res.stdout)))
    output = res.stdout.decode('utf-8')

    lin = True if len(lin_re.findall(output)) > 0 else False
    return stop - start, lin

def violin(fn):
    start = time.perf_counter()
    try:
        res = sp.run([PATH_TO_VIOLIN, "-a", "saturate", "-i", "-r", fn], stdout=sp.PIPE, stderr=sp.PIPE, timeout=TIMEOUT)
    except sp.TimeoutExpired:
        return None, None
    stop = time.perf_counter()
    output = res.stdout.decode('utf-8')
    outputs_vi.append((fn, output))

    lin_res = True if len(violin_lin.findall(output)) else False

    return stop - start, lin_res

repl_re = re.compile('\\?<\d+,\d+>')
find_re = re.compile('.*<(\d+),(\d+)>')
thr_re = re.compile('\[(\d+)\]')

# def prepare_violin_file(fn, th, pushes, pops, rep):
#     op_idx = [0 for i in range(th)]
#     op_mode = [0 for i in range(th)]
#     print(fn)
#     with open(fn) as f:
#         contents = f.readlines()
#     print("converting")
#     for i in range(len(contents)):
#         m = find_re.match(contents[i])
#         if m:
#             contents[i] = re.sub(repl_re, f'{(pushes * int(m[1])) + int(m[2])}', contents[i], count=1)

#         m = thr_re.match(contents[i])
#         if m:
#             t = int(m[1])
#             contents[i] = re.sub(thr_re, f'[{(pushes + pops) * t + op_idx[t]}]', contents[i], count=1)
#             op_mode[t] += 1
#             if op_mode[t] == 2:
#                 op_mode[t] = 0
#                 op_idx[t] += 1
#     with open('tmp.hist', 'w') as o:
#         o.write("".join(contents))
#     return "tmp.hist"

path = sys.argv[1] + "/**"

violin_res = dict({})
limousine_res = dict({})

mismatches = []

fn_re = re.compile('\w+(\d+)-(\d+)r(\d+)')

def parse_file(fn):
    full_path = Path(fn)
    basename = full_path.name

    rm = fn_re.match(basename)
    return Test(fn, rm[1], rm[2])


tests = []

for fn in tqfor(list(glob.glob(path, recursive=True))):
    if not os.path.isfile(fn):
        continue
    if os.path.isdir(fn):
        continue

    # Create test
    tests.append(parse_file(fn))




fn = sys.argv[2]

with open(fn, "w") as f:
    f.write(json.dumps(contents))

separator = "-" * 40 + "\n"

with open("mismatches.txt", "w") as f:
    for fn in mismatches:
        f.write(fn)

with open("outputs_violin.txt", "w") as f:
    for (fn, line) in outputs_vi:
        f.write(separator)
        f.write(fn + "\n")
        f.write(separator)
        f.write(line)


with open("outputs_limousine.txt", "w") as f:
    for (fn, line) in outputs_li:
        f.write(separator)
        f.write(fn + "\n")
        f.write(separator)
        f.write(line)
