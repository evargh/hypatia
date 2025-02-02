from collections import Counter
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('filename')
parser.add_argument('num_orbits')
parser.add_argument('satellites_per_orbit')
args = parser.parse_args()

ttls = []
linecount = 0
with open(args.filename) as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) > 4:
            if inp[4] == "Ipv4ArbiterRouting:RouteInput():" or inp[4] == "Ipv4DhpbArbiterRouting:RouteInput():":
                station_num = int(inp[3][:-1])
                if station_num >= int(args.num_orbits) * int(args.satellites_per_orbit) and int(inp[-1]) != 64:
                    ttls.append(int(inp[-1]))

print(json.dumps(ttls))


