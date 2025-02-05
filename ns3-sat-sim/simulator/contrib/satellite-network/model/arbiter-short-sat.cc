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
								 std::shared_ptr<std::mutex> sdfsm,
								 std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids)
	: ArbiterSatnet(this_node, nodes)
{
	m_next_hop_list = next_hop_list;
	num_orbits = n_o;
	num_satellites_per_orbit = s_p_o;
	m_neighbor_ids = neighbor_ids;

	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;
	std::lock_guard<std::mutex> guard(*shared_data_for_satellites_mutex);
	shared_data_for_satellites->at(m_node_id) = 0;
}

int16_t ArbiterShortSat::GetModularDistance(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	return distance < base - distance ? distance : base - distance;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::DetermineInterface(int16_t alpha_cell, int16_t gamma_cell,
																		  int16_t destination_alpha,
																		  int16_t destination_gamma)
{

	// the right orbit will always be CELL_SCALING_FACTOR units to the right of the current satellite, modulo the
	// number of alpha cells
	int16_t right_alpha_cell =
		(alpha_cell + ArbiterShortSat::CELL_SCALING_FACTOR) % (num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR);
	// the left orbit will always be CELL_SCALING_FACTOR units to the left of the current satellite, modulo the
	// number of alpha cells
	int16_t left_alpha_cell = (alpha_cell + (num_orbits - 1) * ArbiterShortSat::CELL_SCALING_FACTOR) %
							  (num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR);

	// capture whether the right or left orbit is closer
	bool right_shorter =
		GetModularDistance(right_alpha_cell, destination_alpha, num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR) <
		GetModularDistance(alpha_cell, destination_alpha, num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR);

	bool left_shorter =
		GetModularDistance(left_alpha_cell, destination_alpha, num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR) <
		GetModularDistance(alpha_cell, destination_alpha, num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR);

	if (right_shorter)
	{
		// go right (interface 4)
		return m_neighbor_ids[3];
	}
	else if (left_shorter)
	{
		// go left (interface 1)
		return m_neighbor_ids[0];
	}
	else if (!right_shorter && !left_shorter)
	{
		// if neither option is strictly closer, then we just use gamma
		int16_t up_gamma_cell = (gamma_cell + ArbiterShortSat::CELL_SCALING_FACTOR) %
								(num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR);
		int16_t down_gamma_cell = (gamma_cell + (num_satellites_per_orbit - 1) * ArbiterShortSat::CELL_SCALING_FACTOR) %
								  (num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR);

		bool up_shorter = GetModularDistance(up_gamma_cell, destination_gamma,
											 num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR) <
						  GetModularDistance(gamma_cell, destination_gamma,
											 num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR);

		bool down_shorter = GetModularDistance(down_gamma_cell, destination_gamma,
											   num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR) <
							GetModularDistance(gamma_cell, destination_gamma,
											   num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR);
		if (up_shorter)
		{
			// go up (interface 3)
			return m_neighbor_ids[2];
		}
		else if (down_shorter)
		{
			// go down (interface 2)
			return m_neighbor_ids[1];
		}
		else
		{
			// log the frequency of this for now before implementing the counterclockwise thing
			NS_LOG_DEBUG("Dropped inaccessible ground station: " << destination_gamma);
			return std::make_tuple(-1, -1, -1);
		}
	}
	else
	{
		NS_ASSERT_MSG(false, "something incorrect with determinining shortest path");
		return std::make_tuple(-2, -2, -2);
	}
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg)
{
	int16_t alpha_cell =
		static_cast<int16_t>(std::round(m_alpha * num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR / 360));
	if (alpha_cell == num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR)
		alpha_cell--;

	int16_t gamma_cell = static_cast<int16_t>(
		std::round(m_gamma * num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR / 360));
	if (gamma_cell == num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR)
		gamma_cell--;

	int16_t asc_alpha_distance = GetModularDistance(alpha_cell, aa, num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR);
	int16_t desc_alpha_distance = GetModularDistance(alpha_cell, da, num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR);

	// if it requires fewer inter-orbit links to go to the ascending alpha, greedily do that
	if (asc_alpha_distance <= desc_alpha_distance)
		return DetermineInterface(alpha_cell, gamma_cell, aa, ag);
	else
		return DetermineInterface(alpha_cell, gamma_cell, da, dg);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	if (std::get<0>(m_next_hop_list[target_node_id]) >= num_orbits * num_satellites_per_orbit)
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
		NS_ASSERT_MSG(aa >= 0 && aa < num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR && ag >= 0 &&
						  ag < num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR && da >= 0 &&
						  da < num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR && dg >= 0 &&
						  dg < num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR,
					  "invalid cells");
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
