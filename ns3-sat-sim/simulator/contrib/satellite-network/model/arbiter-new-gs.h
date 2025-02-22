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

#ifndef ARBITER_SHORT_GS_H
#define ARBITER_SHORT_GS_H

#include "ns3/abort.h"
#include "ns3/arbiter-satnet.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/udp-header.h"
#include <tuple>

namespace ns3
{

class ArbiterShortGS : public ArbiterSatnet
{
  public:
	static TypeId GetTypeId(void);

	// Constructor for single forward next-hop forwarding state
	ArbiterShortGS(Ptr<Node> this_node, NodeContainer nodes,
				   std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o);

	// Single forward next-hop implementation
	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(int32_t source_node_id, int32_t target_node_id,
																		 ns3::Ptr<const ns3::Packet> pkt,
																		 ns3::Ipv4Header const &ipHeader,
																		 bool is_socket_request_for_source_ip);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	void SetGSShortParams(std::tuple<double, double, double, double> pos);

	void SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table);

	std::tuple<double, double, double, double> GetOtherGSShortParamsAt(int32_t idx);

	std::tuple<int64_t, int64_t> GetOrbitalConfiguration();

	std::tuple<double, double, double, double> GetGSShortParams();

	// Static routing table
	std::string StringReprOfForwardingState();

  private:
	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_next_hop_list;

	// These parameters are relevant for satellites and ground stations
	double m_asc_alpha;
	double m_asc_gamma;
	double m_desc_alpha;
	double m_desc_gamma;

	std::vector<std::tuple<double, double, double, double>> m_other_table;

	int64_t num_orbits;
	int64_t satellites_per_orbit;

	// This parameter is relevant for ground stations
};

} // namespace ns3

#endif // ARBITER_SINGLE_FORWARD_H
