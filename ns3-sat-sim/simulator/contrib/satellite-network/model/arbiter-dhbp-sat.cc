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

std::tuple<int32_t, int32_t, int32_t> ArbiterDhbpSat::DetermineInterface(int16_t source_alpha, int16_t source_gamma,
																		 int16_t destination_alpha,
																		 int16_t destination_gamma,
																		 int32_t source_node_id, int32_t target_node_id)
{
	int32_t right_hops = neighbors.GetHopcount(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma);
	int32_t left_hops = neighbors.GetHopcount(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma);
	int32_t up_hops = neighbors.GetHopcount(NeighborCoordContainer::UP, destination_alpha, destination_gamma);
	int32_t down_hops = neighbors.GetHopcount(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma);

	int32_t current_hops = neighbors.GetHopcount(NeighborCoordContainer::SELF, destination_alpha, destination_gamma);

	std::vector<ArbiterDhbpSat::distance_element> distances = {
		std::make_tuple(NeighborCoordContainer::LEFT, left_hops,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::LEFT - 1)),
											source_node_id, target_node_id) *
							left_hops),
		std::make_tuple(NeighborCoordContainer::DOWN, down_hops,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::DOWN - 1)),
											source_node_id, target_node_id) *
							down_hops),
		std::make_tuple(NeighborCoordContainer::UP, up_hops,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::UP - 1)),
											source_node_id, target_node_id) *
							up_hops),
		std::make_tuple(NeighborCoordContainer::RIGHT, right_hops,
						GetQueueSizeForFlow(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::RIGHT - 1)),
											source_node_id, target_node_id) *
							right_hops)};

	std::vector<ArbiterDhbpSat::distance_element> thresholded_distances;

	// in order to be in the rectangle, you need to be closer in hopcount than the source you select is
	// similar reasoning to INNER's viable paths
	if (!OPTIMIZED)
	{
		std::tuple<int16_t, int16_t> source_tuple = std::make_tuple(source_alpha, source_gamma);
		int32_t source_hops_to_destination = neighbors.GetHopcount(source_tuple, destination_alpha, destination_gamma);
		for (auto elem : distances)
		{
			if (std::get<1>(elem) <= source_hops_to_destination)
			{
				thresholded_distances.push_back(elem);
			}
		}
	}
	else
	{
		for (auto elem : distances)
		{
			if (std::get<1>(elem) <= current_hops)
			{
				thresholded_distances.push_back(elem);
			}
		}
	}

	// then pick the minimum
	std::sort(thresholded_distances.begin(), thresholded_distances.end(),
			  [](ArbiterDhbpSat::distance_element a, ArbiterDhbpSat::distance_element b) {
				  return std::get<2>(a) < std::get<2>(b);
			  });
	// NS_LOG_DEBUG(m_node_id << " num viable targets to " << target_node_id << ": " << thresholded_distances.size());
	//  consider which interface to use here, can be more complex than this
	/*for (auto elem : thresholded_distances)
	{
		NS_LOG_DEBUG("neighbor: " << std::get<0>(elem) << " queue: " << std::get<2>(elem));
	}*/

	if (thresholded_distances.size() > 0)
	{
		auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
		return if_index;
	}
	else
	{
		NS_LOG_DEBUG("no viable paths, dropping");
		return std::make_tuple(-1, -1, -1);
	}
}

std::tuple<int32_t, std::tuple<int16_t, int16_t>> ArbiterDhbpSat::ExtractClosestTuple(
	std::vector<std::tuple<int32_t, std::tuple<int16_t, int16_t>>> adjacent_satellites,
	std::tuple<int16_t, int16_t> coords)
{
	int best_position = -1;
	int16_t min_distance = -1;
	for (int i = 0; i < adjacent_satellites.size(); i++)
	{
		int16_t adjacent_alpha = std::get<0>(std::get<1>(adjacent_satellites.at(i)));
		int16_t adjacent_gamma = std::get<1>(std::get<1>(adjacent_satellites.at(i)));
		int16_t distance = neighbors.GetHopcount(coords, adjacent_alpha, adjacent_gamma);

		if ((min_distance == -1) || distance < min_distance)
		{
			best_position = i;
			min_distance = distance;
		}
	}
	NS_ASSERT_MSG(best_position != -1, "did not find closest/furthest satellites");
	return adjacent_satellites.at(best_position);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterDhbpSat::ShortDecide(
	std::tuple<int32_t, std::tuple<int16_t, int16_t>> source_satellite_data,
	std::tuple<int32_t, std::tuple<int16_t, int16_t>> destination_satellite_data, int32_t source_node_id,
	int32_t target_node_id)
{
	return DetermineInterface(std::get<0>(std::get<1>(source_satellite_data)),
							  std::get<1>(std::get<1>(source_satellite_data)),
							  std::get<0>(std::get<1>(destination_satellite_data)),
							  std::get<1>(std::get<1>(destination_satellite_data)), source_node_id, target_node_id);
}

void ArbiterDhbpSat::SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table)
{
	m_other_table = table;
}

void ArbiterDhbpSat::SetSourceSatelliteTable(
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *table)
{
	source_satellite_per_flow = table;
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

	std::tuple<int32_t, std::tuple<double, double>> source_satellite_data =
		source_satellite_per_flow->at(source_node_id - num_orbits * num_satellites_per_orbit)
			.at(target_node_id - num_orbits * num_satellites_per_orbit);
	std::tuple<int32_t, std::tuple<double, double>> destination_satellite_data =
		source_satellite_per_flow->at(target_node_id - num_orbits * num_satellites_per_orbit)
			.at(source_node_id - num_orbits * num_satellites_per_orbit);

	std::tuple<int32_t, std::tuple<int16_t, int16_t>> source_satellite_cell =
		std::make_tuple(std::get<0>(source_satellite_data),
						std::make_tuple(neighbors.CreateAlphaCell(std::get<0>(std::get<1>(source_satellite_data))),
										neighbors.CreateGammaCell(std::get<1>(std::get<1>(source_satellite_data)))));

	std::tuple<int32_t, std::tuple<int16_t, int16_t>> destination_satellite_cell = std::make_tuple(
		std::get<0>(destination_satellite_data),
		std::make_tuple(neighbors.CreateAlphaCell(std::get<0>(std::get<1>(destination_satellite_data))),
						neighbors.CreateGammaCell(std::get<1>(std::get<1>(destination_satellite_data)))));

	return ShortDecide(source_satellite_cell, destination_satellite_cell, source_node_id, target_node_id);
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
