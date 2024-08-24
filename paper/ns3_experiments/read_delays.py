from collections import Counter
import json
import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

with open(args.filename) as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) == 3 and inp[1] == "PointToPointLaserChannel:TransmitStart():":
            props.append(float(line.split(" ")[-1])/1000000000)

print(json.dumps(Counter(props)))


