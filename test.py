#!/bin/env python3
import glob
import sys
import argparse
try:
    from tqdm import tqdm
except ImportError:
    from extra import tqdm


from subprocess import Popen, PIPE

prog = "./bin/linearize"

history_root = "./histories/simple/"


def args(fn, alg, verbose):
    if verbose:
        return (prog, fn, "-a", alg, "-v")
    return (prog, fn, "-a", alg)

def args_cover(fn, verbose):
    if verbose:
        return (prog, fn, "-v", "-a", "cover")
    return (prog, fn, "-a", "cover")

def test_history(history, alg, result = 0):
    po = Popen(args(f, alg, opts.verbose), stdout=PIPE)
    po.wait()
    output = po.stdout.read()
    s = output.decode('UTF-8').splitlines()[-1]

    lin = s[0] == 'O'
    c_lin = False
    if result == 0:
        po = Popen(args_cover(f, opts.verbose), stdout=PIPE)
        po.wait()
        output = po.stdout.read()
        s = output.decode('UTF-8').splitlines()[-1]
        c_lin = s[0] == 'O'
    elif result == 1:
        c_lin = True

    return lin == c_lin



if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-v", required=False,
                        action='store_const',
                        default=False,
                        const=True,
                        dest='verbose')
    parser.add_argument("-d", required=False,
                        action='store',
                        default='sqt',
                        dest='tests')
    parser.add_argument("-a", required=True,
                        action='store',
                        default='segment',
                        dest='alg')
    
    opts = parser.parse_args()
    v = opts.verbose
    tests = opts.tests
    alg = opts.alg

    for t in tests:
        if t == 's':
            print("Set")
            hr = history_root + "set/"
        elif t == 'q':
            print("Queue")
            hr = history_root + "queue/"
        elif t == 't':
            print("Stack")
            hr = history_root + "stack/"
        else:
            print("Unknown data structure: ", t)
            continue


        print("Testing Violations")
        for f in tqdm(glob.glob(hr + "violation/*.hist", recursive=False)):
            if not test_history(f, alg, -1):
                print("Accepting violating history: ", f)
                
        print("Testing safe")
        for f in tqdm(glob.glob(hr + "safe/*.hist", recursive=False)):
            if not test_history(f, alg, 1):
                print("Rejecting valid history: ", f)

        print("Testing previous mismatches")
        for f in tqdm(glob.glob(hr + "mismatches/*.hist", recursive=False)):
            if not test_history(f, alg):
                print("Mismatch on history: ", f)
