from collections import Counter
import json
import argparse

packetdicts = {}

linecount = 0

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

with open(args.filename, 'r') as file:
    for line in file:
        linecount += 1
        splitline = line.split(" -- ")
        if len(splitline) > 1:
            uidphrase = splitline[2].split(" ")
            if uidphrase[0] == "UID":
                UID = int(uidphrase[-1].strip())
                if UID not in packetdicts:
                    packetdicts[UID] = []
                else:
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

# there are many float rounding errors here
for key, values in packetdicts.items():
    transmits = [item for item in values if item[0] == "Transmit"]
    receives = [item for item in values if item[0] == "Receive"]
    for t in transmits:
            # assume causality for now--a first transmit between a pair of nodes will always be matched with the first receive
        match = [item for item in receives if (item[2] == t[1])]
        if len(match) >= 1:
            timediffs.append(t[3] - match[0][3])
            receives.remove(match[0])
        if len(match) == 0:
            # many of these drops come at the end, and do not involve any repeated transmissions.--it's not unreasonable to assume that they would have been transmitted had the simulation extended for a bit longer. additionally, if a packet is dropped, it will not be resent with the same UID.
            print(f'drop?: {key}')
            pass
print(json.dumps(Counter(timediffs)))
