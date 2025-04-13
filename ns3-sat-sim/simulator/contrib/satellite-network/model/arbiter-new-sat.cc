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
							 std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> *ton,
							 std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd)
	: ArbiterShortSat(this_node, nodes, next_hop_list, n_o, s_p_o, neighbor_ids, lngd, rngd)
{
	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;
	table_of_node = ton;
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	shared_data_for_satellites->at(m_node_id) = 0;
}

double ArbiterNewSat::GetEstimatedPropagationDelay(int32_t horizontal_hops, int32_t vertical_hops)
{
	return (INTER_ORBIT_PROPAGATION_DELAY_SECONDS * double(horizontal_hops) +
			INTRA_ORBIT_PROPAGATION_DELAY_SECONDS * double(vertical_hops)) /
		   double(ArbiterShortSat::CELL_SCALING_FACTOR);
}

int ArbiterNewSat::TestIfViableHop(int32_t node_id, int32_t previous_hops,
								   std::vector<NeighborCoordContainer::Direction> *direction_sequence, int16_t depth,
								   int32_t target_node_id, int16_t destination_alpha, int16_t destination_gamma)
{
	int32_t next_hops;
	if (depth == 0)
	{
		next_hops = neighbors.GetHopcount(NeighborCoordContainer::SELF, destination_alpha, destination_gamma);
	}
	else
	{
		std::vector<NeighborCoordContainer::Direction> direction_sequence_subset;
		direction_sequence_subset.resize(depth);
		for (int i = 0; i < depth; i++)
		{
			direction_sequence_subset.at(i) = direction_sequence->at(i);
		}
		std::tuple<int16_t, int16_t> coords = neighbors.GetCoordsFromSequence(direction_sequence_subset);
		next_hops = neighbors.GetHopcount(coords, destination_alpha, destination_gamma);
	}

	bool node_is_dest = false;
	std::vector<std::tuple<int32_t, std::tuple<double, double>>> adjacent_satellites =
		m_other_table.at(target_node_id - num_orbits * num_satellites_per_orbit);
	for (std::tuple<int32_t, std::tuple<double, double>> elem : adjacent_satellites)
	{
		if (node_id == std::get<0>(elem))
		{
			node_is_dest = true;
			break;
		}
	}
	if (node_is_dest)
		return 2;
	else if (next_hops < previous_hops)
		return 1;
	else
		return 0;
}
double ArbiterNewSat::GetEstimatedCongestionDelay(std::vector<int32_t> *node_sequence,
												  std::vector<NeighborCoordContainer::Direction> *direction_sequence)
{
	int64_t total_num_congested_nodes = 0;
	for (int i = 0; i < direction_sequence->size(); i++)
	{
		total_num_congested_nodes += (GetSharedState(node_sequence->at(i)) >> (direction_sequence->at(i) - 1)) & 1;
	}
	return (LINK_QUEUE_SIZE * total_num_congested_nodes +
			MINIMUM_FULLNESS_THRESHOLD * (direction_sequence->size() - total_num_congested_nodes)) *
		   1500.0 / double(LINK_BANDWIDTH);
}

