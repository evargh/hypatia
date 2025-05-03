"""
    python3 temporal_flow_latencies.py --percentile 95 \
    --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" "INNER n=2" \
    --file-list <path to the snapshot flow aggregate times> \
    <path to the hbm flow aggregate times> \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    <path to the inner flow aggregate times> \
    --marker-list o v D x s \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728 \#9467bd
"""

import json
import matplotlib.pyplot as plt
import argparse

# Create an ArgumentParser object
parser = argparse.ArgumentParser()
parser.add_argument("--name-list", nargs="+", type=str)
parser.add_argument("--file-list", nargs="+", type=str)
parser.add_argument("--marker-list", nargs="+", type=str)
parser.add_argument("--color-list", nargs="+", type=str)
parser.add_argument("--percentile", type=int)
# Parse the command-line arguments
args = parser.parse_args()


def extract_percentile(filename, percentile):
    return_array = []
    with open(filename) as f:
        base_data = json.load(f)
        for i in range(200):
            this_timestamp = []
            for j in base_data.keys():
                if i >= len(base_data[j]):
                    print(j)
                if base_data[j][i] != -1:
                    this_timestamp.append(base_data[j][i])
                else:
                    this_timestamp.append(200000000000)
            this_timestamp.sort()
            # print(len(this_timestamp))
            # print(int(len(this_timestamp)/2))
            return_array.append(
                this_timestamp[int(len(this_timestamp) * percentile)] / 1000000000
            )
    return return_array


percentile = args.percentile / 100

dataset = [extract_percentile(filename, percentile) for filename in args.file_list]
fig, ax = plt.subplots(1, 1)

fig.set_size_inches(10, 5)

# print(snapshot_30mbps[120]/dhbp_30mbps[120])

plots = [
    ax.scatter(
        range(0, 200),
        dataset[i],
        label=args.name_list[i],
        s=10,
        marker=args.marker_list[i],
        color=args.color_list[i],
    )
    for i in range(len(dataset))
]


ax.set_title(
    f"Time for {percentile * 100}th-Percentile Flows to Complete in Starlink Over Time"
)  # , Relative to New Algorithm")
# ax.set_title(f"Average Time for Flow to Finish in Starlink (excluding flows that were dropped before {time_threshold}s)")
ax.set_xlabel("Flow Start Time (s)")
ax.set_ylabel("Flow Elapsed Time (s)")  # (% Relative to New Algorithm)")
# ax.set_yscale("log")
# ax.set_ylim(0, 60)
ax.set_xlim(-1, 175)
ax.legend()
plt.savefig("test.png")
