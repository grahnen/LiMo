#!/usr/bin/env python3

#!/usr/bin/env python3
import sys
import json
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

res_regex = re.compile('.*/(\w+)/[a-z]+(\d+)-(\d+)r(\d+)\.hist,(\d+\.\d+)')

def parse_result(res):
    m = res_regex.match(res)
    if not m:
        print(f"Unmatched: {res}")
        return None
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
        if res[0] != "HWQueue" and res[1] < 50 and res[2] < 1000:
            save_result('LiMo', *res)

with open('violin_result.csv', 'r') as f:
    for l in f.readlines():
        res = parse_result(l)
        if res[0] != "HWQueue" and res[1] < 50 and res[2] < 1000:
            save_result('Violin', *res)


fig = plt.figure(figsize=(5.5,2.5), layout='constrained')
impls = list(data.keys())
axes = { impls[i] : fig.add_subplot(1 + int(len(impls) / 3), 3, 1+ i) for i in range(len(impls)) }

for name, ax in axes.items():
    # ax.set_xlabel('{\\tiny Events}')
    # ax.set_ylabel("{\\tiny Normalized time}")
    ax.set_title(name.replace("_", " "))


handles = []

for impl, res in data.items():
    for alg, res2 in res.items():
        for thrs, res3 in res2.items():
            xaxis = sorted([r for r,v in res3.items() if len(v) > 9])
            yaxis = [res3[k] for k in xaxis]
            if len(yaxis) == 0:
                continue

            ymean = list(map(st.mean, yaxis))
            yrel = list(map(lambda x: x / ymean[0], ymean))

            c_a = 0.6 if alg == 'LiMo' else 0.0
            c_b = 0.0 if alg == 'LiMo' else 0.6
            c_c = 0.1 + 0.8 * thrs * (1.0/64.0)
            clr = (c_a, c_b, c_c, 1.0)
            p = axes[impl].plot(xaxis, yrel, linewidth=0.5, color=clr, label=f'{alg} {thrs}t')
            handles.extend(p)

print(handles)

uniq = [
    "LiMo 8t", "Violin 8t",
    "LiMo 16t", "Violin 16t",
    "LiMo 32t", "Violin 32t",
]

handles, labels = axes['MSQueue'].get_legend_handles_labels()
print(labels)


order = [labels.index(u) for u in uniq]

h = [handles[i] for i in order]

fig.get_layout_engine().set(rect=(0.0,0.1, 1.0, 0.75))
leg = fig.legend(handletextpad=0.1, handles=h, labels=uniq, ncols=3, loc='upper center', bbox_to_anchor=(0.5, 0.2))
leg.set_in_layout(False)

plt.savefig(f"small.pgf")
