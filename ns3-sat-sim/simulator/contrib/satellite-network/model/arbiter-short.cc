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

#include "arbiter-short.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterShort");
NS_OBJECT_ENSURE_REGISTERED(ArbiterShort);
TypeId ArbiterShort::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ArbiterShort").SetParent<ArbiterSatnet>().SetGroupName("BasicSim");
	return tid;
}

ArbiterShort::ArbiterShort(Ptr<Node> this_node, NodeContainer nodes,
						   std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list)
	: ArbiterSatnet(this_node, nodes)
{
	m_next_hop_list = next_hop_list;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShort::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	// TODO: need to see if there's a way to check of the existence of a header on this packet
	// if there isn't, a header needs to be put on. for routing only with satellites, the first ground station
	// can do this.
	// can peekheader to see if the pointer isn't null
	// if the pointer is null, then can generate that from target_node_id
	if (pkt)
	{
		ShortHeader sh;
		pkt->PeekHeader(sh);
		NS_LOG_DEBUG(sh.GetInstanceTypeId().GetName());
	}
	return m_next_hop_list[target_node_id];
}

void ArbiterShort::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										 int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterShort::SetShortParams(double alpha, double gamma)
{
	NS_LOG_DEBUG(m_node_id << ": alpha - " << alpha << " - gamma - " << gamma);
	m_alpha = alpha;
	m_gamma = gamma;
}

std::string ArbiterShort::StringReprOfForwardingState()
{
	std::ostringstream res;
	res << "Single-forward state of node " << m_node_id << std::endl;
	for (size_t i = 0; i < m_nodes.GetN(); i++)
	{
		res << "  -> " << i << ": (" << std::get<0>(m_next_hop_list[i]) << ", " << std::get<1>(m_next_hop_list[i])
			<< ", " << std::get<2>(m_next_hop_list[i]) << ")" << std::endl;
	}
	return res.str();
}

} // namespace ns3
