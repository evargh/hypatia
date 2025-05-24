import argparse
import csv
import json
import numpy as np


# read the csv file
# parse the start and end times of each finished flow, and append it to a list of times for that flow
# with this data, can both plot the increase over time, and the averages
parser = argparse.ArgumentParser()
parser.add_argument("flow_file")
parser.add_argument("distances_dir")
args = parser.parse_args()

stop_time = 190

distance_of_dropped = np.zeros((stop_time, 230))
all_distances = np.zeros((stop_time, 230))

with open(f"{args.flow_file}") as ifile:
    flows = csv.reader(ifile, delimiter=",")
    for flow in flows:
        # coarse grained measurement, just looking at the instantaneous distance at that second
        if int(flow[4]) < stop_time * 1_000_000_000:
            timestep = int(int(flow[4]) / 1_000_000_000)
            with open(
                f"{args.distances_dir}/gravity_flow_{flow[1]}_to_{flow[2]}.json"
            ) as distance_file:
                distances = json.load(distance_file)
                all_distances[timestep, int(flow[0]) % 230] = distances[timestep * 10]
                if flow[8] != "YES":
                    distance_of_dropped[timestep, int(flow[0]) % 230] = distances[
                        timestep * 10
                    ]

percentile_distance_per_time = [sorted(i)[int(len(i) * 0.5)] for i in all_distances]
num_dropped = [
    sum([1 for idx in distance_of_dropped[i] if idx > 0]) for i in range(stop_time)
]
num_over_respective_percentile = [
    sum([1 for idx in all_distances[i] if idx > percentile_distance_per_time[i]])
    for i in range(stop_time)
]
num_dropped_and_over_respective_percentile = [
    sum([1 for idx in distance_of_dropped[i] if idx > percentile_distance_per_time[i]])
    for i in range(stop_time)
]
average_probability_of_drop_given_over_percentile = [
    num_dropped_and_over_respective_percentile[i] / num_over_respective_percentile[i]
    for i in range(stop_time)
]
average_probability_of_over_percentile_given_drop = [
    num_dropped_and_over_respective_percentile[i] / num_dropped[i]
    if num_dropped[i] != 0
    else 0
    for i in range(stop_time)
]
print(average_probability_of_drop_given_over_percentile)
print(average_probability_of_over_percentile_given_drop)

# over_percentile = [i for i in range(200) if i > percentile_distance_per_time[i]]
# over_median_and_dropped = [i for i in size_of_dropped if i > median]

# probability of dropped given over median = probability of dropped and over median/probability of over median
# probability of over median given dropped = probability of dropped and over median/probability of dropped
# print(len(over_median_and_dropped) / len(size_of_dropped))
