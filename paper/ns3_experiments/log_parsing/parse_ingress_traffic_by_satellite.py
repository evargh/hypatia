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
            transmission_type = inp[1].split(":")
            if len(transmission_type) >= 2:
                if transmission_type[1] == "Receive()":
                    ms_time = int(float(inp[0][1:-1])*10)
                    if int(inp[3]) not in time_index[ms_time]:
                         time_index[ms_time][int(inp[3])]= 0
                    time_index[ms_time][int(inp[3])] += 1

print(json.dumps(time_index))


