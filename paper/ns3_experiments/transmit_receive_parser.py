# since each packet is unique, first group all lines based on the unique packet they reference
# if a line does not reference a UID, throw it out
# then, with these vectors of lines, throw out the first transmitstart. then split by space and compute the time difference for each parsed time in milliseconds
# if theres a dropped packet in the middle of the network, then identify the previous transmit and add in an RTT-sized delay. maybe 5 seconds?
# if a packet cycles endlessly through different nodes, throw it out as a dropped packet

from collections import Counter, deque
import json
import argparse

packetdicts = {}

linecount = 0

def sort_by_time(item):
    return item[-1]

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

with open(args.filename, 'r') as file:
    # the file is small enough that we can put it in memory as a queue
    for line in file:
        #linecount += 1
        #if linecount == 30000:
        #    break
        splitline = line.split(" -- ")
        if len(splitline) > 1:
            uidphrase = splitline[2].split(" ")
            if uidphrase[0] == "UID":
                UID = int(uidphrase[-1].strip())
                if UID not in packetdicts:
                    # only log the last transmitgsl
                    packetdicts[UID] = []
                result = line.split(":")[1].strip()
                info = line.split(":")[2].split(" -- ");
                receive_tuple = 0;
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

timediffs = []

# there are many float rounding errors here
for key, values in packetdicts.items():
#values = packetdicts[90]
    values.sort(key=lambda item: item[3])
    #print(values)
    start_gsl_queues = {}
    receive_queues = {}
    for item in values:
        label = item[0]
        from_id = item[1]
        to_id = item[2]
        timestamp = item[3]
        if label == "TransmitGSL":
            if(from_id >= 1584):
                # a transmitGSL is preceded by either nothing or a receive
                # if the first index is greater than 1584, a ground station is transmitting to a satellite
                # it is preceded by nothing
                if to_id not in start_gsl_queues:
                    start_gsl_queues[to_id] = deque()
                start_gsl_queues[to_id].append(timestamp)

            else:
                # if the second index is greater than 1584, a satellite is transmitting to a ground station
                # in that case, it should be preceded by a "receive" or a start_gsl

                minval = 0
                if from_id not in start_gsl_queues or len(start_gsl_queues[from_id]) == 0:
                    # if the item is in the queue of received packets, then add the time to the list of times
                    try: 
                        receive_queues[from_id].popleft() 
                    except KeyError:
                        print(f"investigate {key}")
                        #pass
                elif from_id not in receive_queues or len(receive_queues[from_id]) == 0:
                    # otherwise, just confirm that the spot was used
                    try: 
                        start_gsl_queues[from_id].popleft() 
                    except KeyError:
                        print(f"investigate {key}")
                        #pass
                else:
                    if receive_queues[from_id][0] < start_gsl_queues[from_id][0]:
                        # if the receive is older, do the same thing
                        receive_queues[from_id].popleft()
                    else:
                        # ditto
                        start_gsl_queues[from_id].popleft()
        
        elif label == "TransmitISL":
            # a transmit is either preceded by a transmitgsl or a receive
            # receives match with transmitisl and transmitgsl--if the "to" in the receive message matches the "from" in the transmit, then its a match
            # alternatively, if the "to" in the receive matches the "from" in the transmitgsl, then it's a match
            minval = 0
            if from_id not in start_gsl_queues or len(start_gsl_queues[from_id]) == 0:
                # if the item is in the queue of received packets, then add the time to the list of times
                minval = receive_queues[item[1]].popleft()
                timediffs.append(timestamp - minval)
            elif from_id not in receive_queues or len(receive_queues[from_id]) == 0:
                # otherwise, just confirm that the spot was used
                start_gsl_queues[from_id].popleft()
            else:
                if receive_queues[from_id][0] < start_gsl_queues[from_id][0]:
                    # if the receive is older, do the same thing
                    minval = receive_queues[from_id].popleft()
                    timediffs.append(timestamp - minval)
                else:
                    # ditto
                    start_gsl_queues[from_id].popleft()
        else:
            # a receive is always preceded by a transmitisl
            if to_id not in receive_queues:
                receive_queues[to_id] = deque()
            receive_queues[to_id].append(item[-1])

        #print(f"GSL: {start_gsl_queues}")
        #print(f"Rec: {receive_queues}")
                

    #for t in transmits:
        # for each marker that a packet was received, attempt to find the transmission that corresponds.
        # due to the causality of transmissions for each UID, we can assume that the first transmission can always be correlated with the first reception
        # if the first transmit is from a GSL to a satellite, ignore it (it always should be)
    #    match = [item for item in transmits if (item[2] == r[1])]
    #    if len(match) >= 1:
            #if(t[3] - match[0][3] < 0):
                #print(f"alert for {key}")
                #print(t)
                #print(match[0])
    #        timediffs.append(t[3] - match[0][3])
    #        receives.remove(match[0])
    #    if len(match) == 0:
            # many of these drops come at the end, and do not involve any repeated transmissions.--it's not unreasonable to assume that they would have been transmitted had the simulation extended for a bit longer. additionally, if a packet is dropped, it will not be resent with the same UID.
#            print(f'drop?: {key}')
    #        pass
print(json.dumps(Counter(timediffs)))
