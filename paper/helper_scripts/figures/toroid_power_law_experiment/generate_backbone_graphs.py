import networkx as nx
import numpy as np
import json
import random
from collections import Counter
import argparse
import os

import matplotlib.pyplot as plt
from matplotlib import colormaps
from matplotlib.colors import Normalize

parser = argparse.ArgumentParser()
parser.add_argument("--max_users_exponent")
parser.add_argument("--num_satellites_per_orbit")
parser.add_argument("--num_orbits")
parser.add_argument("--percentile")
args = parser.parse_args()

max_power = int(args.max_users_exponent)
num_satellites_per_orbit = int(args.num_satellites_per_orbit)
num_orbits = int(args.num_orbits)
percentile = float(args.percentile)


def generate_satellite_toroid(num_satellites_per_orbit, num_orbits, gs_num, seed):
    random.seed(seed)
    total_satellites = num_satellites_per_orbit * num_orbits
    # m by n graph
    # when the graph is created, we randomly attach a GS to an unclaimed center node. then we attach it to the 8 nodes around that center node
    G = nx.Graph()
    G.add_nodes_from(range(0, num_satellites_per_orbit * num_orbits))
    orbit_idx = 0
    while orbit_idx < num_orbits:
        starting_satellite_index = orbit_idx * num_satellites_per_orbit
        same_orbit_edges = [
            (
                i + starting_satellite_index,
                (i + 1) % num_satellites_per_orbit + starting_satellite_index,
            )
            for i in range(num_satellites_per_orbit)
        ]
        adjacent_orbit_edges = [
            (
                i + orbit_idx * num_satellites_per_orbit,
                (i + num_satellites_per_orbit + orbit_idx * num_satellites_per_orbit)
                % (total_satellites),
            )
            for i in range(num_satellites_per_orbit)
        ]
        G.add_edges_from(same_orbit_edges)
        G.add_edges_from(adjacent_orbit_edges)
        orbit_idx += 1

    claimed_centers = []
    for i in range(
        total_satellites,
        total_satellites + gs_num,
    ):
        random_satellite = 0
        while True:
            random_satellite = int(random.random() * total_satellites)
            if random_satellite not in claimed_centers:
                claimed_centers.append(random_satellite)
                break

        nearby_random = [random_satellite]
        orbit_number = int(random_satellite / num_satellites_per_orbit)
        # immediately adjacent
        nearby_random.append(
            (random_satellite + 1) % num_satellites_per_orbit
            + num_satellites_per_orbit * orbit_number
        )
        nearby_random.append(
            (random_satellite + num_satellites_per_orbit - 1) % num_satellites_per_orbit
            + num_satellites_per_orbit * orbit_number
        )
        nearby_random.append(
            (random_satellite + num_satellites_per_orbit) % (total_satellites)
        )
        nearby_random.append(
            (random_satellite - num_satellites_per_orbit + total_satellites)
            % (total_satellites)
        )

        # diagonals
        nearby_random.append(
            (
                (random_satellite + 1) % num_satellites_per_orbit
                + num_satellites_per_orbit * (orbit_number + 1)
            )
            % (total_satellites)
        )
        nearby_random.append(
            (
                (random_satellite + 1) % num_satellites_per_orbit
                + num_satellites_per_orbit * (orbit_number - 1)
                + total_satellites
            )
            % (total_satellites)
        )
        nearby_random.append(
            (
                (random_satellite - 1 + num_satellites_per_orbit)
                % num_satellites_per_orbit
                + num_satellites_per_orbit * (orbit_number + 1)
            )
            % (total_satellites)
        )
        nearby_random.append(
            (
                (random_satellite - 1 + num_satellites_per_orbit)
                % num_satellites_per_orbit
                + num_satellites_per_orbit * (orbit_number - 1)
                + total_satellites
            )
            % (total_satellites)
        )
        gs_edges = [(i, k) for k in nearby_random]
        G.add_edges_from(gs_edges)

    return G


# odd numbers can come out of this due to the randomness of how networkx selects one possible shortest path
# we can also note that the toroidal graph has a much longer "average shortest path," I think this is the underlying reason for
# why toroidal graphs have worse utilization
def calculate_leaf_shortest_paths(G, leaves):
    path_lengths = []
    significant_edges = []
    for source in leaves:
        targets = [i for i in leaves if i is not source]
        for target in targets:
            path = nx.shortest_path(G, source, target)
            path_lengths.append(len(path))
            for i in range(len(path) - 1):
                first_idx = min(path[i], path[i + 1])
                last_idx = max(path[i], path[i + 1])
                significant_edges.append((first_idx, last_idx))

    return Counter(significant_edges)


