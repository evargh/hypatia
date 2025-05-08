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
								 int64_t s_p_o, std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids,
								 double lngd, double rngd)
	: ArbiterSatnet(this_node, nodes), num_orbits(n_o), num_satellites_per_orbit(s_p_o),
	  neighbors(lngd, rngd, num_orbits, num_satellites_per_orbit)
{
	m_next_hop_list = next_hop_list;
	m_neighbor_ids = neighbor_ids;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::HandleClose(int16_t destination_alpha, int16_t destination_gamma,
																   int32_t target_node_id)
{
	if (neighbors.CheckIfAlphaIncrease(NeighborCoordContainer::SELF, destination_alpha) == 0)
	{
		// if its directly up, it may be that the right interface is closer due to
		// phasing. as a result, we counterintuitively route right instead of
		// directly up in order to hit all options. if right fails, then it's taken
		// further up again
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == 0)
		{
			NS_ASSERT_MSG(false, "Dropped inaccessible ground station: " << m_node_id << " " << target_node_id);
			return std::make_tuple(-1, -1, -1);
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == 1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma))
				return m_neighbor_ids[3]; // go right (interface 4)
			else if (neighbors.VerifyInRange(NeighborCoordContainer::UP, destination_alpha, destination_gamma))
				return m_neighbor_ids[2]; // go up (interface 3), in case theres a
										  // rounding error for the ground station cell
			else
			{
				NS_ASSERT_MSG(false, "routing failure directly up");
			}
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == -1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma))
				return m_neighbor_ids[0]; // go left (interface 1)
			else if (neighbors.VerifyInRange(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma))
				return m_neighbor_ids[1]; // go down (interface 2), again in case of a
										  // rounding error
			else
				NS_ASSERT_MSG(false, "routing failure directly down");
		}
	}
	if (neighbors.CheckIfAlphaIncrease(NeighborCoordContainer::SELF, destination_alpha) == 1)
	{
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == 0)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma))
				return m_neighbor_ids[3]; // go right, interface 4
			else
				NS_ASSERT_MSG(false, "routing failure for directly right");
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == 1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma))
				return m_neighbor_ids[3]; // go right
			else if (neighbors.VerifyInRange(NeighborCoordContainer::UP, destination_alpha, destination_gamma))
				return m_neighbor_ids[2]; // go up if necessary
			else
				NS_ASSERT_MSG(false,
							  "routing failure for above-right"); // one must be in range for
																  // small-scale phasing
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == -1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma))
				return m_neighbor_ids[1]; // go down
			else if (neighbors.VerifyInRange(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma))
				return m_neighbor_ids[3]; // go right if necessary
			else
				NS_ASSERT_MSG(false,
							  "routing failure for below-right"); // one must be in range for
																  // small-scale phasing
		}
	}
	if (neighbors.CheckIfAlphaIncrease(NeighborCoordContainer::SELF, destination_alpha) == -1)
	{
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == 0)
		{
			// for the same reason as we go right when the point is directly up
			if (neighbors.VerifyInRange(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma))
				return m_neighbor_ids[2]; // go left, interface 3
			else
				NS_ASSERT_MSG(false, "routing failure for directly left");
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == 1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::UP, destination_alpha, destination_gamma))
				return m_neighbor_ids[2]; // go up
			else if (neighbors.VerifyInRange(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma))
				return m_neighbor_ids[0]; // go left if necessary
			else
				NS_ASSERT_MSG(false, "routing failure for above-left"); // one must be in range
																		// for small-scale phasing
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == -1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma))
				return m_neighbor_ids[0]; // go left
			else if (neighbors.VerifyInRange(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma))
				return m_neighbor_ids[1]; // go down if necessary
			else
				NS_ASSERT_MSG(false, "routing failure for below-left"); // one must be in range
																		// for small-scale phasing
		}
	}
	NS_ASSERT_MSG(false, "something incorrect with determinining shortest path");
	return std::make_tuple(-2, -2, -2);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::DetermineInterface(int16_t destination_alpha,
																		  int16_t destination_gamma,
																		  int32_t target_node_id)
{
	// NOTE: while this function is called "SHORT," and while it has all the scaffolding necessary to run SHORT, the
	// actual algorithm being tested is hop-based minimization. This was changed to be a fair comparison to INNER. The
	// difference between SHORT and Hop-Based Minimization is the fact that Hop-Based Minimization routes to destination
	// satellites, while SHORT can use its coordinate system to (more slowly) route to ground stations directly
	int32_t right_hops = neighbors.GetHopcount(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma);
	int32_t left_hops = neighbors.GetHopcount(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma);
	int32_t up_hops = neighbors.GetHopcount(NeighborCoordContainer::UP, destination_alpha, destination_gamma);
	int32_t down_hops = neighbors.GetHopcount(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma);

	int32_t min_hops = std::min({right_hops, left_hops, up_hops, down_hops}, std::less<int32_t>());

	if (min_hops == right_hops)
		return m_neighbor_ids[3];
	if (min_hops == left_hops)
		return m_neighbor_ids[0];
	if (min_hops == up_hops)
		return m_neighbor_ids[2];
	if (min_hops == down_hops)
		return m_neighbor_ids[1];

	NS_ASSERT_MSG(false, "something incorrect with determinining shortest path");
	return std::make_tuple(-2, -2, -2);
}

void ArbiterShortSat::SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table)
{
	m_other_table = table;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::ShortDecide(
	std::vector<std::tuple<int16_t, int16_t>> adjacent_satellites, int32_t target_node_id)
{
	int min_position = 0;
	int16_t min_distance = -1;
	for (int i = 0; i < adjacent_satellites.size(); i++)
	{
		int16_t distance = neighbors.GetHopcount(NeighborCoordContainer::SELF, std::get<0>(adjacent_satellites.at(i)),
												 std::get<1>(adjacent_satellites.at(i)));

		if ((min_distance == -1) || distance < min_distance)
		{
			min_position = i;
			min_distance = distance;
		}
	}

	// if it requires fewer inter-orbit links to go to the ascending alpha,
	// greedily do that
	if (min_distance == -1)
	{
		NS_ASSERT_MSG(min_distance != -1, "no minimum distance set");
	}
	return DetermineInterface(std::get<0>(adjacent_satellites.at(min_position)),
							  std::get<1>(adjacent_satellites.at(min_position)), target_node_id);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	if (std::get<0>(m_next_hop_list[target_node_id]) >= num_orbits * num_satellites_per_orbit)
	{
		return m_next_hop_list[target_node_id];
	}

	std::vector<std::tuple<int32_t, std::tuple<double, double>>> adjacent_satellites =
		m_other_table.at(target_node_id - num_orbits * num_satellites_per_orbit);
	std::vector<std::tuple<int16_t, int16_t>> adjacent_satellites_cells;
	for (auto elem : adjacent_satellites)
	{
		adjacent_satellites_cells.push_back(std::make_tuple(neighbors.CreateAlphaCell(std::get<0>(std::get<1>(elem))),
															neighbors.CreateGammaCell(std::get<1>(std::get<1>(elem)))));
	}

	return ShortDecide(adjacent_satellites_cells, target_node_id);
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
	neighbors.UpdateCoords(alpha, gamma);
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

} // namespace ns3
