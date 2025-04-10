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

#include "arbiter-new-sat.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterNewSat");
NS_OBJECT_ENSURE_REGISTERED(ArbiterNewSat);
TypeId ArbiterNewSat::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ArbiterNewSat").SetParent<ArbiterSatnet>().SetGroupName("BasicSim");
	return tid;
}

ArbiterNewSat::ArbiterNewSat(Ptr<Node> this_node, NodeContainer nodes,
							 std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o,
							 int64_t s_p_o, std::shared_ptr<std::vector<int64_t>> sdfs,
							 std::shared_ptr<std::vector<std::mutex>> sdfsm,
							 std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids,
							 std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> neighbor_neighbors,
							 double lngd, double rngd)
	: ArbiterShortSat(this_node, nodes, next_hop_list, n_o, s_p_o, neighbor_ids, lngd, rngd)
{
	m_neighbor_neighbors = neighbor_neighbors;
	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	shared_data_for_satellites->at(m_node_id) = 0;
}

float ArbiterNewSat::GetEstimatedPropagationDelay(int32_t horizontal_hops, int32_t vertical_hops)
{
	return (INTER_ORBIT_PROPAGATION_DELAY_SECONDS * float(horizontal_hops) +
			INTRA_ORBIT_PROPAGATION_DELAY_SECONDS * float(vertical_hops)) /
		   float(ArbiterShortSat::CELL_SCALING_FACTOR);
}

