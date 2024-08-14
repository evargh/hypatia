# since each packet is unique, first group all lines based on the unique packet they reference
# if a line does not reference a UID, throw it out
# then, with these vectors of lines, throw out the first transmitstart. then split by space and compute the time difference for each parsed time in milliseconds
from collections import Counter
import sys
import json

packetdicts = {}

with open(sys.argv[1], 'r') as file:
    for line in file:
        splitline = line.split(" ")
        if len(splitline) > 4:
            if splitline[-3] == "UID":
                UID = splitline[-1][:-2]
                if UID in packetdicts:
                    packetdicts[UID].append(line)
                else:
                    # ignore the first transmitstart
                    packetdicts[UID] = []

timediffs = []

for key, values in packetdicts.items():
#values = packetdicts["568498"]
    tlines = {}
    for item in values:
        result = item.split(":")[1]
        if(result == "Receive()"):
            nums = item.split(" ")
            tlines[nums[-5]] = float(nums[0][:-2])
        else:
            nums = item.split(" ")
            if nums[-5] in tlines:
                timediffs.append(float(nums[0][:-2]) - tlines[nums[-5]])
                del tlines[nums[-5]]

print(json.dumps(Counter(timediffs)))
# HashMap hs = new HashMap<int, vec<line>>
# for each line:
#   first see if "uid is" is in the line
#   identify last word, trim off parenthesis, see if last word is in the hash map
#   if not, make a new entry and add this line to be the first. if the entry is already there, append this line to the vector of lines

# when complete, go through each entry, iterate over the lines, and for each receive/transmit pair, identify the time difference, and put that into a list
# then histogram all of those time differences
