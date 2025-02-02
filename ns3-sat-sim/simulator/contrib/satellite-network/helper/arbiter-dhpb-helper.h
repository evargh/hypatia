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

#ifndef ARBITER_DHPB_HELPER
#define ARBITER_DHPB_HELPER

#include "ns3/ipv4-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/ipv4-dhpb-arbiter-routing.h"
#include "ns3/arbiter-dhpb.h"
#include "ns3/abort.h"

namespace ns3
{

class ArbiterDhpbHelper
{
  public:
	ArbiterDhpbHelper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

  private:
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> InitialEmptyForwardingState();
	void UpdateForwardingState(int64_t t);

	// Parameters
	Ptr<BasicSimulation> m_basicSimulation;
	NodeContainer m_nodes;
	int64_t m_dynamicStateUpdateIntervalNs;
	std::vector<Ptr<ArbiterDhpb>> m_arbiters;
	std::vector<std::set<int32_t>> m_destination_satellite_list;

	int64_t m_num_orbits;
	int64_t m_satellites_per_orbit;
};

} // namespace ns3

#endif /* ARBITER_DHPB_HELPER */
