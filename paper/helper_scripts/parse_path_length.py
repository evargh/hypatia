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
    print(tid)

    with open(f"{args.fstate_dir}/truncated_dir/fstate_{tid}_truncated.txt") as ifile:
        fstate_entries = csv.reader(ifile, delimiter=",")
        # write (or overwrite) the next step
        for fstate_entry in fstate_entries:
            src_node = int(fstate_entry[0])
            dest_node = int(fstate_entry[1])
            if src_node == int(args.src_gs_id) and dest_node == int(args.dest_gs_id):
                distance_list.append(int(fstate_entry[5]))
                break

    # the last hop in hop_list will have a distance of zero, so we need to find the opposite direction first hop
    # this assumes that the paths are symmetric i.e. the last hop in one direction is the first hop in the other

with open("gravity_flow.json", "w+") as ofile:
    json.dump(distance_list, ofile)
# for each time
# open the path directory
# parse the path into a set of pair hops
# look at the appropriate fstate file and extract the distance
# if its not in that fstate file, go back a file and extract distance
# sum it all up for the path
