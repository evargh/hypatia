Here is how to reproduce the figures:
Fig 2.1:
Drawn in Inkscape.

Fig 2.2:
Follow the guide in `/hypatia/satviz/README.md`, and run `visualize_constellation.py`. The default as I set it is currently the constellation used in the paper.

Fig 2.3:
Drawn in Inkscape.

Fig 2.4:
Drawn in Inkscape.

Fig 2.5:
Drawn in Inkscape.

Fig 2.6:
Drawn in Inkscape.

Fig 3.1:
While this figure requires NS-3, it does not take a long time to run. As a result, I present its introduction separately.

1. Set `NS_LOG="TopologySatelliteNetwork=level_debug"` in your environment.
2. Run any simulation with any routing algorithm, using a command like `tail` to monitor the console. You should see many log lines appear mentioning the lengths of the ISLs. Once the actual simulation starts, quickly terminate the algorithm. This can be done by adding an assert failure to line 329 in `topology-satellite-network.cc`.

Run `python3 parse_lengths.py <our console> > our_linklengths.txt`
Run `python3 link_length_plot.py --constellation_linklengths our_linklengths.txt`

Fig 4.1:
Run `python3 generate_backbone_graphs.py --max_users_exponent 9 --num_satellites_per_orbit 22 --num_orbits 72 --percentile 0.95` to generate the data of 95th-percentile utilization. This experiment is lengthy, so to run tests I recommend reducing `--max_users_exponent`. This will store data to disk in case you want to edit the plots without having to rerun the experiment.
Run `python3 plot_backbone_graphs.py --max_users_exponent <exp> --percentile 0.95` to plot the data of 95th-percentile utilization, where `<exp>` is whatever value you originally used to generate the graphs.
Repeat this process for with flag `--percentile 0.5` for the other plot.

Fig 4.2:
Run the following command:

```
    python3 temporal_flow_latencies_fixed_algorithm.py --percentiles 50 75 95 \
    --name-list "Median Flow in Snapshot Routing @ 10 Mbps Global Traffic" "75th Percentile Flow in Snapshot Routing @ 10 Mbps Global Traffic" "95th Percentile Flow in Snapshot Routing @ 10 Mbps Global Traffic" \
    --file <path to snapshot routing 10 mbps flow_aggregate_times.json> \
    --marker-list o v D \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c

```

Fig: 4.3:
Run the following command:

```
    python3 temporal_flow_latencies_fixed_algorithm.py --percentiles 50 75 95 \
    --name-list "Median Flow in Snapshot Routing @ 30 Mbps Global Traffic" "75th Percentile Flow in Snapshot Routing @ 30 Mbps Global Traffic" "95th Percentile Flow in Snapshot Routing @ 30 Mbps Global Traffic" \
    --file <path to snapshot routing 30 mbps flow_aggregate_times.json> \
    --marker-list o v D \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c

```

Fig 4.4:

```
python3 plot_ingress_traffic_per_node.py --timestamp 120 \
--name-list "Snapshot Routing @ 10 Mbps" "Snapshot Routing @ 30 Mbps" \
--file-list \
<path to snapshot routing 10 mbps ingress.json> \
<path to snapshot routing 30 mbps ingress> \
--line-style-list unbroken unbroken \
--color-list \#1f77b4 \#ff7f0e
```

Fig 4.5:

```
python3 plot_isl_utilization_distribution.py --timestamp 120 \
--name-list "Snapshot Routing @ 10 Mbps" "Snapshot Routing @ 30 Mbps" \
--file-list \
<path to snapshot w1 isl utilization> \
<path to snapshot w3 isl utilization> \ 
--line-style-list unbroken unbroken \
--color-list \#1f77b4 \#ff7f0e
```

Fig 4.6:

```
python3 plot_queue_timelines.py --satellite 1501 --algorithm-name Snapshot --queue-timeline-folder <path to snapshot queue timelines folder>
```

Fig 4.7:

```

    python3 temporal_flow_latencies_fixed_percentile.py --percentile 50 \
    --name-list "Snapshot Routing @ 10 Mbps Global Traffic" "HBM @ 10 Mbps Global Traffic" "Modified DHBP @ 10 Mbps Global Traffic" "ELB over HBM @ 10 Mbps GlobalTraffic" \
    --file-list <path to the snapshot flow aggregate times> \
    <path to the hbm flow aggregate times> \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    --marker-list o v D x \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
```

Then repeat this command with `--percentile 95`

Table 4.1:

```
  python3 tabulate_temporal_flow_latencies.py --percentile 95 \
--name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" \
--file-list-one <path to snapshot flow aggregate times at 10 mbps global traffic> <path to hbm flow aggregate times at 10 mbps global traffic> <path to dhbp flow aggregate times at 10 mbps global traffic> <path to elb flow aggregate times at 10 mbps global traffic> \
--file-list-two <path to snapshot flow aggregate times at 30 mbps global traffic> <path to hbm flow aggregate times at 30 mbps global traffic> <path to dhbp flow aggregate times at 30 mbps global traffic> <path to elb flow aggregate times at 30 mbps global traffic>
```

