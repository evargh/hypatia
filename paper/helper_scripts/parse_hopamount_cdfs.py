from collections import Counter
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("num_orbits")
parser.add_argument("satellites_per_orbit")
args = parser.parse_args()

ttls = []
linecount = 0
with open(args.filename) as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) > 1:
            if (
                inp[1] == "Ipv4ArbiterRouting:RouteInput():"
                or inp[1] == "Ipv4DhpbArbiterRouting:RouteInput():"
                or inp[1] == "Ipv4ShortRouting:RouteInput():"
            ):
                # this assumes that the only nodes calling this function are the ground stations
                ttls.append(int(inp[-1]))

print(json.dumps(ttls))
