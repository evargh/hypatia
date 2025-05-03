"""
    python3 plot_isl_utilization_distribution.py --timestamp 120 \
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
import csv


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


def generate_isl_means_over_1s(filename, ts):
    timestep = ts * 1_000_000_000
    isl_list = {}
    with open(filename) as isl_file:
        isls_at_times = csv.reader(isl_file, delimiter=",")
        # the ISL file only adds a new entry when the utilization changes
        # as a result, the file needs to be traversed
        for isl in isls_at_times:
            # if the isl is not in the list of isls, initialize it
            if f"{isl[0]},{isl[1]}" not in isl_list:
                isl_list[f"{isl[0]},{isl[1]}"] = [(-1, -1)] * 10
            # set the latest value we see (before the timestep) to be the beginning value
            if (
                int(isl[2]) <= timestep
                and int(isl[2]) >= isl_list[f"{isl[0]},{isl[1]}"][0][0]
            ):
                isl_list[f"{isl[0]},{isl[1]}"][0] = (int(isl[2]), float(isl[4]))
            # if the current time is within a second of the timestep, set that index appropriately
            if int(isl[2]) > timestep and int(isl[2]) <= timestep + 900_000_000:
                isl_list[f"{isl[0]},{isl[1]}"][
                    int((int(isl[2]) - timestep) / 100_000_000)
                ] = (int(isl[2]), float(isl[4]))

    # fill in any missing values
    for k in isl_list.keys():
        for item in isl_list[k]:
            for i in range(1, 10):
                if isl_list[k][i] == (-1, -1):
                    isl_list[k][i] = isl_list[k][i - 1]

    # take an arithmetic mean of all utilizations
    isl_utilizations_arithmetic_mean_over_1s = {
        k: sum([i[1] for i in isl_list[k]]) / 10 for k in isl_list.keys()
    }
    num_unused = [
        1
        for k in isl_utilizations_arithmetic_mean_over_1s.keys()
        if isl_utilizations_arithmetic_mean_over_1s[k] == 0
    ]
    num_used = [
        1
        for k in isl_utilizations_arithmetic_mean_over_1s.keys()
        if isl_utilizations_arithmetic_mean_over_1s[k] == 1
    ]
    x = np.sort(list(isl_utilizations_arithmetic_mean_over_1s.values()))
    y = np.arange(len(x)) / float(len(x))
    print(filename)
    print(x[int(len(x) * 0.9)])
    print(x[int(len(x) * 0.95)])
    print(x[int(len(x) * 0.99)])
    print(x[int(len(x) * 0.999)])
    print(len(num_unused))
    return x, y


datasets = [
    generate_isl_means_over_1s(filename, args.timestamp) for filename in args.file_list
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


ax.set_title(f"CDF of ISL Utilization at {args.timestamp}s")
ax.set_xlabel("% ISL Bandwidth Used")
# ax.set_yscale("log")
# ax.set_ylim(0, 60)
ax.set_ylim(0.9, 1.03)
ax.legend()
plt.savefig("test.png")
