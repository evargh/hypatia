/*
 * Copyright (c) 2020 ETH Zurich
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Simon               2020
 */

#include <map>
#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>
#include <stdexcept>

#include "ns3/basic-simulation.h"
#include "ns3/tcp-flow-scheduler.h"
#include "ns3/udp-burst-scheduler.h"
#include "ns3/pingmesh-scheduler.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/tcp-optimizer.h"
#include "ns3/arbiter-single-forward-helper.h"
#include "ns3/arbiter-dhpb-helper.h"
#include "ns3/arbiter-short-helper.h"
#include "ns3/ipv4-arbiter-routing-helper.h"
#include "ns3/ipv4-dhpb-arbiter-routing-helper.h"
#include "ns3/ipv4-short-routing-helper.h"
#include "ns3/gsl-if-bandwidth-helper.h"
#include "ns3/point-to-point-laser-helper.h"
#include "ns3/gsl-helper.h"
#include "ns3/dhpb-laser-helper.h"
#include "ns3/dhpb-gsl-helper.h"

using namespace ns3;

int main(int argc, char *argv[])
{
	int routing_algorithm = 0;

	// No buffering of printf
	setbuf(stdout, nullptr);

	// Retrieve run directory
	CommandLine cmd;
	std::string run_dir = "";
	cmd.Usage("Usage: ./waf --run=\"main_satnet --run_dir='<path/to/run/directory>'\"");
	cmd.AddValue("run_dir", "Run directory", run_dir);
	cmd.Parse(argc, argv);
	if (run_dir.compare("") == 0)
	{
		printf("Usage: ./waf --run=\"main_satnet --run_dir='<path/to/run/directory>'\"");
		return 0;
	}

	// Load basic simulation environment
	Ptr<BasicSimulation> basicSimulation = CreateObject<BasicSimulation>(run_dir);

	// Setting socket type
	Config::SetDefault("ns3::TcpL4Protocol::SocketType",
					   StringValue("ns3::" + basicSimulation->GetConfigParamOrFail("tcp_socket_type")));

	// Optimize TCP
	TcpOptimizer::OptimizeBasic(basicSimulation);

	// TODO: should probably downcast the arbiter pointer safely here, haven't tested
	Ptr<TopologySatelliteNetwork> topology;
	if (routing_algorithm == 0)
	{
		topology = CreateObject<TopologySatelliteNetwork>(basicSimulation, Ipv4ArbiterRoutingHelper(),
														  PointToPointLaserHelper(), GSLHelper());
		ArbiterSingleForwardHelper arbiterHelper(basicSimulation, topology->GetNodes());
		// weird scope thing, just move everything into here
		GslIfBandwidthHelper gslIfBandwidthHelper(basicSimulation, topology->GetNodes());

		// Schedule flows
		TcpFlowScheduler tcpFlowScheduler(basicSimulation, topology); // Requires enable_tcp_flow_scheduler=true

		// Schedule UDP bursts
		UdpBurstScheduler udpBurstScheduler(basicSimulation, topology); // Requires enable_udp_burst_scheduler=true

		// Schedule pings
		PingmeshScheduler pingmeshScheduler(basicSimulation, topology); // Requires enable_pingmesh_scheduler=true

		// Run simulation
		basicSimulation->Run();

		// Write flow results
		tcpFlowScheduler.WriteResults();

		// Write UDP burst results
		udpBurstScheduler.WriteResults();

		// Write pingmesh results
		pingmeshScheduler.WriteResults();

		// Collect utilization statistics
		topology->CollectUtilizationStatistics();

		// Finalize the simulation
		basicSimulation->Finalize();

		return 0;
	}
	else if (routing_algorithm == 1)
	{
		topology = CreateObject<TopologySatelliteNetwork>(basicSimulation, Ipv4DhpbArbiterRoutingHelper(),
														  DhpbPointToPointLaserHelper(), DhpbGSLHelper());
		ArbiterDhpbHelper arbiterHelper(basicSimulation, topology->GetNodes());
		GslIfBandwidthHelper gslIfBandwidthHelper(basicSimulation, topology->GetNodes());

		// Schedule flows
		TcpFlowScheduler tcpFlowScheduler(basicSimulation, topology); // Requires enable_tcp_flow_scheduler=true

		// Schedule UDP bursts
		UdpBurstScheduler udpBurstScheduler(basicSimulation, topology); // Requires enable_udp_burst_scheduler=true

		// Schedule pings
		PingmeshScheduler pingmeshScheduler(basicSimulation, topology); // Requires enable_pingmesh_scheduler=true

		// Run simulation
		basicSimulation->Run();

		// Write flow results
		tcpFlowScheduler.WriteResults();

		// Write UDP burst results
		udpBurstScheduler.WriteResults();

		// Write pingmesh results
		pingmeshScheduler.WriteResults();

		// Collect utilization statistics
		topology->CollectUtilizationStatistics();

		// Finalize the simulation
		basicSimulation->Finalize();

		return 0;
	}
	if (routing_algorithm == 2)
	{
		topology = CreateObject<TopologySatelliteNetwork>(basicSimulation, Ipv4ShortRoutingHelper(),
														  PointToPointLaserHelper(), GSLHelper());
		ArbiterShortHelper arbiterHelper(basicSimulation, topology->GetNodes());
		// weird scope thing, just move everything into here
		GslIfBandwidthHelper gslIfBandwidthHelper(basicSimulation, topology->GetNodes());

		// Schedule flows
		TcpFlowScheduler tcpFlowScheduler(basicSimulation, topology); // Requires enable_tcp_flow_scheduler=true

		// Schedule UDP bursts
		UdpBurstScheduler udpBurstScheduler(basicSimulation, topology); // Requires enable_udp_burst_scheduler=true

		// Schedule pings
		PingmeshScheduler pingmeshScheduler(basicSimulation, topology); // Requires enable_pingmesh_scheduler=true

		// Run simulation
		basicSimulation->Run();

		// Write flow results
		tcpFlowScheduler.WriteResults();

		// Write UDP burst results
		udpBurstScheduler.WriteResults();

		// Write pingmesh results
		pingmeshScheduler.WriteResults();

		// Collect utilization statistics
		topology->CollectUtilizationStatistics();

		// Finalize the simulation
		basicSimulation->Finalize();

		return 0;
	}
	else
	{
		NS_ASSERT(false);
		return 1;
	}
}
