#!/bin/env python3
import sys
import json
import matplotlib.pyplot as plt
import matplotlib as mpl
from collections import defaultdict as dd
import statistics as st

def mean(vals):
    return sum(vals) / len(vals)

if __name__=="__main__":
    if len(sys.argv) < 2:
        print(f'Usage: {sys.argv[0]} [input file]')
        exit(0)


    fn = sys.argv[1]
    data = {}
    results = dd(lambda: dd(lambda : dd(lambda: [])))
    with open(fn, "r") as f:
        data = json.loads(f.read())
        for adt, res1 in data.items():
            for tool,res2 in res1.items():
                for thrs, res3 in res2.items():
                    for vals, times in res3.items():
                        results[adt][tool][int(vals)].extend([t for t in times if t is not None])

    # mpl.use("pgf")
    # mpl.rcParams.update({
    #     "pgf.texsystem": "pdflatex",
    #     'font.family': 'serif',
    #     'text.usetex': True,
    #     'pgf.rcfonts': False,
    # })


    fig = plt.figure(figsize=(5,1.5), layout='constrained')
    keys = list(results.keys())
    print(f"{len(keys)} benchmarks")
    axes = { keys[i] : fig.add_subplot(1 + int(len(keys) / 2) ,2,1+i) for i in range(len(keys)) }
    print(axes)

    for name, ax in axes.items():
        ax.set_xlabel("Events")
        ax.set_ylabel("Normalized time")
        ax.set_title(name.replace("_", " "))

    # ax.xaxis.label = "Operations"
    # ax.yaxis.label = "Relative time increase"

    # axes : threads -> (vals -> times)
    # threads -> (vals * results)

    # for cfg,res in data.items():
    #     l = cfg.split('-')
    #     thrs = int(l[0])
    #     vals = int(l[1])

    #     results[thrs].append((vals, res))

    for name, res in results.items():
        for alg,res2 in res.items():
            clr = "blue" if alg == "limousine" else "brown"
            xaxis = sorted([r for r,v in res2.items() if len(v) > 0])
            yaxis = [res2[k] for k in xaxis]
            if len(yaxis) == 0:
                continue
            print(yaxis)
            ymean = list(map(st.mean, yaxis))
            yrel = list(map(lambda x: x / ymean[0], ymean))
            ymin = list(map(min, yaxis))
            ymax = list(map(max, yaxis))

            ymin_rel = list(map(lambda x: x / ymean[0], ymin))
            ymax_rel = list(map(lambda x: x / ymean[0], ymax))

            axes[name].plot(xaxis, yrel, color=clr)
            axes[name].fill_between(xaxis, ymin_rel, ymax_rel, color=mpl.colors.to_rgba(clr, 0.15))
        #plt.errorbar(xaxis, ymean, yerr=ystddev, fmt='-o', color='red' if alg=="limousine" else 'blue')




    # plt.savefig(f"plot_output.pgf")
    plt.show()
