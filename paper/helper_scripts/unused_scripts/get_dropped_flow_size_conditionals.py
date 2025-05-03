import argparse
import csv
import numpy as np


# read the csv file
# parse the start and end times of each finished flow, and append it to a list of times for that flow
# with this data, can both plot the increase over time, and the averages
parser = argparse.ArgumentParser()
parser.add_argument("flow_file")
args = parser.parse_args()
not_dropped_flows = []
dropped_flows = []

size_of_dropped = []  # correlation between size and drops
all_size = []  # data for average size

with open(f"{args.flow_file}") as ifile:
    flows = csv.reader(ifile, delimiter=",")
    for flow in flows:
        if int(flow[4]) < 190000000000:
            # look at the appropriate fstate files, and average the distance
            # find the median flow distance for that timestep
            all_size.append(int(flow[3]))
            if flow[8] != "YES":
                size_of_dropped.append(int(flow[3]))

median = sorted(all_size)[int(len(all_size) * 0.9)]

over_median = [i for i in all_size if i > median]
over_median_and_dropped = [i for i in size_of_dropped if i > median]

# probability of dropped given over median = probability of dropped and over median/probability of over median
# probability of over median given dropped = probability of dropped and over median/probability of dropped
print(len(over_median_and_dropped) / len(size_of_dropped))
