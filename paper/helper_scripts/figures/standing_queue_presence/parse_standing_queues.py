# look at the queue timelines files

import json
import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("queue_timeline_dir")
parser.add_argument("packet_thresholds", nargs="+", type=int)
parser.add_argument("time_threshold", type=float)
args = parser.parse_args()

packet_thresholds = args.packet_thresholds
time_threshold = args.time_threshold

directory = os.fsencode(args.queue_timeline_dir)

overloaded_counters = []

for p in packet_thresholds:
    overloaded_counter = 0
    for filename in os.listdir(directory):
        timeline = os.fsdecode(filename)
        with open(args.queue_timeline_dir + "/" + timeline) as f:
            interfaces = json.load(f)
            congested_node = False
            for interface in interfaces.keys():
                see_congestion = False
                congestion_start = [0, 0]
                elapsed_time = 0
                for entry in interfaces[interface]:
                    time_seconds = float(entry[0][1:-1])
                    packets_seen = int(entry[1])
                    if entry[1] >= p and not see_congestion:
                        see_congestion = True
                        congestion_start = (time_seconds, packets_seen)
                    if entry[1] >= p and see_congestion:
                        if elapsed_time >= time_threshold:
                            overloaded_counter += 1
                            congested_node = True
                            break
                        elapsed_time = time_seconds - congestion_start[0]
                    if entry[1] < p and see_congestion:
                        see_congestion = False
                        congestion_start = [0, 0]
                        elapsed_time = 0
    overloaded_counters.append(overloaded_counter)

print(overloaded_counters)
