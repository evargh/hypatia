import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename")
parser.add_argument("weight")
args = parser.parse_args()

with open(args.filename, "r") as ifile:
    with open(f"{args.filename}_w{args.weight}.csv", "w") as ofile:
        for line in ifile:
            data = line.split(",")
            # data[2] was the original traffic
            newval = int(int(data[2]) * int(args.weight))
            ofile.write(f"{data[0]},{data[1]},{newval},{data[3]}")
