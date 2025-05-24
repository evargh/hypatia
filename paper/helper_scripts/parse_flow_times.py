import argparse
import json
import csv


# read the csv file
# parse the start and end times of each finished flow, and append it to a list of times for that flow
# with this data, can both plot the increase over time, and the averages
parser = argparse.ArgumentParser()
parser.add_argument("flow_file")
args = parser.parse_args()
flow_dict = {}

with open(f"{args.flow_file}") as ifile:
    flows = csv.reader(ifile, delimiter=",")
    for flow in flows:
        if (flow[1], flow[2]) not in set(flow_dict.keys()):
            flow_dict[(flow[1], flow[2])] = []
        if flow[8] == "YES":
            flow_dict[(flow[1], flow[2])].append(int(flow[5]) - int(flow[4]))
        elif flow[8] == "NO_ONGOING":
            flow_dict[(flow[1], flow[2])].append(-1)
        else:
            flow_dict[(flow[1], flow[2])].append(-1)
            print(flow[8])


with open("flow_aggregate_times.json", "w+") as ofile:
    stringified_flow_dict = {f"{i}": flow_dict[i] for i in flow_dict.keys()}
    json.dump(stringified_flow_dict, ofile)
