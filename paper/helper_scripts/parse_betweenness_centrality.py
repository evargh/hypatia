from collections import Counter
import json
import csv
import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument('fstate_dir')
parser.add_argument('traffic_file')
parser.add_argument('num_satellites')
parser.add_argument('time_index')
parser.add_argument('time_increment')
args = parser.parse_args()

time_index = int(args.time_index)
time_increment = int(args.time_increment)
num_satellites = int(args.num_satellites)
next_step = {}

for tid in range(0, time_index+time_increment, time_increment):
    weight_dict = {}
    with open(args.traffic_file, 'r') as f:
        for line in f:
            row = line.split(",")
            if (int(row[0])+num_satellites, int(row[1])+num_satellites) not in weight_dict:
                weight_dict[(int(row[0])+num_satellites, int(row[1])+num_satellites)] = int(row[2])
                
    between_dict = {i: 0 for i in range(num_satellites)}
    with open(f'{args.fstate_dir}/fstate_{tid}.txt') as ifile:
        steps = csv.reader(ifile, delimiter=',')
        for step in steps:
            next_step[(int(step[0]),int(step[1]))] = int(step[2])

    for (i,j) in weight_dict.keys():
        satellite = next_step[(i,j)]
        while satellite != j:
            between_dict[satellite]+=weight_dict[(i,j)]
            satellite = next_step[(satellite,j)]
    
    with open(f"betweenness_centrality_{tid}.json", 'w') as f:
        json.dump(between_dict, f)

