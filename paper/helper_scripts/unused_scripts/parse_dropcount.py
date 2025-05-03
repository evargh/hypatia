from collections import Counter
import json
import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

linecount = 0
with open(args.filename) as ifile:
    for line in ifile:
        #if linecount > 300000:
        #    break
        #linecount+=1
        inp = line.split(" ")
        if len(inp) > 1 and inp[-1].strip() == "Drop.":
            props.append(float(inp[0][1:-1]))

print(json.dumps(Counter(props)))


