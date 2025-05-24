import json
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--max_users_exponent")
parser.add_argument("--percentile")
args = parser.parse_args()

max_power = int(args.max_users_exponent)
percentile = float(args.percentile)


def plot_data(
    toroid_average_max, power_law_average_max, toroid_stdev, power_law_stdev, max_power
):
    plt.errorbar(
        range(max_power),
        toroid_average_max,
        yerr=toroid_stdev,
        label="Toroidal (Satellite)",
    )
    plt.errorbar(
        range(max_power),
        power_law_average_max,
        yerr=power_law_stdev,
        label="Power Law (Terrestrial)",
    )
    plt.xticks(range(0, max_power), [2**i for i in range(1, max_power + 1)])
    plt.xlabel("# Stubs Connected to Backbone")
    plt.ylabel(
        f"# Flows Through Edge with {int(percentile * 100)}th-Percentile Utilization"
    )
    plt.title(
        f"{int(percentile * 100)}th-Percentile Edge Congestion in Toroidal and \n Barabási–Albert Power-Law Backbone "
    )
    plt.legend()
    plt.savefig(f"{int(percentile * 100)}_utilization_in_two_graphs.png")


toroid_average_max = []
power_law_average_max = []
toroid_stdev = []
power_law_stdev = []
with open(f"{int(percentile * 100)}_percentile/toroid_average.json") as f:
    toroid_average_max = json.load(f)
with open(f"{int(percentile * 100)}_percentile/power_law_average.json") as f:
    power_law_average_max = json.load(f)
with open(f"{int(percentile * 100)}_percentile/toroid_stdev.json") as f:
    toroid_stdev = json.load(f)
with open(f"{int(percentile * 100)}_percentile/power_law_stdev.json") as f:
    power_law_stdev = json.load(f)

plot_data(
    toroid_average_max,
    power_law_average_max,
    toroid_stdev,
    power_law_stdev,
    max_power,
)
