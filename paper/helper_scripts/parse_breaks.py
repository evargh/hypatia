from collections import Counter
import json
import argparse

props = []

parser = argparse.ArgumentParser()
parser.add_argument('interval')
args = parser.parse_args()
broken_link_count = []

with open("gs_neighbors_at_0.json") as ifile:
    input_dict = json.load(ifile)
    starter_dict = {i: set([k[1] for k in input_dict[i]]) for i in input_dict.keys()}

for i in range(int(args.interval)*1000000, 200000000000, int(args.interval)*1000000):
    filename = f"gs_neighbors_at_{i}.json"
    with open(filename) as ifile:
        input_dict = json.load(ifile)
        comp_dict = {i: set([k[1] for k in input_dict[i]]) for i in input_dict.keys()}
        broken_link = 0
        for i in input_dict.keys():
            broken_link += len(starter_dict[i] - comp_dict[i]) + len(comp_dict[i] - starter_dict[i])
        broken_link_count.append(broken_link)
        starter_dict = comp_dict

broken_link_count.sort()
print(broken_link_count[int(len(broken_link_count)*0.9)])
#print(json.dumps(Counter(props)))


