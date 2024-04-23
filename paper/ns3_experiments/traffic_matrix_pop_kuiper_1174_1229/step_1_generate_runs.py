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

local_shell = exputil.LocalShell()

# Clean-up for new a fresh run
local_shell.remove_force_recursive("runs")
local_shell.remove_force_recursive("pdf")
local_shell.remove_force_recursive("data")

for traffic_mode in ["general"]:
    for movement in ["moving"]:

        # Prepare run directory
        run_dir = "runs/run_" + traffic_mode + "_tm_pairing_kuiper_isls_" + movement
        local_shell.remove_force_recursive(run_dir)
        local_shell.make_full_dir(run_dir)

        # config_ns3.properties
        local_shell.copy_file("templates/template_config_ns3.properties", run_dir + "/config_ns3.properties")
        local_shell.sed_replace_in_file_plain(
            run_dir + "/config_ns3.properties",
            "[SATELLITE-NETWORK-FORCE-STATIC]",
            "true" if movement == "static" else "false"
        )

        # Make logs_ns3 already for console.txt mapping
        local_shell.make_full_dir(run_dir + "/logs_ns3")

        # .gitignore (legacy reasons)
        local_shell.write_file(run_dir + "/.gitignore", "logs_ns3")


        list_from_to = []
        start = 1156
        flow = []
        time = []
        with open('traffic_pop.csv', 'r') as f:
            for id, line in enumerate(f.readlines()):
                values = line.strip().split(",")
                list_from_to.append((start + int(values[0]), start + int(values[1])))
                flow.append(int(values[2]))
                time.append(int(values[3]) * 1000000000)
        list_from_to, flow, time = zip(*sorted(zip(list_from_to, flow, time), key=lambda x: x[2]))
        # Write the schedule
        networkload.write_schedule(
            run_dir + "/schedule_kuiper_630.csv",
            len(list_from_to),
            list_from_to,
            flow,
            time
        )

# Finished successfully
print("Success")
