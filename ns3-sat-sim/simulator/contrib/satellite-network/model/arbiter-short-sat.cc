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

#include "arbiter-short-sat.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterShortSat");
NS_OBJECT_ENSURE_REGISTERED(ArbiterShortSat);
TypeId ArbiterShortSat::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ArbiterShortSat").SetParent<ArbiterSatnet>().SetGroupName("BasicSim");
	return tid;
}

ArbiterShortSat::ArbiterShortSat(Ptr<Node> this_node, NodeContainer nodes,
								 std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o,
								 int64_t s_p_o, std::shared_ptr<std::vector<int64_t>> sdfs,
								 std::shared_ptr<std::mutex> sdfsm)
	: ArbiterSatnet(this_node, nodes)
{
	m_next_hop_list = next_hop_list;
	num_orbits = n_o;
	num_satellites_per_orbit = s_p_o;
	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;
	std::lock_guard<std::mutex> guard(*shared_data_for_satellites_mutex);
	shared_data_for_satellites->at(m_node_id) = 0;
}

int16_t ArbiterShortSat::GetModularDistance(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	return distance < base - distance ? distance : base = distance;
}

bool ArbiterShortSat::IncreaseInterface(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	if (a < b)
	{
		if (distance < base - distance)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if (distance < base - distance)
		{
			return false;
		}
		else
		{
			return true;
		}
	}
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::FindDirection(int i)
{
	for (int k = num_orbits * num_satellites_per_orbit; k < m_nodes.GetN(); k++)
	{
		if (std::get<1>(m_next_hop_list[k]) == i)
			return m_next_hop_list[k];
	}
	return std::make_tuple<int32_t, int32_t, int32_t>(-2, -2, -2);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg)
{
	int16_t alpha_cell = static_cast<int16_t>(std::round(m_alpha * num_orbits / 360));
	int16_t gamma_cell = static_cast<int16_t>(std::round(m_gamma * num_satellites_per_orbit / 360));

	int16_t asc_alpha_distance = GetModularDistance(alpha_cell, aa, num_orbits);
	int16_t asc_gamma_distance = GetModularDistance(gamma_cell, ag, num_satellites_per_orbit);
	int16_t desc_alpha_distance = GetModularDistance(alpha_cell, da, num_orbits);
	int16_t desc_gamma_distance = GetModularDistance(gamma_cell, dg, num_satellites_per_orbit);

	if (asc_alpha_distance == 0 || desc_alpha_distance == 0)
	{
		if (asc_gamma_distance == 0 || desc_gamma_distance == 0)
		{
			return std::make_tuple(-2, -2, -2);
		}
		if (asc_gamma_distance <= desc_gamma_distance)
		{
			if (IncreaseInterface(gamma_cell, ag, num_satellites_per_orbit))
			{
				// this is not a good way to do this
				// go up (interface 3)
				return FindDirection(3);
			}
			else
			{
				// go down (interface 2)
				return FindDirection(2);
			}
		}
		else
		{
			if (IncreaseInterface(gamma_cell, dg, num_satellites_per_orbit))
			{
				// go up (interface 3)
				return FindDirection(3);
			}
			else
			{
				// go down (interface 2)
				return FindDirection(2);
			}
		}
	}
	else if (asc_alpha_distance <= desc_alpha_distance)
	{
		if (IncreaseInterface(alpha_cell, aa, num_orbits))
		{
			// go right (interface 4)
			return FindDirection(4);
		}
		else
		{ // go left (interface 1)
			return FindDirection(1);
		}
	}
	else
	{
		if (IncreaseInterface(alpha_cell, da, num_orbits))
		{
			// go right (interface 4)
			return FindDirection(4);
		}
		else
		{ // go left (interface 1)
			return FindDirection(1);
		}
	}
	return std::make_tuple(0, 0, 0);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	if (std::get<0>(m_next_hop_list[target_node_id]) == target_node_id)
	{
		return m_next_hop_list[target_node_id];
	}
	if (pkt)
	{
		ShortHeader sh;
		pkt->PeekHeader(sh);
		auto target_node_coords = sh.GetCoordinates();

		int16_t aa, ag, da, dg;
		std::tie(aa, ag, da, dg) = target_node_coords;
		NS_LOG_DEBUG(aa << " " << ag << " " << da << " " << dg);
		return ShortDecide(aa, ag, da, dg);
	}
	return m_next_hop_list[target_node_id];
}

void ArbiterShortSat::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
											int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterShortSat::SetShortParams(double alpha, double gamma)
{
	NS_LOG_DEBUG(m_node_id << ": alpha - " << alpha << " - gamma - " << gamma);
	m_alpha = alpha;
	m_gamma = gamma;
}

std::string ArbiterShortSat::StringReprOfForwardingState()
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

void ArbiterShortSat::SetSharedState(int64_t val)
{
	std::lock_guard<std::mutex> guard(*shared_data_for_satellites_mutex);
	shared_data_for_satellites->at(m_node_id) = val;
}

int64_t ArbiterShortSat::GetSharedState(size_t loc)
{
	if (loc < num_orbits * num_satellites_per_orbit)
	{
		std::lock_guard<std::mutex> guard(*shared_data_for_satellites_mutex);
		return shared_data_for_satellites->at(loc);
	}
	else
	{
		return -1;
	}
}

} // namespace ns3
