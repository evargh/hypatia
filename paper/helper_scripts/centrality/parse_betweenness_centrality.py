import json
import csv
import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument("fstate_dir")
parser.add_argument("traffic_file")
parser.add_argument("num_satellites")
parser.add_argument("final_time_step")
parser.add_argument("time_increment")
args = parser.parse_args()

time_index = int(args.final_time_step)
time_increment = int(args.time_increment)
num_satellites = int(args.num_satellites)
next_step = {}

weight_dict = {}
with open(args.traffic_file, "r") as f:
    for line in f:
        row = line.split(",")
        src_gs_id = int(row[0]) + num_satellites
        dest_gs_id = int(row[1]) + num_satellites
        traffic_amount_bytes = int(row[2])
        if (
            src_gs_id,
            dest_gs_id,
        ) not in weight_dict:
            weight_dict[src_gs_id, dest_gs_id] = traffic_amount_bytes

for tid in range(0, time_index + time_increment, time_increment):
    # the betweenness maps satellite to expected traffic
    between_dict = {i: 0 for i in range(num_satellites)}

    with open(f"{args.fstate_dir}/fstate_{tid}.txt") as ifile:
        fstate_entries = csv.reader(ifile, delimiter=",")
        # write (or overwrite) the next step
        for fstate_entry in fstate_entries:
            src_node = int(fstate_entry[0])
            dest_node = int(fstate_entry[1])
            next_hop = int(fstate_entry[2])
            next_step[(src_node, dest_node)] = next_hop

    for src_gs, dest_gs in weight_dict.keys():
        satellite = next_step[(src_gs, dest_gs)]
        while satellite != dest_gs:
            between_dict[satellite] += weight_dict[(src_gs, dest_gs)]
            satellite = next_step[(satellite, dest_gs)]

    with open(f"betweenness_centrality_{tid}.json", "w") as f:
        json.dump(between_dict, f)
