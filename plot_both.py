#!/usr/bin/env python3
import sys
import json
import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib as mpl
from collections import defaultdict as dd
import statistics as st
import re

mpl.use("pgf")
mpl.rcParams.update({
    "pgf.texsystem": "pdflatex",
    'font.family': 'serif',
    'text.usetex': True,
    'pgf.rcfonts': False,
})

#res_regex = re.compile('.*/(\w+)/[a-z]+(\d+)-(\d+)r(\d+)\.hist,(\d+\.\d+)')
res_regex = re.compile('.+\t.+\t.+\t(.+)\t.+\t.+\t0\t.+\t.*/(\w+)/[a-z]+(\d+)-(\d+)r(\d+).*\.hist')
def parse_result(res):
    m = res_regex.match(res)
    if m is None:
        return None
    time = float(m[1])
    impl = m[2]
    threads = int(m[3])
    ops = int(m[4])
    rep = int(m[5])
    return impl, threads, ops, time

data = dd(lambda: dd( lambda: dd(lambda: dd(lambda: [])) ))

def save_result(alg, impl, threads, ops, time):
    data[impl][alg][threads][ops].append(time)

with open('limo.log', 'r') as f:
    for l in f.readlines():
        res = parse_result(l)
        if res is not None:
            save_result('LiMo', *res)

with open('violin.log', 'r') as f:
    for l in f.readlines():
        res = parse_result(l)
        if res is not None:
            save_result('Violin', *res)


fig,axs = plt.subplots(2,4, figsize=(5, 3), layout='constrained')

impls = list(data.keys()) + list(map(lambda x: x + "_small", data.keys()))
print(impls)

cols = 4
rows = 2
axes = { impls[i] : axs[int(i / cols)][i%cols] for i in range(len(impls)) }

for name, ax in axes.items():
    if "small" not in name:
        ax.set_title(name.replace("_", " "), fontsize=9)
    else:
        ax.set_title("")

handles = []

for impl, res in data.items():
    for alg, res2 in res.items():
        for thrs, res3 in res2.items():
            xaxis = sorted([r for r,v in res3.items() if len(v) > 0])
            xaxis2 = sorted([r for r,v in res3.items() if len(v) > 0 and r < 1000])
            yaxis = [res3[k] for k in xaxis]
            yaxis2 = [res3[k] for k in xaxis2]

            if len(yaxis) != len(xaxis):
                continue


            c_a = 0.6 if alg == 'LiMo' else 0.0
            c_b = 0.0 if alg == 'LiMo' else 0.6
            c_c = 0.1 + 0.8 * thrs * (1.0/16.0)
            clr = (c_a, c_b, c_c, 1.0)
            ymean = list(map(st.mean, yaxis))
            ymean2 = list(map(st.mean, yaxis2))
            yrel = list(map(lambda x: x / ymean[0], ymean))
            yrel2 = list(map(lambda x: x / ymean2[0], ymean2))

            # clr = "blue" if alg == "limousine" else "brown"
            p = axes[impl].plot(xaxis, yrel, color=clr, label=f'{alg} {thrs}t')
            p2 = axes[impl + '_small'].plot(xaxis2, yrel2, color=clr, label=f'{alg} {thrs}')
            handles.extend(p)


#uniq = list(set([h._label for h in handles]))
# Hard-code...

uniq = [
    "LiMo 4t", "Violin 4t",
    "LiMo 8t", "Violin 8t",
    "LiMo 12t", "Violin 12t",
    "LiMo 16t", "Violin 16t"
]

handles, labels = axes['MSQueue'].get_legend_handles_labels()

order = [labels.index(u) for u in uniq]

h = [handles[i] for i in order]

fig.get_layout_engine().set(rect=(0.0,0.25, 1.0, 0.75))
leg = fig.legend(handles=h, labels=uniq, ncols=4, loc='upper center', bbox_to_anchor=(0.5, 0.2))
leg.set_in_layout(False)
#fig.tight_layout()
plt.savefig(f"combined.pgf")
