import argparse
from collections import Counter

import json
import csv


# read the csv file
# parse the start and end times of each finished flow, and append it to a list of times for that flow
# with this data, can both plot the increase over time, and the averages
parser = argparse.ArgumentParser()
parser.add_argument("flow_file")
args = parser.parse_args()
not_dropped_flows = {}
dropped_flows = {}

with open(f"{args.flow_file}") as ifile:
    flows = csv.reader(ifile, delimiter=",")
    for flow in flows:
        flow_size = int(flow[3])
        if flow_size not in not_dropped_flows:
            not_dropped_flows[flow_size] = 0
            dropped_flows[flow_size] = 0
        if flow[8] == "YES":
            not_dropped_flows[flow_size] += 1
        elif flow[8] == "NO_ONGOING":
            dropped_flows[flow_size] += 1
        else:
            dropped_flows[flow_size] += 1
            print(flow[8])


dropped_flows_freq = Counter(dropped_flows)
not_dropped_flows_freq = Counter(not_dropped_flows)
with open("dropped_flows_counter.json", "w+") as d:
    json.dump(dropped_flows_freq, d)
with open("all_flows_counter.json", "w+") as a:
    json.dump(not_dropped_flows_freq, a)