Fig 4.8:

```

    python3 temporal_flow_latencies_fixed_percentile.py --percentile 50 \
    --name-list "Snapshot Routing @ 30 Mbps Global Traffic" "HBM @ 30 Mbps Global Traffic" "Modified DHBP @ 30 Mbps Global Traffic" "ELB over HBM @ 30 Mbps GlobalTraffic" \
    --file-list <path to the snapshot flow aggregate times> \
    <path to the hbm flow aggregate times> \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    --marker-list o v D x \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
```

Then repeat this command with `--percentile 95`

Fig 4.9:

```
  python3 plot_unfinished_flows.py \
    --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" \
    --file-list <path to the snapshot flow aggregate times> \
    <path to the hbm flow aggregate times> \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    --line-style-list unbroken dashed dotdash dotted \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
```

Fig 4.10/4.11:

```
    python3 plot_ingress_traffic_per_node.py --timestamp 120 \
    --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" \
    --file-list \
    <path to the snapshot ingress> \
    <path to the hbm ingress> \
    <path to the dhbp ingress> \
    <path to the elb ingress> \
    --line-style-list unbroken dashed dotdash dotted \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
```

This produces two figures. Adjust the `--timestamp` field as necessary.

Table 4.2:

```

    python3 plot_isl_distribution_utilization.py --timestamp 120 \
    --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" \
    --file-list \
    <path to the snapshot isl utilization file> \
    <path to the hbm isl utilization file> \
    <path to the dhbp isl utilization file> \
    <path to the elb isl utilization file> \
    --line-style-list unbroken dashed dotdash dotted \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728

```

This will print out the percentiles at 120s into console.

Fig 4.12:

```

python3 plot_isl_utilization_distribution.py --timestamp 120 --name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" --file-list ~/graphing_data/data/snapshot_w3/isl_utilization.csv ~/graphing_data/data/hbm_w3/isl_utilization.csv ~/graphing_data/data/dhbp_w3_optimized/isl_utilization.csv ~/graphing_data/data/elb_w3/isl_utilization.csv --line-style-list unbroken dashed dotdash dotted --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
```

Fig 4.13:

```
python3 plot_queue_timelines.py --satellite 1501 --algorithm-name DHBP --queue-timeline-folder <path to dhbp queue timelines folder>
```

Table 4.3:
For each algorithm, create a folder called `queue_timeline`. Go to that directory, then run `python3 ~/hypatia-container/hypatia/paper/helper_scripts/parse_queue_length_timeline.py <path to algorithm console.txt>`. The directory will be populated by json files.

Then, for each algorithm, run:
```python3 parse_standing_queues.py <queue timeline directory for chosen algorithm> 30 45 60 75 90 105 1```

Fig 5.1:
Drawn in Inkscape.

Table 6.1:

```
python3 tabulate_temporal_flow_latencies.py --percentile 95 --name-list "nil" --file-list-one /home/evanjv2/graphing_data/data/inner_n2_w3/flow_aggregate_times.json --file-list-two /home/evanjv2/graphing_data/data/dhbp_w3_optimized/flow_aggregate_times.json
```

Fig 6.1:

```

    python3 temporal_flow_latencies_fixed_percentile.py --percentile 50 \
    --name-list "Modified DHBP @ 30 Mbps Global Traffic" "ELB over HBM @ 30 Mbps Global Traffic" "INNER n=2 @ 30 Mbps Global Traffic" \
    --file-list
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    <path to inner flow aggregate times> \
    --marker-list D x s\
    --color-list \#2ca02c \#d62728 \#9467bd
```

Then repeat with `--percentile 95`.

Fig 6.2:

```

  python3 plot_unfinished_flows.py \
    --name-list Modified DHBP" "ELB over HBM" "INNER n=2" \
    --file-list <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    <path to the inner flow aggregate times> \
    --line-style-list dotdash dotted longdash \
    --color-list \#2ca02c \#d62728 \#9467bd

```

Fig 6.3:

```
    python3 plot_flow_distributions.py --timestamp 80 \
    --name-list "Modified DHBP" "ELB over HBM" "INNER n=2" \
    --file-list \
    <path to the dhbp flow aggregate times> \
    <path to the elb flow aggregate times> \
    <path to the inner flow aggregate times> \
    --line-style-list dotdash dotted longdash \
    --color-list \#2ca02c \#d62728 \#9467bd
```

Then repeat with `--timestamp 120` and `--timestamp 160`.

Table 6.2:
To calculate the DHBP overhead:
Run `grep -rIi "PointToPointLaserNetDevice:Receive" <dhbp console file> | wc -l` to get the number of times a satellite received a packet (a)
Run `grep -rIi "PointToPointLaserNetDevice:Send" <dhbp console file> | wc -l` to get the number of times a satellite sent a packet (b)
Run `grep -rIi "GSLChannel:TransmitTo" <dhbp console file> | wc -l` to get the number of times a satellite transmitted to or received from a ground station (c)

