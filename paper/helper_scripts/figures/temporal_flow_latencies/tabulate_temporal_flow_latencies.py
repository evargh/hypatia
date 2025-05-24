import json
import argparse
import numpy as np

# Create an ArgumentParser object
parser = argparse.ArgumentParser()
parser.add_argument("--name-list", nargs="+", type=str)
parser.add_argument("--file-list-one", nargs="+", type=str)
parser.add_argument("--file-list-two", nargs="+", type=str)
parser.add_argument("--percentile", type=int)
# Parse the command-line arguments
args = parser.parse_args()


def create_average(data):
    return sum(data) / len(data)


# we ignore the differences involving unfinished flows
def generate_arithmetic_difference(array1, array2, m_range):
    return [
        array1[i] - array2[i]
        for i in m_range
        if (array1[i] != 200.0 and array2[i] != 200.0)
    ]


# we ignore the differences involving unfinished flows
def generate_geometric_difference(array1, array2, m_range):
    return [
        array1[i] / array2[i]
        for i in m_range
        if (array1[i] != 200.0 and array2[i] != 200.0)
    ]


def extract_percentile(filename, percentile):
    return_array = []
    with open(filename) as f:
        base_data = json.load(f)
        for i in range(200):
            this_timestamp = []
            for j in base_data.keys():
                if i >= len(base_data[j]):
                    print(j)
                if base_data[j][i] != -1:
                    this_timestamp.append(base_data[j][i])
                else:
                    this_timestamp.append(200000000000)
            this_timestamp.sort()
            # print(len(this_timestamp))
            # print(int(len(this_timestamp)/2))
            return_array.append(
                this_timestamp[int(len(this_timestamp) * percentile)] / 1000000000
            )
    return return_array


range_iterator = range(40, 180)
dataset_one = [
    extract_percentile(filename, args.percentile / 100.0)
    for filename in args.file_list_one
]
averages_one = {
    args.name_list[i]: create_average(
        generate_arithmetic_difference(dataset_one[i], [0] * 200, range_iterator)
    )
    for i in range(len(dataset_one))
}
print(f"Averages of First Data Set: {averages_one}")

dataset_two = [
    extract_percentile(filename, args.percentile / 100.0)
    for filename in args.file_list_two
]
averages_two = {
    args.name_list[i]: create_average(
        generate_arithmetic_difference(dataset_two[i], [0] * 200, range_iterator)
    )
    for i in range(len(dataset_two))
}
print(f"Averages of Second Data Set: {averages_two}")

geometric_differences_per_flow_start_time = [
    generate_geometric_difference(dataset_two[i], dataset_one[i], range_iterator)
    for i in range(len(dataset_one))
]
geometric_averages_per_flow_start_time = {
    args.name_list[i]: np.prod(geometric_differences_per_flow_start_time[i])
    ** (1 / len(geometric_differences_per_flow_start_time[i]))
    for i in range(len(geometric_differences_per_flow_start_time))
}
print(
    f"Time Increase from Data Set 1 to Data Set 2 (slowdown/speedup): {geometric_averages_per_flow_start_time}"
)
