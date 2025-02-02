from collections import Counter
import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

time_index = {i: {} for i in range(0, 2000)}
linecount = 0
with open(args.filename) as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) > 1:
            if inp[1] == "PointToPointLaserNetDevice:Receive():":
                time_step = int(float(inp[0][1:-1])*10)
                if int(inp[6]) not in time_index[time_step]:
                    time_index[time_step][int(inp[6])]= 0
                time_index[time_step][int(inp[6])] += int(inp[-1])

print(json.dumps(time_index))