Find the sum of (a) and (b), then run `python3 calculate_overhead_dhbp.py --num-isl-transcieves <a+b> --num-gsl-transcieves <c> --total-traffic-kb <total amount of traffic in kilobytes>`, where the third argument is the total amount of traffic across the whole simulation, not per second. For example, 30 Mbps global traffic per second really means 30\*1000\*200 total kilobytes of traffic across the simulation.

To calculate the ELB overhead:
Run `grep -rIi "congestion change to beta" <elb console file> | wc -l` to get the number of times a node reported that it's in the busy state (a)
 Run `grep -rIi "congestion change to alpha" <elb console file> | wc -l` to get the number of times a node reported that it's entering FBS (b)
Run `grep -rIi "congestion change to uncongested" <elb console file> | wc -l` to get the number of times a node reported that it's uncongested (c)

Find the sum of (a) and (b), then run `python3 calculate_overhead_dhbp.py --num-beta-messages <a> --num-non-beta-messages <b+c> --total-traffic-kb <total amount of traffic in kilobytes>`, where the third argument is the same as for DHBP.

To calculate the INNER overhead:

Run `grep -rIi "congested" <inner console file> | wc -l` to get the number of times a node reported that an interface is congested (a)

Run `python3 calculate_overhead_inner.py --num-congestion-messages <a> --total-traffic-kb <total amount of traffic in kilobytes>`

Fig 6.4:

    python3 plot_isl_distribution_utilization.py --timestamp 80 \
    --name-list "Modified DHBP" "ELB over HBM" "INNER n=2" \
    --file-list \
    <path to the dhbp isl utilization file> \
    <path to the elb isl utilization file> \
    <path to the inner isl utilization file> \
    --line-style-list dotdash dotted longdash \
    --color-list \#2ca02c \#d62728 \#9467bd

Fig 6.5:

```

    python3 temporal_flow_latencies_fixed_percentile.py --percentile 95 \
    --name-list "INNER n=2 @ 30 Mbps Global Traffic" "INNER n=3 @ 30 Mbps Global Traffic" "INNER n=2 @ 50 Mbps Global Traffic" "INNER n=3 @ 50 Mbps Global Traffic"\
    --file-list
    <path to inner n=2 flow aggregate times for 30 mbps> \
    <path to inner n=3 flow aggregate times for 30 mbps> \
    <path to inner n=2 flow aggregate times for 50 mbps> \
    <path to inner n=3 flow aggregate times for 50 mbps> \
    --marker-list s s s s\
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728

```

Fig 6.6:

```
  python3 plot_unfinished_flows.py \
    --name-list "INNER n=2 @ 30 Mbps" "INNER n=3 @ 30 Mbps" "INNER n=2 @ 50 Mbps" "INNER n=3 @ 50 Mbps" \
    --file-list
    <path to inner n=2 flow aggregate times for 30 mbps> \
    <path to inner n=3 flow aggregate times for 30 mbps> \
    <path to inner n=2 flow aggregate times for 50 mbps> \
    <path to inner n=3 flow aggregate times for 50 mbps> \
    --line-style-list unbroken dashed dotdash dotted \
    --color-list \#1f77b4 \#ff7f0e \#2ca02c \#d62728
```

Table 8.1:

```
  python3 tabulate_temporal_flow_latencies.py --percentile 50 \
--name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" "INNER n=2" \
--file-list-one <path to snapshot flow aggregate times at 10 mbps global traffic> \
<path to hbm flow aggregate times at 10 mbps global traffic> \
<path to dhbp flow aggregate times at 10 mbps global traffic> \
<path to elb flow aggregate times at 10 mbps global traffic> \
<path to inner flow aggregate times at 10 mbps global traffic> \
--file-list-two <path to snapshot flow aggregate times at 30 mbps global traffic with 30 mbps isls> \
<path to hbm flow aggregate times at 30 mbps global traffic with 30 mbps isls> \
<path to dhbp flow aggregate times at 30 mbps global traffic with 30 mbps isls> \
<path to elb flow aggregate times at 30 mbps global traffic with 30 mbps isls>
<path to inner flow aggregate times at 30 mbps global traffic with 30 mbps isls>

```

Table 8.2:

```
  python3 tabulate_temporal_flow_latencies.py --percentile 95 \
--name-list "Snapshot Routing" "HBM" "Modified DHBP" "ELB over HBM" "INNER n=2" \
--file-list-one <path to snapshot flow aggregate times at 10 mbps global traffic> \
<path to hbm flow aggregate times at 10 mbps global traffic> \
<path to dhbp flow aggregate times at 10 mbps global traffic> \
<path to elb flow aggregate times at 10 mbps global traffic> \
<path to inner flow aggregate times at 10 mbps global traffic> \
--file-list-two <path to snapshot flow aggregate times at 30 mbps global traffic with 30 mbps isls> \
<path to hbm flow aggregate times at 30 mbps global traffic with 30 mbps isls> \
<path to dhbp flow aggregate times at 30 mbps global traffic with 30 mbps isls> \
<path to elb flow aggregate times at 30 mbps global traffic with 30 mbps isls>
<path to inner flow aggregate times at 30 mbps global traffic with 30 mbps isls>

```
