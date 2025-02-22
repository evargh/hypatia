import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("num_satellites")
args = parser.parse_args()

num_satellites = int(args.num_satellites)
time_index = {i: {} for i in range(0, 2000)}
linecount = 0
# open the list of paths at time 0
# read the input file for as long as the timesteps are relevant to that list of paths
# when done, close the file, restart the process with the next set of files
# the file is too large to completely load into memory
# since files behave as iterators in python, we probably dont have to re-open the file for each path file
with open(args.filename) as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) > 1:
            if inp[1] == "PointToPointLaserNetDevice:Receive():":
                time_step = int(float(inp[0][1:-1]) * 10)
                node_num = int(inp[6])
                if node_num not in time_index[time_step]:
                    time_index[time_step][node_num] = 0
                time_index[time_step][node_num] += int(inp[-1])

            if inp[1] == "GSLNetDevice:Receive():":
                time_step = int(float(inp[0][1:-1]) * 10)
                node_num = int(inp[3])
                # if this is a satellite,
                if node_num < num_satellites:
                    if node_num not in time_index[time_step]:
                        time_index[time_step][node_num] = 0
                    time_index[time_step][node_num] += int(inp[-1])


print(json.dumps(time_index))
