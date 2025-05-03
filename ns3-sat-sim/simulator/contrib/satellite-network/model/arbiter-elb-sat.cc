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

#include "arbiter-elb-sat.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterElbSat");
NS_OBJECT_ENSURE_REGISTERED(ArbiterElbSat);
TypeId ArbiterElbSat::GetTypeId(void)
{
	static TypeId tid = TypeId("ns3::ArbiterElbSat").SetParent<ArbiterSatnet>().SetGroupName("BasicSim");
	return tid;
}

ArbiterElbSat::ArbiterElbSat(Ptr<Node> this_node, NodeContainer nodes,
							 std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o,
							 int64_t s_p_o, std::shared_ptr<std::vector<std::tuple<int8_t, double>>> sdfs,
							 std::shared_ptr<std::vector<std::mutex>> sdfsm,
							 std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd,
							 int64_t isl_qsize, int64_t gsl_qsize, int64_t elb_update_interval)
	: ArbiterShortSat(this_node, nodes, next_hop_list, n_o, s_p_o, neighbor_ids, lngd, rngd),
	  m_isl_queue_size_packets(isl_qsize), m_gsl_queue_size_packets(gsl_qsize), m_o_counter(0), m_is_counter(0),
	  m_it_counter(0), elb_update_interval_s(double(elb_update_interval) / 1000000000), m_changed(false)
{
	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;

	chi_compare = CreateObject<UniformRandomVariable>();
	chi_compare->SetAttribute("Min", DoubleValue(0.0));
	chi_compare->SetAttribute("Max", DoubleValue(1.0));

	interface_head_uids.resize(5);
	interface_ingress_egress_counters.resize(5);
}

std::tuple<int32_t, int32_t, int32_t> ArbiterElbSat::DetermineInterface(int16_t source_alpha, int16_t source_gamma,
																		int16_t destination_alpha,
																		int16_t destination_gamma,
																		int32_t source_node_id, int32_t target_node_id)
{
	int32_t right_hops = neighbors.GetHopcount(NeighborCoordContainer::RIGHT, destination_alpha, destination_gamma);
	int32_t left_hops = neighbors.GetHopcount(NeighborCoordContainer::LEFT, destination_alpha, destination_gamma);
	int32_t up_hops = neighbors.GetHopcount(NeighborCoordContainer::UP, destination_alpha, destination_gamma);
	int32_t down_hops = neighbors.GetHopcount(NeighborCoordContainer::DOWN, destination_alpha, destination_gamma);

	int32_t current_hops = neighbors.GetHopcount(NeighborCoordContainer::SELF, destination_alpha, destination_gamma);

	std::vector<ArbiterElbSat::distance_element> distances = {
		std::make_tuple(NeighborCoordContainer::LEFT, left_hops,
						GetSharedState(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::LEFT - 1)))),
		std::make_tuple(NeighborCoordContainer::DOWN, down_hops,
						GetSharedState(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::DOWN - 1)))),
		std::make_tuple(NeighborCoordContainer::UP, up_hops,
						GetSharedState(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::UP - 1)))),
		std::make_tuple(NeighborCoordContainer::RIGHT, right_hops,
						GetSharedState(std::get<0>(m_neighbor_ids.at(NeighborCoordContainer::RIGHT - 1))))};

	std::vector<ArbiterElbSat::distance_element> thresholded_distances;

	for (auto elem : distances)
	{
		if (std::get<1>(elem) < current_hops)
		{
			thresholded_distances.push_back(elem);
		}
	}

	// ELB argues that satellites should consider "alternate routes," but provides no specification about what those
	// alternate routes should be. It argues for the usage of a metric which considers congestion as well as propagation
	// delay, but doesn't say what to do with that metric
	//
	// As a result, I'm designing what I think is a reasonable implementation in this way:
	//    packets are forwarded deterministically along the 1 or 2 best paths
	//    if there is 1 satellite on the best path:
	//      forward through it even if it gets busy
	//    if there are 2 satellites on the best path:
	//      if that satellite gets busy, 1 - chi is forwarded to the second best path
	//      if both satellites get busy, use the instantaneous queueing delay metric that's provided to pick one of
	//      them.
	//    if there are 3 or more, isolate the top 2 (sorted by hop count)
	//
	// The stated goal of the metric seems to be to avoid loops. If this is already done by other tools, I won't use the
	// metric.

	if (thresholded_distances.size() == 1)
	{
		auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
		return if_index;
	}
	else if (thresholded_distances.size() >= 2)
	{
		std::sort(thresholded_distances.begin(), thresholded_distances.end(),
				  [](ArbiterElbSat::distance_element a, ArbiterElbSat::distance_element b) {
					  return std::get<1>(a) < std::get<1>(b);
				  });
		std::tuple<int8_t, double> busyness_0 = std::get<2>(thresholded_distances.at(0));
		std::tuple<int8_t, double> busyness_1 = std::get<2>(thresholded_distances.at(1));
		if (std::get<0>(busyness_0) == 2 && std::get<0>(busyness_1) == 2)
		{
			double value = chi_compare->GetValue();
			if (value < 0.5)
			{
				auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances.at(0)) - 1];
				return if_index;
			}
			else
			{
				auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances.at(1)) - 1];
				return if_index;
			}
		}
		else if (std::get<0>(busyness_0) == 2)
		{
			double chi = std::get<1>(busyness_0);

			double value = chi_compare->GetValue();
			if (value < chi)
			{
				auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances.at(0)) - 1];
				return if_index;
			}
			else
			{
				auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances.at(1)) - 1];
				return if_index;
			}
		}
		else if (std::get<0>(busyness_1) == 2)
		{
			double chi = std::get<1>(busyness_1);

			double value = chi_compare->GetValue();
			if (value < chi)
			{
				auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances.at(1)) - 1];
				return if_index;
			}
			else
			{
				auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances.at(0)) - 1];
				return if_index;
			}
		}
		else
		{
			auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
			return if_index;
		}
	}
	NS_LOG_DEBUG("no viable paths, dropping");
	return std::make_tuple(-1, -1, -1);
}

