# since each packet is unique, first group all lines based on the unique packet they reference
# if a line does not reference a UID, throw it out
# then, with these vectors of lines, throw out the first transmitstart. then split by space and compute the time difference for each parsed time in milliseconds
# if theres a dropped packet in the middle of the network, then identify the previous transmit and add in an RTT-sized delay. maybe 5 seconds?
# if a packet cycles endlessly through different nodes, throw it out as a dropped packet

from collections import Counter
import json

packetdicts = {}

linecount = 0

with open('console.txt', 'r') as file:
    # the file is small enough that we can put it in memory as a queue
    for line in file:
        linecount += 1
        splitline = line.split(" -- ")
        if len(splitline) > 1:
            uidphrase = splitline[2].split(" ")
            if uidphrase[0] == "UID":
                UID = int(uidphrase[-1].strip())
                if UID not in packetdicts:
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
                    receive_tuple = ("Transmit", fromspot, tospot, timestamp + float(info[-1].split(" ")[-1]))
                packetdicts[UID].append(receive_tuple)

timediffs = []

for key, values in packetdicts.items():
    tuple_collection = []
    transmits = [item for item in values if item[0] == "Transmit"]
    receives = [item for item in values if item[0] == "Receive"]
    for t in transmits:
        if (t[0] == "Transmit"):
            # assume causality for now--a first transmit between a pair of nodes will always be matched with the first receive
            match = [item for item in receives if (item[1] == t[1] and item[2] == t[2])]
            if len(match) >= 1:
                timediffs.append(match[0][3] - t[3])
            if len(match) == 0:
                # many of these drops come at the end, and do not involve any repeated transmissions.--it's not unreasonable to assume that they would have been transmitted had the simulation extended for a bit longer. additionally, if a packet is dropped, it will not be resent with the same UID.
                #print(f'drop?: {key}')
                pass
print(json.dumps(Counter(timediffs)))
