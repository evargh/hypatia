import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument("filename")
args = parser.parse_args()
link_lengths = []

with open(args.filename) as f:
    for line in f:
        data = line.split(" ")
        if len(data) >= 3:
            if data[1] == "TopologySatelliteNetwork:ReadISLs():":
                link_lengths.append(float(data[2]))

print(link_lengths)
# print(json.dumps(Counter(props)))
