import sys

weight = int(sys.argv[1])

with open("traffic_pop.csv", "r") as ifile:
    with open(f"traffic_pop_w{weight}.csv", "w") as ofile:
        for line in ifile:
            data = line.split(",")
            newval = int(int(data[2])*weight/20)
            ofile.write(f"{data[0]},{data[1]},{newval},{data[3]}")
