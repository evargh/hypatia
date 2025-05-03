import json
import matplotlib.pyplot as plt
import argparse
import numpy as np
import csv
import os


linestyle_converter = {
    "unbroken": "-",
    "dashed": "--",
    "dotdash": "-.",
    "dotted": ":",
    "longdash": (5, (10, 3)),
}

# Create an ArgumentParser object
parser = argparse.ArgumentParser()
parser.add_argument("--satellite", type=str)
parser.add_argument("--algorithm-name", type=str)
parser.add_argument("--queue-timeline-folder", type=str)
# Parse the command-line arguments
args = parser.parse_args()

# fix the absence of interfaces

fig, ax = plt.subplots(4, 1, sharex=True)
with open(
    f"{args.queue_timeline_folder}/{args.satellite}_queue_timeline.json",
    "r",
) as input_file:
    queue_data = input_file.read()
    queue = json.loads(queue_data)
    i = 0
    for k in queue.keys():
        times = [float(i[0][1:-1]) for i in queue[k]]
        qsizes = [int(i[1]) for i in queue[k]]
        ax[i].scatter(
            times,
            qsizes,
            label=f"Queue Sizes for Satellite {args.satellite} on Interface {k}",
            s=1,
        )
        # ax[i].set_ylim(-1, 2000)
        i += 1

fig.suptitle(
    f"Queue Fullness for Satellite {args.satellite} \n in the Starlink Constellation with {args.algorithm_name} Routing"
)
fig.set_size_inches(10, 6)
fig.text(5, 0.04, "Shared X Label", ha="center")
fig.text(0.04, 0.5, "Shared Y Label", va="center", rotation="vertical")

# ax.set_xlim(4,4.1)
plt.savefig("test.png")
