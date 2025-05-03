"""
    python3 plot_flow_distributions.py --timestamp 80 \
    --name-list "Modified DHBP" "ELB over HBM" "INNER n=2" \
    --file-list \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    <path to the inner flow aggregate times> \
    --line-style-list dotdash dotted longdash \
    --color-list \#2ca02c \#d62728 \#9467bd
"""

import json
import matplotlib.pyplot as plt
import argparse
import numpy as np


linestyle_converter = {
    "unbroken": "-",
    "dashed": "--",
    "dotdash": "-.",
    "dotted": ":",
    "longdash": (5, (10, 3)),
}

# Create an ArgumentParser object
parser = argparse.ArgumentParser()
parser.add_argument("--name-list", nargs="+", type=str)
parser.add_argument("--file-list", nargs="+", type=str)
parser.add_argument("--line-style-list", nargs="+", type=str)
parser.add_argument("--color-list", nargs="+", type=str)
parser.add_argument("--timestamp", type=int)
# Parse the command-line arguments
args = parser.parse_args()


def get_flow_time_cdf(filename, time):
    with open(filename) as f:
        base_data = json.load(f)
        this_timestamp = []
        for j in base_data.keys():
            if base_data[j][time] != -1:
                this_timestamp.append(base_data[j][time])
            # else:
            #    this_timestamp.append(200000000000)
        this_timestamp.sort()
        this_timestamp = [x / 1000000000 for x in this_timestamp]

    return this_timestamp


datasets = [get_flow_time_cdf(filename, args.timestamp) for filename in args.file_list]
totalpoints = np.arange(230) / 230.0
yaxes = [[totalpoints[i] for i in range(len(dataset))] for dataset in datasets]
fig, ax = plt.subplots(1, 1)

fig.set_size_inches(10, 5)

# print(snapshot_30mbps[120]/dhbp_30mbps[120])

plots = [
    ax.plot(
        datasets[i],
        yaxes[i],
        label=args.name_list[i],
        linestyle=linestyle_converter[args.line_style_list[i]],
        color=args.color_list[i],
    )
    for i in range(len(datasets))
]


ax.set_title(
    f"Flow Finish Time Tail CDF at {args.timestamp} s"
)  # , Relative to New Algorithm")
# ax.set_title(f"Average Time for Flow to Finish in Starlink (excluding flows that were dropped before {time_threshold}s)")
ax.set_xlabel("Flow Finish Time (s)")
# ax.set_yscale("log")
# ax.set_ylim(0, 60)
ax.set_ylim(0.8, 1.03)
ax.legend()
plt.savefig("test.png")
