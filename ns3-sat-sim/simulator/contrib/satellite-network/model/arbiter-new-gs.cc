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

#include "arbiter-short-gs.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterShortGS");
NS_OBJECT_ENSURE_REGISTERED(ArbiterShortGS);
TypeId ArbiterShortGS::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ArbiterShort").SetParent<ArbiterSatnet>().SetGroupName("BasicSim");
	return tid;
}

ArbiterShortGS::ArbiterShortGS(Ptr<Node> this_node, NodeContainer nodes,
							   std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o,
							   int64_t s_p_o)
	: ArbiterSatnet(this_node, nodes)
{
	m_next_hop_list = next_hop_list;
	num_orbits = n_o;
	satellites_per_orbit = s_p_o;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortGS::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	return m_next_hop_list[target_node_id];
}

void ArbiterShortGS::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										   int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterShortGS::SetGSShortParams(std::tuple<double, double, double, double> pos)
{
	m_asc_alpha = std::get<0>(pos);
	m_asc_gamma = std::get<1>(pos);
	m_desc_alpha = std::get<2>(pos);
	m_desc_gamma = std::get<3>(pos);
}

std::tuple<double, double, double, double> ArbiterShortGS::GetOtherGSShortParamsAt(int32_t idx)
{
	return m_other_table.at(idx - num_orbits * satellites_per_orbit);
}

std::tuple<int64_t, int64_t> ArbiterShortGS::GetOrbitalConfiguration()
{
	return std::make_tuple(num_orbits, satellites_per_orbit);
}

void ArbiterShortGS::SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table)
{
	m_other_table = table;
}

std::tuple<double, double, double, double> ArbiterShortGS::GetGSShortParams()
{
	return std::make_tuple(m_asc_alpha, m_asc_gamma, m_desc_alpha, m_desc_gamma);
}

std::string ArbiterShortGS::StringReprOfForwardingState()
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
