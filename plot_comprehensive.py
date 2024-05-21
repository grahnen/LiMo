#!/usr/bin/env python3
import sys
import json
import matplotlib.pyplot as plt
import matplotlib as mpl
from collections import defaultdict as dd
import statistics as st
import re

res_regex = re.compile('.*/(\w+)/\w+(\d+)-(\d+)r(\d+)\.hist,(\d+\.\d+)')

def parse_result(res):
    m = res_regex.match(res)
    impl = m[1]
    threads = int(m[2])
    ops = int(m[3])
    rep = int(m[4])
    time = float(m[5])
    return impl, threads, ops, time

data = dd(lambda: dd( lambda: dd(lambda: dd(lambda: [])) ))

def save_result(alg, impl, threads, ops, time):
    data[impl][alg][threads][ops].append(time)

with open('limousine_result.csv', 'r') as f:
    for l in f.readlines():
        res = parse_result(l)
        if res[0] != "HWQueue":
            save_result('limousine', *res)

with open('violin_result.csv', 'r') as f:
    for l in f.readlines():
        res = parse_result(l)
        if res[0] != "HWQueue":
            save_result('violin', *res)


fig = plt.figure(figsize=(5,1.5), layout='constrained')
impls = list(data.keys())
axes = { impls[i] : fig.add_subplot(1 + int(len(impls) / 2), 2, 1+ i) for i in range(len(impls)) }

for name, ax in axes.items():
    ax.set_xlabel('Events')
    ax.set_ylabel("Normalized time")
    ax.set_title(name.replace("_", " "))


for impl, res in data.items():
    for alg, res2 in res.items():
        for thrs, res3 in res2.items():
            xaxis = sorted([r for r,v in res3.items() if len(v) > 0])
            yaxis = [res3[k] for k in xaxis]
            if len(yaxis) == 0:
                continue

            ymean = list(map(st.mean, yaxis))
            yrel = list(map(lambda x: x / ymean[0], ymean))

            clr = "blue" if alg == "limousine" else "brown"
            axes[impl].plot(xaxis, yrel, color=clr)

plt.show()
