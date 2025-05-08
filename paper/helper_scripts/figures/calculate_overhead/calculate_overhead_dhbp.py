# For each packet transmission/reception from an isl, a node transmits the following:
#   unbounded bits: identify the relevant flow
#   1 bit: increase or decrease the flow
#   in our simulation, we have 230 flows. This is low, but cannot be served by 7 bits.
#       As a result, we assume 8 bits (1 bytes) in total to identify and mark an increase or decrease in the queue length
#       THIS IS AN UNDERESTIMATION
#
#   It only needs to transmit this information to 3 of its neighbors, since one of the neighbors was the one transmitting this information originally
#
#
# For each packet transmission/reception from a gsl, a node transmits the same. However, it only needs to transmit this information to 4 of its neighbors, since one of the neighbors was the one transmitting this information originally

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--num-isl-transcieves", type=int)
parser.add_argument("--num-gsl-transcieves", type=int)
parser.add_argument("--total-traffic-kb", type=int)
args = parser.parse_args()

isl_trx = args.num_isl_transcieves
gsl_trx = args.num_gsl_transcieves
total_traffic_kb = args.total_traffic_kb

# 3 for the neighbors, 2 for the number of bytes
isl_overhead_data_bytes = 3 * (isl_trx * 1)

# 4 for the neighbors, 2 for the number of bytes
gsl_overhead_data_bytes = 4 * (gsl_trx * 1)

total_overhead_kb = (isl_overhead_data_bytes + gsl_overhead_data_bytes) / 1000
ratio = total_overhead_kb / total_traffic_kb
print(f"{total_overhead_kb} kb")
print(f"ratio: {total_overhead_kb / total_traffic_kb}")
