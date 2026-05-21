#!/usr/bin/env python3
import sys
import json
import matplotlib.pyplot as plt
import matplotlib as mpl
from collections import defaultdict as dd
import statistics as st


with open('register_bench.json', 'r') as f:
    raw_data = json.load(f)

parsed_data = dd(lambda: dd(list))

for key, times in raw_data.items():
    threads_str, ops_str = key.split('-')
    threads = int(threads_str)
    ops = int(ops_str)
    parsed_data[threads][ops].extend(times)

fig, ax = plt.subplots(figsize=(3.5, 2.5), layout='constrained')

target_threads = [8, 16, 32]


# fig = plt.figure(figsize=(5.5,2.5), layout='constrained')
# impls = list(data.keys())
# axes = { impls[i] : fig.add_subplot(1 + int(len(impls) / 3), 3, 1+ i) for i in range(len(impls)) }

for thrs in target_threads:

    xaxis = sorted(parsed_data[thrs].keys())
    yaxis = [st.mean(parsed_data[thrs][ops]) for ops in xaxis]
    
    # blue_intensity = 0.1 + 0.8 * thrs * (1.0 / 64.0)
    green_intensity = min((thrs + 10) * (1.0 / 74.0), 1.0)
    clr = (0.0, green_intensity, 0.5, 1.0)     
    ax.plot(xaxis, yaxis, color=clr, linewidth=1.0, marker='o', markersize=3, label=f'{thrs}t')

ax.set_ylabel("time (s)")
ax.set_xlabel("history size")

fig.get_layout_engine().set(rect=(0.0, 0.0, 1.0, 0.85))
leg = ax.legend(
    handletextpad=0.5, 
    ncols=3, 
    loc='upper center', 
    bbox_to_anchor=(0.5, 1.25),
    frameon=False
)
leg.set_in_layout(False)

plt.savefig("register.png", dpi=300)
