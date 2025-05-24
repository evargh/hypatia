# When INNER is congested or decongested, it reports to its neighboring satellites which of its four interfaces are congested.
# This requires 4 bits.
# We can be naive and assume that, for our simulation, a satellite communicates to its neighbors by directly stating its ID in the network
# along with its interface.
# This would require 15 bits—11 for the id (max 1584), and 4 for the interface
#
# In total, this is 2 bytes per congestion/decongestion message

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--num-congestion-messages", type=int)
parser.add_argument("--total-traffic-kb", type=int)
args = parser.parse_args()

isl_trx = args.num_congestion_messages
total_traffic_kb = args.total_traffic_kb

isl_overhead_data_bytes = 4 * (isl_trx * 2)

total_overhead_kb = (isl_overhead_data_bytes) / 1000
ratio = total_overhead_kb / total_traffic_kb
print(f"{total_overhead_kb} kb")
print(f"ratio: {total_overhead_kb / total_traffic_kb}")