std::vector<ArbiterNewSat::path_element> ArbiterNewSat::CreateViablePaths(int32_t start_node_id,
																		  int16_t max_depth_level,
																		  int16_t destination_alpha,
																		  int16_t destination_gamma,
																		  int32_t target_node_id)
{
	std::vector<path_element> viable_paths;
	std::set<int32_t> visited_nodes;

	std::queue<path_element> path_exploration_queue;
	std::vector<NeighborCoordContainer::Direction> d;
	std::vector<int32_t> node_ids = {start_node_id};
	visited_nodes.insert(start_node_id);
	path_exploration_queue.push(std::make_tuple(
		d, node_ids, 0, neighbors.GetHopcount(NeighborCoordContainer::SELF, destination_alpha, destination_gamma), 0,
		0));

	while (!path_exploration_queue.empty())
	{
		std::vector<NeighborCoordContainer::Direction> direction_sequence;
		std::vector<int32_t> node_subpath;
		int16_t depth;
		int32_t previous_hops;
		double _gx, _fx;
		path_element tested_path = path_exploration_queue.front();
		std::tie(direction_sequence, node_subpath, depth, previous_hops, _gx, _fx) = tested_path;
		path_exploration_queue.pop();
		int32_t last_node_in_path = node_subpath.at(node_subpath.size() - 1);

		for (int i = 0; i < 4; i++)
		{
			int32_t neighbor_id = std::get<0>(table_of_node->at(last_node_in_path).at(i));

			std::vector<NeighborCoordContainer::Direction> extended_direction_sequence(direction_sequence);
			extended_direction_sequence.push_back(static_cast<NeighborCoordContainer::Direction>(i + 1));

			int viability = TestIfViableHop(neighbor_id, previous_hops, &extended_direction_sequence, depth + 1,
											target_node_id, destination_alpha, destination_gamma);

			if (viability != 0)
			{
				std::vector<int32_t> extended_node_subpath(node_subpath);
				extended_node_subpath.push_back(neighbor_id);
				std::tuple<int16_t, int16_t> coords = neighbors.GetCoordsFromSequence(extended_direction_sequence);
				int16_t horizontal_hops_from_edge_to_destination, vertical_hops_from_edge_to_destination;
				std::tie(horizontal_hops_from_edge_to_destination, vertical_hops_from_edge_to_destination) =
					neighbors.GetHopcountTuple(coords, destination_alpha, destination_gamma);

				int16_t horizontal_hops_to_edge, vertical_hops_to_edge;
				std::tie(horizontal_hops_to_edge, vertical_hops_to_edge) =
					neighbors.GetHopcountTuple(NeighborCoordContainer::SELF, std::get<0>(coords), std::get<1>(coords));
				double prop_delay_to_edge =
					GetEstimatedPropagationDelay(horizontal_hops_to_edge, vertical_hops_to_edge);
				double congestion_to_edge =
					GetEstimatedCongestionDelay(&extended_node_subpath, &extended_direction_sequence);

				double prop_delay_from_edge_to_destination = GetEstimatedPropagationDelay(
					horizontal_hops_from_edge_to_destination, vertical_hops_from_edge_to_destination);
				// estimate g(x) and f(x)

				if (viability == 1)
				{

					path_element path_to_add = std::make_tuple(
						extended_direction_sequence, extended_node_subpath, depth + 1,
						horizontal_hops_from_edge_to_destination + vertical_hops_from_edge_to_destination,
						prop_delay_to_edge + congestion_to_edge, prop_delay_from_edge_to_destination);
					if (depth + 1 == max_depth_level)
					{
						viable_paths.push_back(path_to_add);
					}
					else
					{
						path_exploration_queue.push(path_to_add);
					}
				}
				if (viability == 2)
				{
					path_element path_to_add = std::make_tuple(
						extended_direction_sequence, extended_node_subpath, depth + 1,
						horizontal_hops_from_edge_to_destination + vertical_hops_from_edge_to_destination,
						prop_delay_to_edge + congestion_to_edge, 0);
					viable_paths.push_back(path_to_add);
				}
			}
		}
	}
	return viable_paths;
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

	std::vector<path_element> paths =
		CreateViablePaths(m_node_id, EXPLORATION_DEPTH, destination_alpha, destination_gamma, target_node_id);

	// direction, direction, is in range, fastpath, propagation delay to this neighbor based on hops + standing queue
	// delay, propagation delay to destination based on hops

	NS_ASSERT_MSG(paths.size() != 0, "no one to forward to");
	NS_LOG_DEBUG(m_node_id << " num viable targets to " << target_node_id << ": " << paths.size());

	std::sort(paths.begin(), paths.end(), [](path_element a, path_element b) {
		return std::get<4>(a) + std::get<5>(a) < std::get<4>(b) + std::get<5>(b);
	});

	std::tuple<int32_t, int32_t, int32_t> if_index = m_neighbor_ids.at(std::get<0>(paths.at(0)).at(0) - 1);
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
