# The MIT License (MIT)
#
# Copyright (c) 2020 ETH Zurich
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import exputil
import networkload
import random
import argparse
import os

NS3_SAT_SIM_DIRECTORY = os.environ['HYPATIA_NS3_DIR']

local_shell = exputil.LocalShell()

# Clean-up for new a fresh run
local_shell.remove_force_recursive("runs")
local_shell.remove_force_recursive("pdf")
local_shell.remove_force_recursive("data")

parser = argparse.ArgumentParser()
# figure this out
parser.add_argument('--poisson', action=argparse.BooleanOptionalAction)
parser.add_argument('queue_size', type=int)
parser.add_argument('traffic_filename')
parser.add_argument('transport', choices=["tcp", "udp"])

args = parser.parse_args()

for traffic_mode in ["general"]:
    for movement in ["moving"]:
        config_file = ""
        if args.transport == "tcp":
            config_file = "template_config_ns3.properties"
        elif args.transport == "udp":
            config_file = "template_config_ns3_udp.properties"

        if args.poisson:
            local_shell.exec(f"cd {NS3_SAT_SIM_DIRECTORY}/simulator/contrib/basic-sim/ ; git switch hypatia")
        else:
            local_shell.exec(f"cd {NS3_SAT_SIM_DIRECTORY}/simulator/contrib/basic-sim/ ; git switch nopoisson")
         
# Prepare run directory
        run_dir = "runs/run_" + traffic_mode + "_tm_pairing_starlink_0p75_isls_" + movement
        local_shell.remove_force_recursive(run_dir)
        local_shell.make_full_dir(run_dir)

        # config_ns3.properties
        local_shell.copy_file(f"templates/{config_file}", run_dir + "/config_ns3.properties")
        local_shell.sed_replace_in_file_plain(
            run_dir + "/config_ns3.properties",
            "[SATELLITE-NETWORK-FORCE-STATIC]",
            "true" if movement == "static" else "false"
        )
        
        local_shell.sed_replace_in_file_plain(
            run_dir + "/config_ns3.properties",
            "[MAX-QUEUE-SIZE-PKTS]",
            f"{args.queue_size}"
        )

        # Make logs_ns3 already for console.txt mapping
        local_shell.make_full_dir(run_dir + "/logs_ns3")

        # .gitignore (legacy reasons)
        local_shell.write_file(run_dir + "/.gitignore", "logs_ns3")

        list_from_to = []
        start = 918
        flow = []
        time = []
        with open(args.traffic_filename, 'r') as f:
            for id, line in enumerate(f.readlines()):
                values = line.strip().split(",")
                list_from_to.append((start + int(values[0]), start + int(values[1])))
                flow.append(int(values[2]))
                time.append(int(values[3]) * 1000000000)
        list_from_to, flow, time = zip(*sorted(zip(list_from_to, flow, time), key=lambda x: x[2]))
        # Write the schedule
        if args.transport == "tcp":
            networkload.write_schedule(
                run_dir + "/schedule_starlink_0p75_550.csv",
                len(list_from_to),
                list_from_to,
                flow,
                time
            )
        elif args.transport == "udp":
            # udp sets up traffic flows in a different way.
            # "send from A to B at a rate of X Mbit/s at time T for duration D"
            with open(run_dir + "/schedule_starlink_0p75_550.csv", "w+") as f_out:
                checked_flows = set()
                for i in range(len(list_from_to)):
                    if list_from_to[i][0] not in checked_flows:
                        f_out.write(
                            "%d,%d,%d,%.10f,%d,%d,,\n" % (
                                i,
                                list_from_to[i][0], # A
                                list_from_to[i][1], # B
                                flow[i]/1024.0/1024.0,            # Rate (mbit)
                                0,            # Start Time
                                200000000000       # Duration (ns)
                            )
                        )
                        checked_flows.add(list_from_to[i][0])

# Finished successfully
print("Success")
