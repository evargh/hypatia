from collections import Counter
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('filename')
parser.add_argument('orbits', type=int)
parser.add_argument('satellites_per_orbit', type=int)
args = parser.parse_args()

len_by_satellite = {}
linecount = 0
with open(args.filename) as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) > 1:
            if inp[1] == "DhpbPointToPointLaserNetDevice:Send():" or inp[1] == "PointToPointLaserNetDevice:Send():":
                orbit_num = int(inp[3])
                if orbit_num not in len_by_satellite:
                    len_by_satellite[orbit_num] = []
                if int(inp[-1]) != 0:
                    len_by_satellite[orbit_num].append((inp[0], int(inp[-1])))
                if len(len_by_satellite[orbit_num]) == 0 or (int(inp[-1]) == 0 and len_by_satellite[orbit_num][-1][1] != 0):
                    len_by_satellite[orbit_num].append((inp[0], int(inp[-1])))

print(json.dumps(len_by_satellite))