std::vector<ArbiterNewSat::distance_element> ArbiterNewSat::PopulateDistances(int32_t current_hops,
																			  int16_t destination_alpha,
																			  int16_t destination_gamma,
																			  int32_t target_node_id)
{
	// get the hopcounts for each second-hop node and populate the distance element
	// also need to know the approximate difference between intra-orbit and inter-orbit links

	std::vector<distance_element> distances;
	for (int neighbor_idx = 1; neighbor_idx < 5; neighbor_idx++)
	{
		// for each neighbor
		std::vector<NeighborCoordContainer::Direction> direction_sequence;
		direction_sequence.resize(2);
		direction_sequence.at(0) = static_cast<NeighborCoordContainer::Direction>(neighbor_idx);

		int64_t neighbor_distance_to_target =
			neighbors.GetHopcount(direction_sequence.at(0), destination_alpha, destination_gamma);

		auto neighbor_id = std::get<0>(m_neighbor_ids.at(neighbor_idx - 1));
		bool neighbor_is_dest = false;
		std::vector<std::tuple<int32_t, std::tuple<double, double>>> adjacent_satellites =
			m_other_table.at(target_node_id - num_orbits * num_satellites_per_orbit);
		for (std::tuple<int32_t, std::tuple<double, double>> elem : adjacent_satellites)
		{
			if (neighbor_id == std::get<0>(elem))
			{
				neighbor_is_dest = true;
			}
		}

		if (neighbor_is_dest || neighbor_distance_to_target < current_hops)
		{
			int64_t congestion_to_neighbor = (GetSharedState(m_node_id) >> (neighbor_idx - 1)) & 1;
			for (int neighbor_of_neighbor_idx = 1; neighbor_of_neighbor_idx < 5; neighbor_of_neighbor_idx++)
			{
				// look at their neighbors
				int32_t neighbor_neighbor_id =
					std::get<0>(m_neighbor_neighbors.at(neighbor_idx - 1).at(neighbor_of_neighbor_idx - 1));
				if (neighbor_neighbor_id != m_node_id)
				{
					direction_sequence.at(1) = static_cast<NeighborCoordContainer::Direction>(neighbor_of_neighbor_idx);
					std::tuple<int16_t, int16_t> coords = neighbors.GetCoordsFromSequence(direction_sequence);
					int16_t neighbor_neighbor_distance_to_target =
						neighbors.GetHopcount(coords, destination_alpha, destination_gamma);

					if (neighbor_is_dest || neighbor_neighbor_distance_to_target < neighbor_distance_to_target)
					{
						int64_t neighbor_congestion_to_neighbor =
							(GetSharedState(neighbor_id) >> (neighbor_of_neighbor_idx - 1)) & 1;

						NS_ASSERT_MSG(
							(congestion_to_neighbor == 0 || congestion_to_neighbor == 1) &&
								(neighbor_congestion_to_neighbor == 0 || neighbor_congestion_to_neighbor == 1),
							"bitshift error");
						int16_t horizontal_hops, vertical_hops;
						std::tie(horizontal_hops, vertical_hops) =
							neighbors.GetHopcountTuple(coords, destination_alpha, destination_gamma);

						int16_t horizontal_hops_to_neighbor = 0, vertical_hops_to_neighbor = 0;
						if (neighbor_idx == NeighborCoordContainer::LEFT ||
							neighbor_idx == NeighborCoordContainer::RIGHT)
						{
							horizontal_hops_to_neighbor += ArbiterShortSat::CELL_SCALING_FACTOR;
						}
						if (neighbor_of_neighbor_idx == NeighborCoordContainer::LEFT ||
							neighbor_of_neighbor_idx == NeighborCoordContainer::RIGHT)
						{
							horizontal_hops_to_neighbor += ArbiterShortSat::CELL_SCALING_FACTOR;
						}
						if (neighbor_idx == NeighborCoordContainer::UP || neighbor_idx == NeighborCoordContainer::DOWN)
						{
							vertical_hops_to_neighbor += ArbiterShortSat::CELL_SCALING_FACTOR;
						}
						if (neighbor_of_neighbor_idx == NeighborCoordContainer::UP ||
							neighbor_of_neighbor_idx == NeighborCoordContainer::DOWN)
						{
							vertical_hops_to_neighbor += ArbiterShortSat::CELL_SCALING_FACTOR;
						}

						float metric_to_neighbor = 0;
						float prop_delay_to_neighbor =
							GetEstimatedPropagationDelay(horizontal_hops_to_neighbor, vertical_hops_to_neighbor);

						if (congestion_to_neighbor + neighbor_congestion_to_neighbor == 0)
						{
							metric_to_neighbor = prop_delay_to_neighbor +
												 2 * MINIMUM_FULLNESS_THRESHOLD * 1500.0 / float(LINK_BANDWIDTH);
						}
						else if (congestion_to_neighbor + neighbor_congestion_to_neighbor == 1)
						{
							metric_to_neighbor =
								prop_delay_to_neighbor +
								(LINK_QUEUE_SIZE + MINIMUM_FULLNESS_THRESHOLD) * 1500.0 / float(LINK_BANDWIDTH);
						}
						else if (congestion_to_neighbor + neighbor_congestion_to_neighbor == 2)
						{
							metric_to_neighbor =
								prop_delay_to_neighbor + 2 * LINK_QUEUE_SIZE * 1500.0 / float(LINK_BANDWIDTH);
						}
						else
						{
							NS_ASSERT_MSG(false, "testing congestion failed");
						}

						float neighbor_metric_to_destination =
							GetEstimatedPropagationDelay(horizontal_hops, vertical_hops);

						// direction, direction, is in range, propagation delay to this neighbor based on hops +
						// standing queue delay, propagation delay to destination based on hops
						distance_element this_dist = std::make_tuple(
							static_cast<NeighborCoordContainer::Direction>(neighbor_idx),
							static_cast<NeighborCoordContainer::Direction>(neighbor_of_neighbor_idx),
							//    use the fact that the gamma/alpha differentials are constant to get
							//    the position of their neighbors
							neighbor_is_dest, congestion_to_neighbor + neighbor_congestion_to_neighbor == 0,
							metric_to_neighbor, neighbor_metric_to_destination);

						distances.push_back(this_dist);
					}
				}
			}
		}
	}
	return distances;
}

