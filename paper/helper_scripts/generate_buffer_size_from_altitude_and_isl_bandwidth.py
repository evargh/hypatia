import math

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("isl_link_bandwidth_bps", type=int)
parser.add_argument("orbit_altitude_km", type=int)
args = parser.parse_args()

# r = (6378 + y)*1000
semi_circumference_around_earth_m = math.pi * (6378 + args.orbit_altitude_km) * 1000
rtt_around_semi_circumference_s = 2 * semi_circumference_around_earth_m / 300000000
print(f"{args.isl_link_bandwidth_bps * rtt_around_semi_circumference_s} bits")
# 1500 bytes per packet MTU * 8 bits per byte
print(
    f"{args.isl_link_bandwidth_bps * rtt_around_semi_circumference_s / (1500 * 8)} packets"
)
