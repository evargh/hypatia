"""
    python3 plot_ingress_traffic_per_node.py --timestamp 120 \
    --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" \
    --file-list \
    <path to the snapshot ingress> \
    <path to the hbm ingress> \
    <path to the dhbp ingress> \
    <path to the elb ingress> \
    --line-style-list unbroken dashed dotdash dotted \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
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


def get_ingress_traffic_cdf(filename, time):
    with open(filename) as f:
        # multiply with 10, since the ingress is calculated 10 times per second
        counts = json.load(f)[str(time * 10)]
        zero_filler = [0] * (1584 - len(counts.values()))
        plotter = list(counts.values()) + zero_filler

        x = np.sort(plotter)
        y = np.arange(len(x)) / float(len(x))

    return x, y


datasets = [
    get_ingress_traffic_cdf(filename, args.timestamp) for filename in args.file_list
]
fig, ax = plt.subplots(1, 1)

fig.set_size_inches(10, 5)

plots = [
    ax.plot(
        datasets[i][0],
        datasets[i][1],
        label=args.name_list[i],
        linestyle=linestyle_converter[args.line_style_list[i]],
        color=args.color_list[i],
    )
    for i in range(len(datasets))
]


ax.set_title(f"CDF of Ingress Traffic per Node at {args.timestamp} s")
# ax.set_title(f"Average Time for Flow to Finish in Starlink (excluding flows that were dropped before {time_threshold}s)")
ax.set_xlabel("Bytes Received")
# ax.set_yscale("log")
# ax.set_ylim(0, 60)
ax.set_ylim(0.8, 1.03)
ax.legend()
plt.savefig("node_utilization_distribution.png")


vals = [100 * sum([1 for k in i[0] if k != 0]) / 1584 for i in datasets]
print(vals)
bar_colors = ["tab:red", "tab:blue", "tab:blue", "tab:blue"]

fig, ax = plt.subplots(1, 1)
fig.set_size_inches(10, 6)
bars = ax.bar(args.name_list, vals, color=bar_colors)
# ax.set_xticklabels(rotation=45, ha='right')
ax.set_ylabel("% Satellites in Constellation Used")
ax.set_title(
    f"% of Nodes Used by Different Routing Algorithms for 30 Mbps Global Traffic at {args.timestamp} s"
)

# Manually add labels
for bar in bars:
    height = bar.get_height()
    ax.text(
        bar.get_x() + bar.get_width() / 2,
        height,
        f"{round(height, 2)}%",
        ha="center",
        va="bottom",
    )

plt.savefig("percentage_utilized_nodes.png")
