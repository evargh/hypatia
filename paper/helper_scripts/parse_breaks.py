import json
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("final_time_step")
parser.add_argument("time_increment")
args = parser.parse_args()

props = []
broken_links_over_time = []
old_neighbors = {}

final_time_step = int(args.final_time_step)
time_increment = int(args.time_increment)

with open("gs_neighbors_at_0.json") as ifile:
    input_dict = json.load(ifile)
    # k[1] denotes the index of the satellite in gs_neighbors_at_{i}
    old_neighbors = {i: set([k[1] for k in input_dict[i]]) for i in input_dict.keys()}

for i in range(time_increment, final_time_step, time_increment):
    with open(f"gs_neighbors_at_{i}.json") as ifile:
        input_dict = json.load(ifile)
        current_neighbors = {
            i: set([k[1] for k in input_dict[i]]) for i in input_dict.keys()
        }
        broken_links = 0
        for i in input_dict.keys():
            broken_links += len(current_neighbors[i] - old_neighbors[i]) + len(
                current_neighbors[i] - old_neighbors[i]
            )
        broken_links_over_time.append(broken_links)
        old_neighbors = current_neighbors

print(json.dumps(broken_links_over_time))
