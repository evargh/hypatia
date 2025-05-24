import matplotlib.pyplot as plt
import json
import numpy as np
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--constellation_linklengths")
args = parser.parse_args()

print(args.constellation_linklengths)

fig, ax = plt.subplots(1, 1)
ax.set_title(f"ECDF of Link Lengths")
with open(args.constellation_linklengths) as ifile:
    plotter = json.load(ifile)

    x = np.sort(plotter)
    y = np.arange(len(x)) / float(len(x))
    ax.plot(x, y, label="Our Grid+ Topology", linestyle="--")

ax.set_xlabel("Link Length (m)")
ax.legend()
plt.savefig("link_lengths_cdf.png")