def generate_BA_with_attachments(total_satellites, k, ground_stations, seed):
    random.seed(seed)
    complete = nx.complete_graph(k)
    G = nx.barabasi_albert_graph(total_satellites, k - 1, seed, complete)
    # for now, cities are uniformly distributed around the resulting graph,
    # but we can do preferential attachment based on degree as well

    claimed_centers = []
    for i in range(
        total_satellites,
        total_satellites + ground_stations,
    ):
        random_satellite = 0
        while True:
            random_satellite = int(random.random() * total_satellites)
            if random_satellite not in claimed_centers:
                claimed_centers.append(random_satellite)
                break

        G.add_edges_from([(i, random_satellite)])

    return G


def generate_data(
    num_satellites_per_orbit, num_orbits, ground_stations, max_power, percentile
):
    toroid_average_max = []
    toroid_stdev = []
    power_law_average_max = []
    power_law_stdev = []

    # the base toroidal graph has 2*num_satellites_per_orbit*num_orbits edges
    # for a fixed k, a power law graph has (k*(k-1))/2 + (k-1)(n-k) edges
    # for an equivalent amount of edges, the BA graph needs
    # 2(num_satellites_per_orbit * num_orbits)/2
    #
    # as a result, the requisite amount of nodes is:
    # (4*num_orbits*num_satellites_per_orbit - k**2 - k)/(2k-2) + k
    k = 4
    BA_node_count = (4 * num_orbits * num_satellites_per_orbit - k**2 + k) / (
        2 * k - 2
    ) + k

    while ground_stations <= 2**max_power:
        toroid_max_edge = []
        power_law_max_edge = []
        for i in range(5):
            toroidgraph = generate_satellite_toroid(
                num_satellites_per_orbit, num_orbits, ground_stations, i * 10
            )
            power_law_graph = generate_BA_with_attachments(
                int(BA_node_count), 4, ground_stations, i * 10
            )
            toroid_edges = calculate_leaf_shortest_paths(
                toroidgraph,
                [
                    i
                    for i in range(
                        toroidgraph.number_of_nodes() - ground_stations,
                        toroidgraph.number_of_nodes(),
                    )
                ],
            )
            power_law_edges = calculate_leaf_shortest_paths(
                power_law_graph,
                [
                    i
                    for i in range(
                        power_law_graph.number_of_nodes() - ground_stations,
                        power_law_graph.number_of_nodes(),
                    )
                ],
            )
            print(f"toroid: {sum(list(toroid_edges.values()))}")
            toroid_edges = list(toroid_edges.values()) + [0] * (
                toroidgraph.number_of_edges() - len(list(toroid_edges.values()))
            )
            print(f"pl: {sum(list(power_law_edges.values()))}")
            power_law_edges = list(power_law_edges.values()) + [0] * (
                power_law_graph.number_of_edges() - len(list(power_law_edges.values()))
            )

            # print(int(percentile * len(power_law_edges)))
            access_percentile_toroid = sorted(toroid_edges)[
                int(percentile * len(toroid_edges))
            ]
            access_percentile_power_law = sorted(power_law_edges)[
                int(percentile * len(power_law_edges))
            ]
            # print(access_percentile_power_law)
            toroid_max_edge.append(access_percentile_toroid)
            power_law_max_edge.append(access_percentile_power_law)

        toroid_average_max.append(sum(toroid_max_edge) / len(toroid_max_edge))
        power_law_average_max.append(sum(power_law_max_edge) / len(power_law_max_edge))
        toroid_stdev.append(np.std(toroid_max_edge))
        power_law_stdev.append(np.std(power_law_max_edge))
        ground_stations *= 2

    return (toroid_average_max, power_law_average_max, toroid_stdev, power_law_stdev)


(toroid_average_max, power_law_average_max, toroid_stdev, power_law_stdev) = (
    generate_data(num_satellites_per_orbit, num_orbits, 2, max_power, percentile)
)

os.makedirs(os.path.dirname(f"{int(percentile * 100)}_percentile2/"), exist_ok=True)
with open(f"{int(percentile * 100)}_percentile2/toroid_average.json", "w+") as f:
    json.dump(toroid_average_max, f)
with open(f"{int(percentile * 100)}_percentile2/power_law_average.json", "w+") as f:
    json.dump(power_law_average_max, f)
with open(f"{int(percentile * 100)}_percentile2/toroid_stdev.json", "w+") as f:
    json.dump(toroid_stdev, f)
with open(f"{int(percentile * 100)}_percentile2/power_law_stdev.json", "w+") as f:
    json.dump(power_law_stdev, f)
