# since each packet is unique, first group all lines based on the unique packet they reference
# if a line does not reference a UID, throw it out
# then, with these vectors of lines, throw out the first transmitstart. then split by space and compute the time difference for each parsed time in milliseconds
# if theres a dropped packet in the middle of the network, then identify the previous transmit and add in an RTT-sized delay. maybe 5 seconds?
# if a packet cycles endlessly through different nodes, throw it out as a dropped packet

# this requires a log with only point to point and gslchannel log lines

from collections import Counter, deque
import json
import argparse
import sys

packetdicts = {}

def sort_by_time(item):
    return item[-1]

parser = argparse.ArgumentParser()
parser.add_argument('filename')
parser.add_argument('orbits', type=int)
parser.add_argument('satellites_per_orbit', type=int)
args = parser.parse_args()

#linecount = 0
# string parsing is somewhat messy and inelegant
with open(args.filename, 'r') as file:
    # the file is small enough that we can put it in memory as a queue
    for line in file:
        #if linecount > 15000000:
        #    break
        #linecount+=1
        splitline = line.split(" -- ")
        # if the line is valid: somewhat hacky, but the only source of " -- " are my log lines for the net device transmitting and
        # receiving packets
        if len(splitline) > 2:
            # i consistently place the UID at a particular index of all my log lines
            uidphrase = splitline[2].split(" ")
            if uidphrase[0] == "UID":
                UID = int(uidphrase[-1].strip())
                if UID not in packetdicts:
                    packetdicts[UID] = []
                # receive_tuple is the data structure that is ultimately queued
                line_data = line.split(":")
                context = line_data[0].strip()
                result = line_data[1].strip()
                info = line.split(":")[2].split(" -- ");
                receive_tuple = 0;
                # messy way to parse from and to
                fromspot = int(info[0].split(" ")[-1].strip())
                tospot = int(info[1].split(" ")[-1].strip())
                timestamp = float(line.split(" ")[0][1:-1])

                if result == "Receive()":
                    receive_tuple = ("Receive", fromspot, tospot, timestamp)
                if result == "TransmitStart()":
                    receive_tuple = ("TransmitISL", fromspot, tospot, timestamp + float(info[-1].split(" ")[-1]))
                if result == "TransmitTo()":
                    receive_tuple = ("TransmitGSL", fromspot, tospot, timestamp + float(info[-1].split(" ")[-1]))
                packetdicts[UID].append(receive_tuple)

# timediffs is a dictionary, indexed by orbit, that stores a list of non-propagation delays for that orbit
timediffs = {}

def check_blank_deque(deq, fid):
    if fid not in deq or len(deq[fid]) == 0:
        return True
    return False

def pop_appropriate_deque(deq, fid, key):
    try:
        return deq[fid].popleft()
    except KeyError:
        print(f"Investigate {fid} for {key}", file=sys.stderr)
        return None

ttl_fails = set()
for key, values in packetdicts.items():
    if key not in ttl_fails:
        values.sort(key=lambda item: item[3])
        start_gsl_queues = {}
        receive_queues = {}
        for item in values:
            label = item[0]
            from_id = item[1]
            to_id = item[2]
            timestamp = item[3]
            this_orbit = int(from_id/args.satellites_per_orbit)
            if label == "TransmitGSL":
                if(from_id >= int(args.orbits * args.satellites_per_orbit)):
                    # a transmitGSL is preceded by either nothing or a receive
                    # if the first index is greater than 1584, a ground station is transmitting to a satellite
                    if to_id not in start_gsl_queues:
                        start_gsl_queues[to_id] = deque()
                    start_gsl_queues[to_id].append(timestamp)

                else:
                    # if the second index is greater than 1584, a satellite is transmitting to a ground station
                    if check_blank_deque(start_gsl_queues, from_id) and check_blank_deque(receive_queues, from_id):
                        ttl_fails.add(key)
                        print(f"no populated queues for TransmitGSL, assuming TTL error", file=sys.stderr)

                    elif check_blank_deque(start_gsl_queues, from_id) and not check_blank_deque(receive_queues, from_id):
                        test = pop_appropriate_deque(receive_queues, from_id, key)
                        if test is None:
                            sys.exit("GSL: popping from blank receive queue")

                    elif not check_blank_deque(start_gsl_queues, from_id) and check_blank_deque(receive_queues, from_id):
                        test = pop_appropriate_deque(start_gsl_queues, from_id, key)
                        if test is None:
                            sys.exit("GSL: popping from blank gsl queue")
                    else:
                        if receive_queues[from_id][0] < start_gsl_queues[from_id][0]:
                            test = pop_appropriate_deque(receive_queues, from_id, key)
                            if test is None:
                                sys.exit("GSL: both queues full, but popped blank receive queue")
                        else:
                            test = pop_appropriate_deque(start_gsl_queues, from_id, key)
                            if test is None:
                                sys.exit("GSL: both queues full, but popped blank gsl queue")
            
            elif label == "TransmitISL":
                # a transmit is either preceded by a transmitgsl or a receive
                # receives match with transmitisl and transmitgsl--if the "to" in the receive message matches the "from" in the transmit, then its a match
                # alternatively, if the "to" in the receive matches the "from" in the transmitgsl, then it's a match
                minval = None
                if check_blank_deque(start_gsl_queues, from_id) and check_blank_deque(receive_queues, from_id):
                    ttl_fails.add(key)
                    print(f"no populated queues for transmitISL, assuming TTL error", file=sys.stderr)

                elif check_blank_deque(start_gsl_queues, from_id) and not check_blank_deque(receive_queues, from_id):
                    # if the item is in the queue of received packets, then add the time to the list of times
                    minval = pop_appropriate_deque(receive_queues, from_id, key)
                    if minval is None:
                        sys.exit("ISL: popping from blank receive queue")
                    else:    
                        if this_orbit not in timediffs:
                            timediffs[this_orbit] = []
                        timediffs[this_orbit].append(timestamp - minval)

                elif not check_blank_deque(start_gsl_queues, from_id) and check_blank_deque(receive_queues, from_id):
                    # otherwise, just confirm that the spot was used
                    test = pop_appropriate_deque(start_gsl_queues, from_id, key)
                    if test is None:
                        sys.exit("ISL: popping from blank gsl queue")

                else:
                    if receive_queues[from_id][0] < start_gsl_queues[from_id][0]:
                        # if the receive is older, do the same thing
                        minval = pop_appropriate_deque(receive_queues, from_id, key)
                        if minval is None:
                            sys.exit("ISL: both queues full, but popped blank receive queue")
                        else:    
                            if this_orbit not in timediffs:
                                timediffs[this_orbit] = []
                            timediffs[this_orbit].append(timestamp - minval)
                    else:
                        test = pop_appropriate_deque(start_gsl_queues, from_id, key)
                        # should never happen
                        if test is None:
                            sys.exit("ISL: both queues full, but popped blank gsl queue")
            else:
                # a receive is always preceded by a transmitisl
                if to_id not in receive_queues:
                    receive_queues[to_id] = deque()
                receive_queues[to_id].append(timestamp)

            #print(f"GSL: {start_gsl_queues}")
            #print(f"Rec: {receive_queues}")
                

accumulated_counters = {}
for key in timediffs:
    accumulated_counters[key] = Counter(timediffs[key])
print(json.dumps(accumulated_counters))
