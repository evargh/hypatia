# ELB has two kinds of transmissions: it either communicates that its entering state beta (or changing its value of chi), which requires
# a 32 bit float. lets just do 34 bits (4.25 bytes)
#
# It also can say that its entering a different state, which only requires two bits (0.25 bytes)

import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--num-beta-messages", type=int)
parser.add_argument("--num-non-beta-messages", type=int)
parser.add_argument("--total-traffic-kb", type=int)
args = parser.parse_args()

beta_mes = args.num_beta_messages
non_beta_mes = args.num_non_beta_messages
total_traffic_kb = args.total_traffic_kb

beta_overhead_bytes = 4 * (beta_mes * 4.25)
non_beta_overhead_bytes = 4 * (non_beta_mes * 0.25)

total_overhead_kb = (beta_overhead_bytes + non_beta_overhead_bytes) / 1000
ratio = total_overhead_kb / total_traffic_kb
print(f"{total_overhead_kb} kb")
print(f"ratio: {total_overhead_kb / total_traffic_kb}")