void ArbiterNewSat::SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table)
{
	m_other_table = table;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::DetermineInterface(int16_t destination_alpha,
																		int16_t destination_gamma,
																		int32_t target_node_id)
{
	/*
	  if (neighbors.VerifyInRange(NeighborCoordContainer::SELF, destination_alpha, destination_gamma))
	  {
		  return HandleClose(destination_alpha, destination_gamma, target_node_id);
	  }*/
	int32_t current_hops = neighbors.GetHopcount(NeighborCoordContainer::SELF, destination_alpha, destination_gamma);
	NS_LOG_DEBUG(m_node_id << " starting at: (" << std::get<0>(neighbors.GetCoords(NeighborCoordContainer::SELF))
						   << ", " << std::get<1>(neighbors.GetCoords(NeighborCoordContainer::SELF)) << ") ");
	NS_LOG_DEBUG("ending at: (" << destination_alpha << ", " << destination_gamma << ") " << " for " << target_node_id);
	NS_LOG_DEBUG("hopcount: " << current_hops);

	std::vector<distance_element> distances =
		PopulateDistances(current_hops, destination_alpha, destination_gamma, target_node_id);

	// direction, direction, is in range, fastpath, propagation delay to this neighbor based on hops + standing queue
	// delay, propagation delay to destination based on hops

	std::vector<distance_element> very_close_distances;
	std::copy_if(distances.begin(), distances.end(), std::back_inserter(very_close_distances),
				 [](distance_element i) { return std::get<2>(i); });

	std::vector<distance_element> zero_congestion_distances;
	std::copy_if(distances.begin(), distances.end(), std::back_inserter(zero_congestion_distances),
				 [](distance_element i) { return std::get<3>(i); });

	std::vector<distance_element> load_balancing_distances;
	std::copy_if(distances.begin(), distances.end(), std::back_inserter(load_balancing_distances),
				 [](distance_element i) { return !std::get<2>(i) && !std::get<3>(i); });

	NS_ASSERT_MSG(distances.size() != 0, "no one to forward to");
	NS_LOG_DEBUG(m_node_id << " num viable targets to " << target_node_id << ": " << distances.size());

	if (very_close_distances.size() != 0)
	{
		std::sort(very_close_distances.begin(), very_close_distances.end(), [](distance_element a, distance_element b) {
			if (std::get<3>(a) && !std::get<3>(b))
				return true;
			return std::get<4>(a) + std::get<5>(a) < std::get<4>(b) + std::get<5>(b);
		});
		// at low congestion, be deterministic
		auto if_index = m_neighbor_ids.at(std::get<0>(very_close_distances.at(0)) - 1);
		return if_index;
	}
	if (zero_congestion_distances.size() != 0)
	{
		std::sort(zero_congestion_distances.begin(), zero_congestion_distances.end(),
				  [](distance_element a, distance_element b) {
					  return std::get<4>(a) + std::get<5>(a) < std::get<4>(b) + std::get<5>(b);
				  });
		// ditto
		auto if_index = m_neighbor_ids.at(std::get<0>(zero_congestion_distances.at(0)) - 1);
		return if_index;
	}
	if (load_balancing_distances.size() != 0)
	{
		std::sort(load_balancing_distances.begin(), load_balancing_distances.end(),
				  [](distance_element a, distance_element b) {
					  return std::get<4>(a) + std::get<5>(a) < std::get<4>(b) + std::get<5>(b);
				  });
		// as congestion increases, load balance
		NS_LOG_DEBUG(m_node_id << " IS LOAD BALANCING");
		// sort the list of load balancing distances by singular metric
		auto if_index = m_neighbor_ids.at(std::get<0>(load_balancing_distances.at(0)) - 1);
		return if_index;
	}
	NS_ASSERT_MSG(false, "interface check failed");
	auto if_index = m_neighbor_ids.at(std::get<0>(distances.at(0)) - 1);
	return if_index;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::ShortDecide(
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

	// if it requires fewer inter-orbit links to go to the ascending alpha, greedily do that
	if (min_distance == -1)
	{
		NS_ASSERT_MSG(min_distance != -1, "no minimum distance set");
	}
	return DetermineInterface(std::get<0>(adjacent_satellites.at(min_position)),
							  std::get<1>(adjacent_satellites.at(min_position)), target_node_id);
}

void ArbiterNewSat::SetInterfaceCongestionBits()
{
	// when an interface is found to be at an intolerable level, freeze a timestamp
	// if the interface stays congested for an appropriate amount of time after that frozen timestamp, then mark the
	// interface as experiencing a standing queue otherwise, set it as congested, and keep it as congested until the
	// queue goes under intolerable levels
	for (auto i : m_neighbor_ids)
	{
		int32_t id, outgoing_if_id, dummy;
		std::tie(id, outgoing_if_id, dummy) = i;
		Ptr<NetDevice> outgoing_if = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetNetDevice(outgoing_if_id);
		NS_ASSERT_MSG(outgoing_if != 0, "No NetDevice on Interface");
		auto num_packets = outgoing_if->GetObject<PointToPointLaserNetDevice>()->GetQueue()->GetNPackets();
		bool under_target = num_packets < ArbiterNewSat::MINIMUM_FULLNESS_THRESHOLD;

		int64_t current_congestion = GetSharedState(m_node_id);
		int64_t this_interface_congestion_flag = (current_congestion >> (outgoing_if_id - 1)) & 1;
		NS_ASSERT_MSG(this_interface_congestion_flag == 0 || this_interface_congestion_flag == 1,
					  "Flag incorrectly retrieved");

		if (under_target)
		{
			// update the timestamp regardless
			m_interface_congestion_timer.at(outgoing_if_id - 1) = Simulator::Now().GetNanoSeconds();
			// if the node is currently marked as congested
			if (this_interface_congestion_flag == 1)
			{
				auto newval = current_congestion - (1 << (outgoing_if_id - 1));
				SetSharedState(newval);
				NS_LOG_DEBUG(m_node_id << " DE-CONGESTED ON INTERFACE " << outgoing_if_id << ", FROM "
									   << current_congestion << " TO " << newval);
			}
		}
		// if the node is currently marked as uncongested
		else if (this_interface_congestion_flag == 0 &&
				 Simulator::Now().GetNanoSeconds() >
					 m_interface_congestion_timer.at(outgoing_if_id - 1) + ArbiterNewSat::MINIMUM_FULLNESS_INTERVAL_NS)
		{
			auto newval = current_congestion + (1 << (outgoing_if_id - 1));
			SetSharedState(newval);
			NS_LOG_DEBUG(m_node_id << " CONGESTED ON INTERFACE " << outgoing_if_id << ", FROM " << current_congestion
								   << " TO " << newval);
		}
	}
	//    get their queue lengths. if the queue lengths are under 20 packets, store the timestamp for that moment
	//    if the queue lengths are over 20 packets, compare that time since the last timestamp
	//      if the time was over what is appropriate, then mark the bit for that interface as congested
}

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::TopologySatelliteNetworkDecide(
	int32_t source_node_id, int32_t target_node_id, Ptr<const Packet> pkt, Ipv4Header const &ipHeader,
	bool is_request_for_source_ip_so_no_next_header)
{
	SetInterfaceCongestionBits();
	if (source_node_id < num_orbits * num_satellites_per_orbit)
	{
		NS_LOG_DEBUG("dropped packet at " << m_node_id);
	}
	if (std::get<0>(m_next_hop_list[target_node_id]) >= num_orbits * num_satellites_per_orbit)
	{
		return m_next_hop_list[target_node_id];
	}
	if (target_node_id < num_orbits * num_satellites_per_orbit)
	{
		NS_LOG_DEBUG("dropped packet");
		return std::make_tuple(-1, -1, -1);
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

void ArbiterNewSat::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										  int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterNewSat::SetSharedState(int64_t val)
{
	NS_ASSERT_MSG(val >= 0 && val <= 15, "Invalid Shared State Value");
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	shared_data_for_satellites->at(m_node_id) = val;
}

int64_t ArbiterNewSat::GetSharedState(size_t loc)
{
	NS_ASSERT_MSG(loc >= 0 && loc < num_orbits * num_satellites_per_orbit, "Incorrect Index Access");
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(loc));
	return shared_data_for_satellites->at(loc);
}

} // namespace ns3
