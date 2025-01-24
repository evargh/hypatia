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
            if inp[1] == "DhpbPointToPointLaserNetDevice:Send():":
                if int(int(inp[3])/args.satellites_per_orbit) not in len_by_satellite:
                    len_by_satellite[int(int(inp[3])/args.satellites_per_orbit)] = []
                len_by_satellite[int(int(inp[3])/args.satellites_per_orbit)].append(int(inp[-1]))

for k in len_by_satellite:
    len_by_satellite[k] = Counter(len_by_satellite[k])

print(json.dumps(len_by_satellite))


