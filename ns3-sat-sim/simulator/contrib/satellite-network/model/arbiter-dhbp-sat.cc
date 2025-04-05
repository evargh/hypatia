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

#include "arbiter-dhbp-sat.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterDhbpSat");
NS_OBJECT_ENSURE_REGISTERED(ArbiterDhbpSat);
TypeId ArbiterDhbpSat::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ArbiterDhbpSat").SetParent<ArbiterSatnet>().SetGroupName("BasicSim");
	return tid;
}

ArbiterDhbpSat::ArbiterDhbpSat(Ptr<Node> this_node, NodeContainer nodes,
							   std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o,
							   int64_t s_p_o, std::shared_ptr<std::vector<std::vector<int64_t>>> sdfs,
							   std::shared_ptr<std::vector<std::mutex>> sdfsm,
							   std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd,
							   double rngd)
	: ArbiterShortSat(this_node, nodes, next_hop_list, n_o, s_p_o, neighbor_ids, lngd, rngd)
{
	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterDhbpSat::DetermineInterface(int16_t destination_alpha,
																		 int16_t destination_gamma,
																		 int32_t source_node_id, int32_t target_node_id)
{
	if (neighbors.VerifyInRange(NeighborCoordContainer::SELF, destination_alpha, destination_gamma))
	{
		return HandleClose(destination_alpha, destination_gamma, target_node_id);
	}
	// refactor to use the new functions later
	int32_t right_distance = neighbors.GetHopcount(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma);
	int32_t left_distance = neighbors.GetHopcount(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma);
	int32_t up_distance = neighbors.GetHopcount(NeighborCoordContainer::UP, destination_alpha, destination_gamma);
	int32_t down_distance = neighbors.GetHopcount(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma);

	int32_t current_distance =
		neighbors.GetHopcount(NeighborCoordContainer::SELF, destination_alpha, destination_gamma);

	typedef std::tuple<NeighborCoordContainer::Direction, int32_t, int64_t> distance_element;

	auto distances = {
		std::make_tuple(NeighborCoordContainer::LEFT, left_distance,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::LEFT - 1)),
											source_node_id, target_node_id) *
							left_distance),
		std::make_tuple(NeighborCoordContainer::DOWN, down_distance,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::DOWN - 1)),
											source_node_id, target_node_id) *
							down_distance),
		std::make_tuple(NeighborCoordContainer::UP, up_distance,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::UP - 1)),
											source_node_id, target_node_id) *
							up_distance),
		std::make_tuple(NeighborCoordContainer::RIGHT, right_distance,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::RIGHT - 1)),
											source_node_id, target_node_id) *
							right_distance)};

	std::vector<distance_element> thresholded_distances;
	// the original DHBP has a dependence on knowing the destination and source satellites for making a space
	// restriction.
	// this requires a packet to embed information about source satellite (2 bytes in starlink), and requires
	// some inference about what the destination satellite could be ahead of time.
	//
	// instead of that, for now we restrict the space by only permitting the use of satellites that are geographically
	// closer this also prevents loops in case the traffic is ever sparse
	std::copy_if(distances.begin(), distances.end(), std::back_inserter(thresholded_distances),
				 [current_distance](distance_element i) { return std::get<1>(i) <= current_distance; });

	// then pick the minimum
	std::sort(thresholded_distances.begin(), thresholded_distances.end(),
			  [](distance_element a, distance_element b) { return std::get<2>(a) < std::get<2>(b); });
	NS_LOG_DEBUG(m_node_id << " num viable targets to " << target_node_id << ": " << thresholded_distances.size());
	// consider which interface to use here, can be more complex than this
	for (auto elem : thresholded_distances)
	{
		NS_LOG_DEBUG("neighbor: " << std::get<0>(elem) << " distance: " << std::get<1>(elem)
								  << " queue: " << std::get<2>(elem));
	}

	if (thresholded_distances.size() > 0)
	{
		auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
		return if_index;
	}
	else
	{
		NS_ASSERT_MSG(thresholded_distances.size() != 0, "something incorrect with determinining shortest path");
		return std::make_tuple(-2, -2, -2);
	}
}

std::tuple<int32_t, int32_t, int32_t> ArbiterDhbpSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg,
																  int32_t source_node_id, int32_t target_node_id)
{
	int16_t asc_alpha_distance = neighbors.GetAlphaModularDistance(NeighborCoordContainer::SELF, aa);
	int16_t desc_alpha_distance = neighbors.GetAlphaModularDistance(NeighborCoordContainer::SELF, da);

	// if it requires fewer inter-orbit links to go to the ascending alpha, greedily do that
	if (asc_alpha_distance <= desc_alpha_distance)
	{
		return DetermineInterface(aa, ag, source_node_id, target_node_id);
	}
	else
	{
		return DetermineInterface(da, dg, source_node_id, target_node_id);
	}
}

std::tuple<int32_t, int32_t, int32_t> ArbiterDhbpSat::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	if (target_node_id < num_orbits * num_satellites_per_orbit ||
		source_node_id < num_orbits * num_satellites_per_orbit)
	{
		return std::make_tuple(-1, -1, -1);
	}
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

	return ShortDecide(aac, agc, dac, dgc, source_node_id, target_node_id);
}

void ArbiterDhbpSat::IncreaseQueue(int32_t source_node_id, int32_t target_node_id)
{
	int64_t spot = GetFlowMapping(source_node_id, target_node_id) % 1024;
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	NS_ASSERT(spot < (int32_t)shared_data_for_satellites->at(m_node_id).size());
	shared_data_for_satellites->at(m_node_id).at(spot) += 1;
}

void ArbiterDhbpSat::DecreaseQueue(int32_t source_node_id, int32_t target_node_id)
{
	int64_t spot = GetFlowMapping(source_node_id, target_node_id) % 1024;
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	NS_ASSERT(spot < (int32_t)shared_data_for_satellites->at(m_node_id).size());
	if (shared_data_for_satellites->at(m_node_id).at(spot) > 0)
	{
		shared_data_for_satellites->at(m_node_id).at(spot) -= 1;
	}
	else
	{
		NS_LOG_DEBUG("attempted to reduce 0-length distance queue");
	}
}

int64_t ArbiterDhbpSat::GetQueueSizeForFlow(int32_t neighbor_id, int32_t source_node_id, int32_t target_node_id)
{
	int64_t spot = GetFlowMapping(source_node_id, target_node_id) % 1024;
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(neighbor_id));
	NS_ASSERT(spot < (int32_t)shared_data_for_satellites->at(neighbor_id).size());
	return shared_data_for_satellites->at(neighbor_id).at(spot);
}

int64_t ArbiterDhbpSat::GetFlowMapping(int32_t source_node_id, int32_t target_node_id)
{
	// bit interleaving, source first
	int64_t interleaved = 0;
	for (int i = 0; i < sizeof(int32_t) * 8; i++)
	{
		int64_t source_masked_at_i = (source_node_id) & (1 << i);
		int64_t dest_masked_at_i = (target_node_id) & (1 << i);
		interleaved |= (source_masked_at_i << i);
		interleaved |= (dest_masked_at_i << (i + 1));
	}
	return interleaved;
}

int32_t ArbiterDhbpSat::GSLNodeIdToGSLIndex(int32_t id)
{
	return id - num_orbits * num_satellites_per_orbit;
}

int32_t ArbiterDhbpSat::GSLIndexToGSLNodeId(int32_t id)
{
	return num_orbits * num_satellites_per_orbit + id;
}

void ArbiterDhbpSat::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										   int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

} // namespace ns3
