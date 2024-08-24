from collections import Counter
import json

props = []

with open("console.txt") as ifile:
    for line in ifile:
        inp = line.split(" ")
        if len(inp) == 3 and inp[1] == "PointToPointLaserChannel:TransmitStart():":
            props.append(float(line.split(" ")[-1])/1000000000)

print(json.dumps(Counter(props)))


