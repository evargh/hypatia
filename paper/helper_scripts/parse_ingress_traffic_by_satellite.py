from collections import Counter
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('filename')
parser.add_argument('paths_dir')
args = parser.parse_args()

time_index = {i: {} for i in range(0, 2000)}
linecount = 0
# open the list of paths at time 0
# read the input file for as long as the timesteps are relevant to that list of paths
# when done, close the file, restart the process with the next set of files
# the file is too large to completely load into memory
# since files behave as iterators in python, we probably dont have to re-open the file for each path file
for i in time_index.keys():
    with open(f"{args.paths_dir}/paths_{i*100000000}.json") as checkfile:
        pathset = json.load(checkfile)
        with open(args.filename) as ifile:
            for line in ifile:
                inp = line.split(" ")
                if len(inp) > 1:
                    if inp[1] == "PointToPointLaserNetDevice:Receive():":
                        time_step = int(float(inp[0][1:-1])*10)
                        if time_step == i:
                            if int(inp[12]) in pathset[f"({int(inp[3])}, {int(inp[6])})"]:
                                if int(inp[12]) not in time_index[time_step]:
                                    time_index[time_step][int(inp[12])] = 0
                                time_index[time_step][int(inp[12])] += int(inp[-1])
                        if time_step > i:
                            break

#with open(args.filename) as ifile:
#    for line in ifile:
#        inp = line.split(" ")
#        if len(inp) > 1:
#            if inp[1] == "PointToPointLaserNetDevice:Receive():":
#                time_step = int(float(inp[0][1:-1])*10)
#                # this takes forever. make it so that 
#                with open(f"{args.paths_dir}/paths_{time_step*100000000}.json") as checkfile:
#                    dataset = json.load(checkfile)
#                    if inp[12] in dataset[f"({int(inp[3])}, {int(inp[6])})"]:
#                        if int(inp[12]) not in time_index[time_step]:
#                            time_index[time_step][int(inp[12])]= 0
#                        time_index[time_step][int(inp[12])] += int(inp[-1])

print(json.dumps(time_index))


