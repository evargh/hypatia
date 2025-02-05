from collections import Counter
import json
import csv
import argparse
import itertools

props = []

parser = argparse.ArgumentParser()
parser.add_argument('fstate_dir')
parser.add_argument('num_satellites')
parser.add_argument('time_index')
parser.add_argument('time_increment')
args = parser.parse_args()

time_index = int(args.time_index)
time_increment = int(args.time_increment)
num_satellites = int(args.num_satellites)
next_step = {}

for tid in range(0, time_index+time_increment, time_increment): 
    # paths_dict: set of ground station keypairs
    paths_dict = {i: [] for i in itertools.product(list(range(num_satellites,100+num_satellites)), list(range(num_satellites, 100+num_satellites))) if (i[0] != i[1])}
    with open(f'{args.fstate_dir}/fstate_{tid}.txt') as ifile:
        steps = csv.reader(ifile, delimiter=',')
        # write (or overwrite) the next step
        for step in steps:
            next_step[(int(step[0]),int(step[1]))] = int(step[2])

    for (i,j) in paths_dict.keys():
        satellite = next_step[(i,j)]
        while satellite != j:
            paths_dict[(i,j)].append(satellite)
            satellite = next_step[(satellite,j)]
    
    with open(f"paths_{tid}.json", 'w') as f:
        paths_dictx = {str(i) : paths_dict[i] for i in paths_dict.keys()}
        json.dump(paths_dictx, f)

