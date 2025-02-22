import csv
import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument("fstate_dir")
parser.add_argument("num_satellites")
parser.add_argument("time_index")
parser.add_argument("time_increment")
args = parser.parse_args()

num_satellites = int(args.num_satellites)

time_index = int(args.time_index)
time_increment = int(args.time_increment)
gs_pairs = set()

next_step = {}

for tid in range(0, time_index + time_increment, time_increment):
    print(f"on {tid}")
    with open(f"{args.fstate_dir}/fstate_{tid}.txt") as ifile:
        with open(
            f"{args.fstate_dir}/truncated_dir/fstate_{tid}_truncated.txt", "w+"
        ) as ofile:
            fstate_entries = csv.reader(ifile, delimiter=",")
            for fstate_entry in fstate_entries:
                src_node = int(fstate_entry[0])
                dest_node = int(fstate_entry[1])
                next_hop = int(fstate_entry[2])

                if src_node >= num_satellites:
                    ofile.write(",".join(fstate_entry) + "\n")
                # if already in gs_pairs, that must mean the path has changed, and the destination ground station is no longer reachable by that satellite
                elif dest_node != next_hop and (src_node, dest_node) in gs_pairs:
                    gs_pairs.remove((src_node, dest_node))
                    ofile.write(f"{src_node},{dest_node},-1,-1,-1,0\n")
                # if we do not have this gs_pair, that means it's a new direct connection from a satellite to a ground station
                elif dest_node == next_hop and (src_node, dest_node) not in gs_pairs:
                    gs_pairs.add((src_node, dest_node))
                    ofile.write(",".join(fstate_entry) + "\n")
