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

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::DetermineInterface(int16_t destination_alpha,
																		int16_t destination_gamma,
																		int32_t target_node_id)
{
	if (neighbors.VerifyInRange(NeighborCoordContainer::SELF, destination_alpha, destination_gamma))
	{
		return HandleClose(destination_alpha, destination_gamma, target_node_id);
	}
	int32_t current_distance = neighbors.GetSquaredEuclideanModularDistance(NeighborCoordContainer::SELF,
																			destination_alpha, destination_gamma);
	// generate a list of all next hops that are closer to the final target, ideally going to a functional approach
	// iterate through that list, up to 0, and check the shared state
	// load balance as a function of those and the distance of the nodes from the target

	// the next hops are constrained under a grid+ topology, there is a small search space of the next few hops.
	//
	// the sketch:
	//    first, look at all the nodes that are one hop away
	//    see which of their neighbors are closer than both you and your neighbor to the destination
	//    see which interfaces go to those neighbors
	//    forward to the satellite that posesses an uncongested interface that goes to the closest satellite
	// store the first direction, the second direction, and then all the congestion information
	typedef std::tuple<NeighborCoordContainer::Direction, NeighborCoordContainer::Direction, bool, int32_t, int64_t,
					   int64_t>
		distance_element;

	std::set<distance_element> distances;

	// LEFT, DOWN, UP, RIGHT
	for (int neighbor_idx = 1; neighbor_idx < 5; neighbor_idx++)
	{
		// for each neighbor
		std::vector<NeighborCoordContainer::Direction> direction_sequence;
		direction_sequence.resize(2);
		direction_sequence.at(0) = static_cast<NeighborCoordContainer::Direction>(neighbor_idx);
		for (int neighbor_of_neighbor_idx = 1; neighbor_of_neighbor_idx < 5; neighbor_of_neighbor_idx++)
		{
			// look at their neighbors
			int32_t neighbor_neighbor_id =
				std::get<0>(m_neighbor_neighbors.at(neighbor_idx - 1).at(neighbor_of_neighbor_idx - 1));
			if (neighbor_neighbor_id != m_node_id)
			{
				direction_sequence.at(1) = static_cast<NeighborCoordContainer::Direction>(neighbor_of_neighbor_idx);
				int64_t neighbor_congestion = GetSharedState(std::get<0>(m_neighbor_ids.at(neighbor_idx - 1)));
				int64_t neighbor_neighbor_congestion = GetSharedState(neighbor_neighbor_id);

				distance_element this_dist = std::make_tuple(
					static_cast<NeighborCoordContainer::Direction>(neighbor_idx),
					static_cast<NeighborCoordContainer::Direction>(neighbor_of_neighbor_idx),
					//    use the fact that the gamma/alpha differentials are constant to get the position of their
					//    neighbors
					neighbors.VerifyInRange(direction_sequence.at(0), destination_alpha, destination_gamma),
					neighbors.GetSquaredEuclideanModularDistance(neighbors.GetCoordsFromSequence(direction_sequence),
																 destination_alpha, destination_gamma),
					(neighbor_congestion >> (neighbor_idx - 1)),
					(neighbor_neighbor_congestion >> (neighbor_of_neighbor_idx - 1)));

				distances.emplace(this_dist);
			}
			// get their position
		}
	}
	// for each satellite in your list of neighbors
	//    combine your own interface congestion information, and neighbor congestion information, to determine likely
	//    congestion along that path forward to the most uncongested interface possible. if theres a tie, forward to the
	//    closer interface
	//

	std::vector<distance_element> thresholded_distances;
	std::copy_if(
		distances.begin(), distances.end(), std::back_inserter(thresholded_distances),
		[current_distance](distance_element i) { return std::get<2>(i) || (std::get<3>(i) < current_distance); });

	std::sort(thresholded_distances.begin(), thresholded_distances.end(), [](distance_element a, distance_element b) {
		if (std::get<2>(a) && !std::get<2>(b))
			return true;
		else
			return (std::get<3>(a)) < std::get<3>(b);
	});
	NS_LOG_DEBUG(m_node_id << " num viable targets to " << target_node_id << ": " << thresholded_distances.size());
	// consider which interface to use here, can be more complex than this
	for (int i = 0; i < thresholded_distances.size(); i++)
	{
		// first, identify the interfaces that go directly to the loop
		// if this condition is true, we have direct access
		if (std::get<2>(thresholded_distances.at(0)))
		{
			int option = 0;
			while (option < thresholded_distances.size() && std::get<2>(thresholded_distances.at(option)))
			{
				if (std::get<4>(thresholded_distances.at(option)) == 0)
				{
					auto if_index = m_neighbor_ids.at(std::get<0>(thresholded_distances.at(option)) - 1);
					return if_index;
				}
				option += 1;
			}
			// if all of my interfaces to that node are congested, just pick the first one

			auto if_index = m_neighbor_ids.at(std::get<0>(thresholded_distances.at(0)) - 1);
			return if_index;
		}
		// if we don't have that, then we optimize based on two hop distance
		// refactor this to be smarter later, this is just a test for now
		int option = 0;
		while (option < thresholded_distances.size())
		{
			if (std::get<4>(thresholded_distances.at(option)) + std::get<5>(thresholded_distances.at(option)) == 0)
			{
				auto if_index = m_neighbor_ids.at(std::get<0>(thresholded_distances.at(option)) - 1);
				return if_index;
			}
			option += 1;
		}
		option = 0;
		while (option < thresholded_distances.size())
		{
			if (std::get<4>(thresholded_distances.at(option)) + std::get<5>(thresholded_distances.at(option)) == 1)
			{
				auto if_index = m_neighbor_ids.at(std::get<0>(thresholded_distances.at(option)) - 1);
				return if_index;
			}
			option += 1;
		}
		option = 0;
		while (option < thresholded_distances.size())
		{
			if (std::get<4>(thresholded_distances.at(option)) + std::get<5>(thresholded_distances.at(option)) == 2)
			{
				auto if_index = m_neighbor_ids.at(std::get<0>(thresholded_distances.at(option)) - 1);
				return if_index;
			}
			option += 1;
		}
	}
	NS_ASSERT_MSG(thresholded_distances.size() != 0, "something incorrect with determinining shortest path");
	auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
	return if_index;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg,
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

void ArbiterNewSat::SetInterfaceCongestionBits()
{
	// when an interface is found to be at an intolerable level, record that timestamp
	// if the interface goes under `interval` in that timestamp, its not congested
	// otherwise, set it as congested, and keep it as congested until the queue goes under intolerable levels
	for (auto i : m_neighbor_ids)
	{
		int32_t id, outgoing_if, dummy;
		std::tie(id, outgoing_if, dummy) = i;
		auto num_packets = m_nodes.Get(m_node_id)
							   ->GetObject<Ipv4>()
							   ->GetNetDevice(outgoing_if)
							   ->GetObject<PointToPointLaserNetDevice>()
							   ->GetQueue()
							   ->GetNPackets();
		bool under_target = num_packets < ArbiterNewSat::MINIMUM_FULLNESS_THRESHOLD;
		if (under_target)
		{
			// update the timestamp regardless
			m_interface_congestion_timer.at(outgoing_if - 1) = Simulator::Now().GetNanoSeconds();
			// writing is not multithreaded
			// if the node is not currently marked as congested
			if ((GetSharedState(m_node_id) >> (outgoing_if - 1) & 1) == 1)
			{
				auto newval = GetSharedState(m_node_id) - (1 << (outgoing_if - 1));
				SetSharedState(newval);
			}
		}
		// if the node is currently marked as uncongested
		else if ((GetSharedState(m_node_id) >> (outgoing_if - 1) & 1) == 0 &&
				 Simulator::Now().GetNanoSeconds() >
					 m_interface_congestion_timer.at(outgoing_if - 1) + ArbiterNewSat::MINIMUM_FULLNESS_INTERVAL_NS)
		{
			auto newval = GetSharedState(m_node_id) + (1 << (outgoing_if - 1));
			SetSharedState(newval);
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

	return ShortDecide(aac, agc, dac, dgc, target_node_id);
}

void ArbiterNewSat::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										  int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterNewSat::SetSharedState(int64_t val)
{
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	shared_data_for_satellites->at(m_node_id) = val;
}

int64_t ArbiterNewSat::GetSharedState(size_t loc)
{
	if (loc < num_orbits * num_satellites_per_orbit)
	{
		std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(loc));
		return shared_data_for_satellites->at(loc);
	}
	else
	{
		return -1;
	}
}

} // namespace ns3
