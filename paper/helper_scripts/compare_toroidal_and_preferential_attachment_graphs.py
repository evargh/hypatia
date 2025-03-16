import networkx as nx
import matplotlib.pyplot as plt
import random
from collections import Counter

random.seed(1)


def generate_satellite_toroid(num_satellites_per_orbit, num_orbits, gs_num):
    total_satellites = num_satellites_per_orbit * num_orbits
    # m by n graph
    # when the graph is created, we randomly attach a gs to an unclaimed center node. then we attach it to the 8 nodes around that center node
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
        print(nearby_random)
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
        print(nearby_random)
        gs_edges = [(i, k) for k in nearby_random]
        G.add_edges_from(gs_edges)

    return G


# check correctness, because I'm getting odd numbers here
def calculate_leaf_shortest_paths(G, leaves):
    significant_edges = []
    for source in leaves:
        targets = [i for i in leaves if i is not source]
        for target in targets:
            path = nx.shortest_path(G, source, target)
            for i in range(len(path) - 1):
                first_idx = min(path[i], path[i + 1])
                last_idx = max(path[i], path[i + 1])
                significant_edges.append((first_idx, last_idx))

    return Counter(significant_edges)


num_satellites_per_orbit = 22
num_orbits = 72
ground_stations = 64
toroidgraph = generate_satellite_toroid(
    num_satellites_per_orbit, num_orbits, ground_stations
)
edges = calculate_leaf_shortest_paths(
    toroidgraph,
    [
        i
        for i in range(
            num_satellites_per_orbit * num_orbits,
            num_satellites_per_orbit * num_orbits + ground_stations,
        )
    ],
)

print(edges)
# nx.draw(toroidgraph)
# plt.show()
