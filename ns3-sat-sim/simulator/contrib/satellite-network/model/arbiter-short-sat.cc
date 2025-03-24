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
		// phasing. as a result, we counterintuitively route right instead of directly up in order to
		// hit all options. if right fails, then it's taken further up again
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
				return m_neighbor_ids[2]; // go up (interface 3), in case theres a rounding error for the ground station
										  // cell
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
				return m_neighbor_ids[1]; // go down (interface 2), again in case of a rounding error
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
				NS_ASSERT_MSG(false, "routing failure for above-right"); // one must be in range for small-scale phasing
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == -1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma))
				return m_neighbor_ids[1]; // go down
			else if (neighbors.VerifyInRange(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma))
				return m_neighbor_ids[3]; // go right if necessary
			else
				NS_ASSERT_MSG(false, "routing failure for below-right"); // one must be in range for small-scale phasing
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
				NS_ASSERT_MSG(false, "routing failure for above-left"); // one must be in range for small-scale phasing
		}
		if (neighbors.CheckIfGammaIncrease(NeighborCoordContainer::SELF, destination_gamma) == -1)
		{
			if (neighbors.VerifyInRange(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma))
				return m_neighbor_ids[0]; // go left
			else if (neighbors.VerifyInRange(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma))
				return m_neighbor_ids[1]; // go down if necessary
			else
				NS_ASSERT_MSG(false, "routing failure for below-left"); // one must be in range for small-scale phasing
		}
	}
	NS_ASSERT_MSG(false, "something incorrect with determinining shortest path");
	return std::make_tuple(-2, -2, -2);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::DetermineInterface(int16_t destination_alpha,
																		  int16_t destination_gamma,
																		  int32_t target_node_id)
{
	// if i am within a certain distance, but cant reach the cell, see where the cell is relative to me.
	// if its right and above, both right and up may be valid. if both are, do right
	// if its left and above, both left and up may be valid. if both are, do up
	// if its left and below, both left and down may be valid. if both are, do left
	// if its right and below, both down and right may be valid. if both are, do right

	if (neighbors.VerifyInRange(NeighborCoordContainer::SELF, destination_alpha, destination_gamma))
	{
		return HandleClose(destination_alpha, destination_gamma, target_node_id);
	}

	int32_t right_distance = neighbors.GetSquaredEuclideanModularDistance(NeighborCoordContainer::RIGHT,
																		  destination_alpha, destination_gamma);
	int32_t left_distance = neighbors.GetSquaredEuclideanModularDistance(NeighborCoordContainer::LEFT,
																		 destination_alpha, destination_gamma);
	int32_t up_distance =
		neighbors.GetSquaredEuclideanModularDistance(NeighborCoordContainer::UP, destination_alpha, destination_gamma);
	int32_t down_distance = neighbors.GetSquaredEuclideanModularDistance(NeighborCoordContainer::DOWN,
																		 destination_alpha, destination_gamma);

	int32_t min_distance = std::min({right_distance, left_distance, up_distance, down_distance}, std::less<int32_t>());

	if (min_distance == right_distance)
		return m_neighbor_ids[3];
	if (min_distance == left_distance)
		return m_neighbor_ids[0];
	if (min_distance == up_distance)
		return m_neighbor_ids[2];
	if (min_distance == down_distance)
		return m_neighbor_ids[1];

	NS_ASSERT_MSG(false, "something incorrect with determinining shortest path");
	return std::make_tuple(-2, -2, -2);
}

void ArbiterShortSat::SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table)
{
	m_other_table = table;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg,
																   int32_t target_node_id)
{
	int16_t asc_alpha_distance = neighbors.GetAlphaModularDistance(NeighborCoordContainer::SELF, aa);
	int16_t desc_alpha_distance = neighbors.GetAlphaModularDistance(NeighborCoordContainer::SELF, da);

	// if it requires fewer inter-orbit links to go to the ascending alpha, greedily do that
	if (asc_alpha_distance <= desc_alpha_distance)
		return DetermineInterface(aa, ag, target_node_id);
	else
		return DetermineInterface(da, dg, target_node_id);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	if (std::get<0>(m_next_hop_list[target_node_id]) >= num_orbits * num_satellites_per_orbit)
	{
		return m_next_hop_list[target_node_id];
	}
	double aa, ag, da, dg;
	std::tie(aa, ag, da, dg) = m_other_table.at(target_node_id - num_orbits * num_satellites_per_orbit);

	int16_t aac = neighbors.CreateAlphaCell(aa);
	int16_t agc = neighbors.CreateGammaCell(ag);
	int16_t dac = neighbors.CreateAlphaCell(da);
	int16_t dgc = neighbors.CreateGammaCell(dg);

	NS_LOG_DEBUG(aac << " " << agc << " " << dac << " " << dgc);
	return ShortDecide(aac, agc, dac, dgc, target_node_id);
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
