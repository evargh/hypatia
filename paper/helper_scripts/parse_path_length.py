import argparse
import json
import csv

parser = argparse.ArgumentParser()
parser.add_argument("path_dir")
parser.add_argument("fstate_dir")
parser.add_argument("src_gs_id")
parser.add_argument("dest_gs_id")
parser.add_argument("time_increment")
parser.add_argument("final_time_step")

args = parser.parse_args()
time_index = int(args.final_time_step)
time_increment = int(args.time_increment)

next_step = {}
distance_list = []


for tid in range(0, time_index + time_increment, time_increment):
    distance = 0
    hop_list = []
    with open(f"{args.path_dir}/paths_{tid}.json") as path_file:
        all_paths = json.load(path_file)
        hop_list = all_paths[f"({args.src_gs_id}, {args.dest_gs_id})"]

    print(hop_list)
    with open(f"{args.fstate_dir}/fstate_{tid}.txt") as ifile:
        fstate_entries = csv.reader(ifile, delimiter=",")
        # write (or overwrite) the next step
        for fstate_entry in fstate_entries:
            src_node = int(fstate_entry[0])
            dest_node = int(fstate_entry[1])
            next_step[(src_node, dest_node)] = int(fstate_entry[5])

    for i in hop_list:
        distance += next_step[(i, int(args.dest_gs_id))]

    distance_list.append(distance)

print(distance_list)
# for each time
# open the path directory
# parse the path into a set of pair hops
# look at the appropriate fstate file and extract the distance
# if its not in that fstate file, go back a file and extract distance
# sum it all up for the path
