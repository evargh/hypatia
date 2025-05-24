"""
    python3 plot_unfinished_flows.py \
    --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" \
    --file-list <path to the snapshot flow aggregate times> \
    <path to the hbm flow aggregate times> \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    --line-style-list unbroken dashed dotdash dotted \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
"""

import json
import matplotlib.pyplot as plt
import argparse

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
# Parse the command-line arguments
args = parser.parse_args()


def count_drops_over_time(filename):
    drop_hist = [0] * 200
    base_data = {}
    with open(filename) as f:
        base_data = json.load(f)
    for k in base_data.keys():
        for i in range(200):
            if base_data[k][i] == -1:
                drop_hist[i] += 1

    for i in range(200):
        if i != 0:
            drop_hist[i] += drop_hist[i - 1]
    return drop_hist


dataset = [count_drops_over_time(filename) for filename in args.file_list]
fig, ax = plt.subplots(1, 1)

fig.set_size_inches(10, 5)

# print(snapshot_30mbps[120]/dhbp_30mbps[120])

print([linestyle_converter[i] for i in args.line_style_list])
plots = [
    ax.plot(
        range(0, 200),
        dataset[i],
        label=args.name_list[i],
        linestyle=linestyle_converter[args.line_style_list[i]],
        color=args.color_list[i],
    )
    for i in range(len(dataset))
]


ax.set_title(
    "Cumulative Number of Unfinished Flows Over Time"
)  # , Relative to New Algorithm")
# ax.set_title(f"Average Time for Flow to Finish in Starlink (excluding flows that were dropped before {time_threshold}s)")
ax.set_xlabel("Simulation Time (s)")
ax.set_ylabel(
    "Cumulative Number of Unfinished Flows"
)  # (% Relative to New Algorithm)")
# ax.set_yscale("log")
# ax.set_ylim(0, 60)
ax.legend()
plt.savefig("unfinished_flows.png")
