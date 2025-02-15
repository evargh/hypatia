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
            steps = csv.reader(ifile, delimiter=",")
            for step in steps:
                if int(step[0]) >= num_satellites:
                    ofile.write(",".join(step) + "\n")
                elif (int(step[0]), int(step[1])) in gs_pairs:
                    gs_pairs.remove((int(step[0]), int(step[1])))
                    ofile.write(f"{step[0]},{step[1]},-1,-1,-1,0\n")
                elif int(step[1]) == int(step[2]):
                    gs_pairs.add((int(step[0]), int(step[1])))
                    ofile.write(",".join(step) + "\n")