std::tuple<int32_t, std::tuple<int16_t, int16_t>> ArbiterElbSat::ExtractClosestTuple(
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

std::tuple<int32_t, int32_t, int32_t> ArbiterElbSat::ShortDecide(
	std::tuple<int32_t, std::tuple<int16_t, int16_t>> source_satellite_data,
	std::tuple<int32_t, std::tuple<int16_t, int16_t>> destination_satellite_data, int32_t source_node_id,
	int32_t target_node_id)
{
	return DetermineInterface(std::get<0>(std::get<1>(source_satellite_data)),
							  std::get<1>(std::get<1>(source_satellite_data)),
							  std::get<0>(std::get<1>(destination_satellite_data)),
							  std::get<1>(std::get<1>(destination_satellite_data)), source_node_id, target_node_id);
}

void ArbiterElbSat::SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table)
{
	m_other_table = table;
}

void ArbiterElbSat::SetSourceSatelliteTable(
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *table)
{
	source_satellite_per_flow = table;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterElbSat::TopologySatelliteNetworkDecide(
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

void ArbiterElbSat::UpdateELBState()
{
	if (m_changed)
	{
		std::vector<int64_t> queue_capacities = {m_isl_queue_size_packets, m_isl_queue_size_packets,
												 m_isl_queue_size_packets, m_isl_queue_size_packets,
												 m_gsl_queue_size_packets};
		std::vector<int64_t> queue_occupancies;
		queue_occupancies.resize(5);
		// implementation decision: ELB assumes that satellites just have one queue shared by all of their interfaces
		// we can model this by iterating over all net devices attached to the node
		uint32_t num_interfaces = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetNInterfaces();
		// 0 is loopback, so skip that
		for (int i = 1; i < num_interfaces; i++)
		{
			Ptr<NetDevice> outgoing_if = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetNetDevice(i);
			if (outgoing_if->GetInstanceTypeId() == TypeId::LookupByName("ns3::PointToPointLaserNetDevice"))
			{
				queue_occupancies.at(i - 1) =
					outgoing_if->GetObject<PointToPointLaserNetDevice>()->GetQueue()->GetNPackets();
			}
			if (outgoing_if->GetInstanceTypeId() == TypeId::LookupByName("ns3::GSLNetDevice"))
			{
				queue_occupancies.at(i - 1) = outgoing_if->GetObject<GSLNetDevice>()->GetQueue()->GetNPackets();
			}
		}
		std::vector<double> i_minus_o_per_interface;
		i_minus_o_per_interface.reserve(5);
		std::transform(interface_ingress_egress_counters.begin(), interface_ingress_egress_counters.end(),
					   std::back_inserter(i_minus_o_per_interface), [](std::tuple<double, double, double> x) {
						   return std::max(double(std::get<0>(x)) + double(std::get<1>(x)) - double(std::get<2>(x)),
										   0.0);
					   });
		/*for (int i = 0; i < i_minus_o_per_interface.size(); i++)
		{
			NS_LOG_DEBUG("i minus o really " << std::get<0>(interface_ingress_egress_counters.at(i)) +
													std::get<1>(interface_ingress_egress_counters.at(i)) -
													std::get<2>(interface_ingress_egress_counters.at(i))
											 << ": " << i_minus_o_per_interface.at(i));
		}*/
		std::tuple<int8_t, double> current_state = GetSharedState(m_node_id);
		std::vector<double> queue_fullness_ratios, betas, alphas;
		queue_fullness_ratios.resize(5);
		betas.resize(5);
		alphas.resize(5);
		bool level_one_congestion = false;
		bool level_two_congestion = false;
		int32_t problem_interface = -1;
		for (int i = 0; i < queue_occupancies.size(); i++)
		{
			queue_fullness_ratios.at(i) = double(queue_occupancies.at(i)) / queue_capacities.at(i);
			// uncaught divide byzero error in case the queue capacity is equal to the queue occupancy
			// in that case, just set it to 1
			double delta_d_inverse = 0;
			if (queue_capacities.at(i) != queue_occupancies.at(i))
			{
				delta_d_inverse =
					double(i_minus_o_per_interface.at(i)) / ((queue_capacities.at(i) - queue_occupancies.at(i)) * 1320);
			}
			else
			{
				delta_d_inverse = double(i_minus_o_per_interface.at(i)) / 1320;
			}
			betas.at(i) = 1 - std::min(1.0, delta_d_inverse *
												(elb_update_interval_s + ArbiterElbSat::MAX_PROPAGATION_DELAY_SECONDS));
			alphas.at(i) = betas.at(i) / 2;
			if (queue_fullness_ratios.at(i) > betas.at(i))
			{
				level_two_congestion = true;
				problem_interface = i;
			}
			else if (queue_fullness_ratios.at(i) > alphas.at(i))
			{
				level_one_congestion = true;
			}
			/*NS_LOG_DEBUG(m_node_id << " interface " << i << ": " << " i minus o: " << i_minus_o_per_interface.at(i)
								   << " queue occupancy: " << queue_occupancies.at(i) << " qf ratio: "
								   << queue_fullness_ratios.at(i) << " delta d inverse: " << delta_d_inverse
								   << " beta: " << betas.at(i) << " alpha: " << alphas.at(i));*/
		}
		// the other terms can be directly plugged in to the formula
		if (level_two_congestion)
		{
			double qtbsa = std::min(double(queue_capacities.at(problem_interface)),
									double(queue_capacities.at(problem_interface)) * betas.at(problem_interface) +
										ArbiterElbSat::MAX_PROPAGATION_DELAY_SECONDS *
											(i_minus_o_per_interface.at(problem_interface)) / 1320.0);

			double isnew =
				1320.0 * (qtbsa - queue_capacities.at(problem_interface) * alphas.at(problem_interface)) / ELB_THETA_S +
				std::get<2>(interface_ingress_egress_counters.at(problem_interface)) -
				std::get<1>(interface_ingress_egress_counters.at(problem_interface));
			double chi = std::min(
				std::max(0.0, isnew / std::get<0>(interface_ingress_egress_counters.at(problem_interface))), 1.0);
			// NS_LOG_DEBUG("chi is " << chi);
			NS_LOG_DEBUG("congestion change to beta (chi changed)");
			SetSharedState(std::make_tuple(2, chi));
		}
		else if (level_one_congestion)
		{
			if (std::get<0>(current_state) != 1)
			{
				NS_LOG_DEBUG("congestion change to alpha");
			}
			SetSharedState(std::make_tuple(1, 0));
		}
		else
		{
			if (std::get<0>(current_state) != 0)
			{
				NS_LOG_DEBUG("congestion change to uncongested");
			}
			SetSharedState(std::make_tuple(0, 0));
		}

		FlushCounters();
		m_changed = false;
	}
}

void ArbiterElbSat::IncrementTxCounter(uint32_t interface_id)
{
	std::get<2>(interface_ingress_egress_counters.at(interface_id)) += 1320 / elb_update_interval_s;

	/*NS_LOG_DEBUG(m_node_id << "interface: " << interface_id
						   << " ingress/egress: " << std::get<0>(interface_ingress_egress_counters.at(interface_id))
						   << ", " << std::get<1>(interface_ingress_egress_counters.at(interface_id)));*/
	m_changed = true;
}

void ArbiterElbSat::IncrementRxCounter(uint32_t interface_id)
{
	// increment
	if (interface_id == 4)
	{
		std::get<1>(interface_ingress_egress_counters.at(interface_id)) += 1320 / elb_update_interval_s;
	}
	else
	{
		std::get<0>(interface_ingress_egress_counters.at(interface_id)) += 1320 / elb_update_interval_s;
	}
	/*NS_LOG_DEBUG(m_node_id << "interface: " << interface_id
						   << " ingress/egress: " << std::get<0>(interface_ingress_egress_counters.at(interface_id))
						   << ", " << std::get<1>(interface_ingress_egress_counters.at(interface_id)));*/
	m_changed = true;
}

void ArbiterElbSat::SetSharedState(std::tuple<int8_t, double> val)
{
	NS_ASSERT_MSG(std::get<0>(val) >= 0 && std::get<0>(val) <= 2, "Invalid Shared State Value");
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	shared_data_for_satellites->at(m_node_id) = val;
}

std::tuple<int8_t, double> ArbiterElbSat::GetSharedState(size_t loc)
{
	NS_ASSERT_MSG(loc >= 0 && loc < num_orbits * num_satellites_per_orbit, "Incorrect Index Access");
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(loc));
	return shared_data_for_satellites->at(loc);
}

void ArbiterElbSat::FlushCounters()
{
	m_o_counter = m_is_counter = m_it_counter = 0;
	for (int i = 0; i < interface_ingress_egress_counters.size(); i++)
	{
		std::get<0>(interface_ingress_egress_counters.at(i)) = 0;
		std::get<1>(interface_ingress_egress_counters.at(i)) = 0;
		std::get<2>(interface_ingress_egress_counters.at(i)) = 0;
	}
}

void ArbiterElbSat::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										  int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}
} // namespace ns3
