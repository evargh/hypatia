import json
import csv
import argparse
import itertools

props = []

parser = argparse.ArgumentParser()
parser.add_argument("fstate_dir")
parser.add_argument("num_satellites")
parser.add_argument("final_time_step")
parser.add_argument("time_increment")
args = parser.parse_args()

time_index = int(args.final_time_step)
time_increment = int(args.time_increment)
num_satellites = int(args.num_satellites)
# next_step: dictionary that stores all the next hops to a destination
# must be stored separately, because not all paths are updated with each timestep
next_step = {}

for tid in range(0, time_index + time_increment, time_increment):
    # paths_dict: set of ground station keypairs to be mapped to a list of satellites representing a path
    paths_dict = {
        i: []
        for i in itertools.product(
            list(range(num_satellites, 100 + num_satellites)),
            list(range(num_satellites, 100 + num_satellites)),
        )
        if (i[0] != i[1])
    }

    with open(f"{args.fstate_dir}/fstate_{tid}.txt") as ifile:
        fstate_entries = csv.reader(ifile, delimiter=",")
        # write (or overwrite) the next step
        for fstate_entry in fstate_entries:
            src_node = int(fstate_entry[0])
            dest_node = int(fstate_entry[1])
            next_hop = int(fstate_entry[2])
            next_step[(src_node, dest_node)] = next_hop

    for src_gs, dest_gs in paths_dict.keys():
        satellite = next_step[(src_gs, dest_gs)]
        while satellite != dest_gs:
            paths_dict[(src_gs, dest_gs)].append(satellite)
            satellite = next_step[(satellite, dest_gs)]

    with open(f"paths_{tid}.json", "w") as f:
        paths_dict_stringified = {str(i): paths_dict[i] for i in paths_dict.keys()}
        json.dump(paths_dict_stringified, f)
