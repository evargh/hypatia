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
							 std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd)
	: ArbiterSatnet(this_node, nodes)
{
	m_next_hop_list = next_hop_list;
	num_orbits = n_o;
	num_satellites_per_orbit = s_p_o;
	m_neighbor_ids = neighbor_ids;

	alpha_base = num_orbits * ArbiterNewSat::CELL_SCALING_FACTOR;
	gamma_base = num_satellites_per_orbit * ArbiterNewSat::CELL_SCALING_FACTOR;

	left_neighbor_gamma_difference = lngd;
	right_neighbor_gamma_difference = rngd;

	shared_data_for_satellites = sdfs;
	shared_data_for_satellites_mutex = sdfsm;
	std::lock_guard<std::mutex> guard(shared_data_for_satellites_mutex->at(m_node_id));
	shared_data_for_satellites->at(m_node_id) = 0;
}

int32_t ArbiterNewSat::GetSquaredEuclideanModularDistance(int16_t alpha_cell, int16_t gamma_cell,
														  int16_t destination_alpha, int16_t destination_gamma)
{
	int16_t alpha_dist = GetModularDistance(alpha_cell, destination_alpha, alpha_base);
	int16_t gamma_dist = GetModularDistance(gamma_cell, destination_gamma, gamma_base);
	return static_cast<int32_t>(alpha_dist) * static_cast<int32_t>(alpha_dist) +
		   static_cast<int32_t>(gamma_dist) * static_cast<int32_t>(gamma_dist);
}

int16_t ArbiterNewSat::GetModularDistance(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	return distance < base - distance ? distance : base - distance;
}

int8_t ArbiterNewSat::IncreaseInterface(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	if (a == b)
		return 0;

	if (a < b)
	{
		if (distance < base - distance)
			return 1;
		else
			return -1;
	}
	else
	{
		if (distance < base - distance)
			return -1;
		else
			return 1;
	}
}

bool ArbiterNewSat::VerifyInRange(int16_t alpha_cell, int16_t gamma_cell, int16_t destination_alpha,
								  int16_t destination_gamma)

{
	return GetModularDistance(alpha_cell, destination_alpha, alpha_base) <= ArbiterNewSat::CELL_SCALING_FACTOR &&
		   GetModularDistance(gamma_cell, destination_gamma, gamma_base) <= ArbiterNewSat::CELL_SCALING_FACTOR;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::DetermineInterface(int16_t alpha_cell, int16_t gamma_cell,
																		int16_t destination_alpha,
																		int16_t destination_gamma,
																		int32_t target_node_id)
{
	// the right orbit will always be CELL_SCALING_FACTOR units to the right of the current satellite, modulo the
	// number of alpha cells
	int16_t right_alpha_cell = (alpha_cell + ArbiterNewSat::CELL_SCALING_FACTOR) % alpha_base;
	// the left orbit will always be CELL_SCALING_FACTOR units to the left of the current satellite, modulo the
	// number of alpha cells
	int16_t left_alpha_cell = (alpha_cell + (num_orbits - 1) * ArbiterNewSat::CELL_SCALING_FACTOR) % alpha_base;

	int16_t up_gamma_cell = (gamma_cell + ArbiterNewSat::CELL_SCALING_FACTOR) % (gamma_base);
	int16_t down_gamma_cell =
		(gamma_cell + (num_satellites_per_orbit - 1) * ArbiterNewSat::CELL_SCALING_FACTOR) % (gamma_base);

	int16_t right_gamma_cell = CreateGammaCell(m_gamma + right_neighbor_gamma_difference);
	int16_t left_gamma_cell = CreateGammaCell(m_gamma + left_neighbor_gamma_difference);

	// if i am within a certain distance, but cant reach the cell, see where the cell is relative to me.
	// if its right and above, both right and up may be valid. if both are, do right
	// if its left and above, both left and up may be valid. if both are, do up
	// if its left and below, both left and down may be valid. if both are, do left
	// if its right and below, both down and right may be valid. if both are, do right

	if (VerifyInRange(alpha_cell, gamma_cell, destination_alpha, destination_gamma))
	{
		if (IncreaseInterface(alpha_cell, destination_alpha, alpha_base) == 0)
		{
			// if its directly up, it may be that the right interface is closer due to
			// phasing. as a result, we counterintuitively route right instead of directly up in order to
			// hit all options. if right fails, then it's taken further up again
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 0)
			{
				NS_ASSERT_MSG(false, "Dropped inaccessible ground station: " << m_node_id << " " << target_node_id);
				return std::make_tuple(-1, -1, -1);
			}
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 1)
			{
				if (VerifyInRange(right_alpha_cell, right_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[3]; // go right (interface 4)
				else if (VerifyInRange(alpha_cell, up_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[2]; // go up (interface 3)
				else
				{
					NS_LOG_DEBUG(alpha_cell << " " << gamma_cell << " -- " << right_alpha_cell << " "
											<< right_gamma_cell);
					NS_ASSERT_MSG(false, "routing failure directly up");
				}
			}
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == -1)
			{
				if (VerifyInRange(left_alpha_cell, left_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[0]; // go left (interface 1)
				else if (VerifyInRange(alpha_cell, down_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[1]; // go down (interface 2)
				else
					NS_ASSERT_MSG(false, "routing failure directly down");
			}
		}
		if (IncreaseInterface(alpha_cell, destination_alpha, alpha_base) == 1)
		{
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 0)
				if (VerifyInRange(right_alpha_cell, right_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[3]; // go right, interface 4
				else
					NS_ASSERT_MSG(false, "routing failure for directly right");

			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 1)
			{
				if (VerifyInRange(right_alpha_cell, right_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[3]; // go right
				else if (VerifyInRange(alpha_cell, up_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[2]; // go up if necessary
				else
					NS_ASSERT_MSG(false,
								  "routing failure for above-right"); // one must be in range for small-scale phasing
			}
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == -1)
			{
				if (VerifyInRange(alpha_cell, down_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[1]; // go down
				else if (VerifyInRange(right_alpha_cell, right_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[3]; // go right if necessary
				else
					NS_ASSERT_MSG(false,
								  "routing failure for below-right"); // one must be in range for small-scale phasing
			}
		}
		if (IncreaseInterface(alpha_cell, destination_alpha, alpha_base) == -1)
		{
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 0)
			{
				// for the same reason as we go right when the point is directly up
				if (VerifyInRange(alpha_cell, up_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[2]; // go left, interface 3
				else
					NS_ASSERT_MSG(false, "routing failure for directly left");
			}
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 1)
			{
				if (VerifyInRange(alpha_cell, up_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[2]; // go up
				else if (VerifyInRange(left_alpha_cell, left_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[0]; // go left if necessary
				else
					NS_ASSERT_MSG(false,
								  "routing failure for above-left"); // one must be in range for small-scale phasing
			}
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == -1)
			{
				if (VerifyInRange(left_alpha_cell, left_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[0]; // go left
				else if (VerifyInRange(alpha_cell, down_gamma_cell, destination_alpha, destination_gamma))
					return m_neighbor_ids[1]; // go down if necessary
				else
					NS_ASSERT_MSG(false,
								  "routing failure for below-left"); // one must be in range for small-scale phasing
			}
		}
	}
	int32_t right_distance =
		GetSquaredEuclideanModularDistance(right_alpha_cell, right_gamma_cell, destination_alpha, destination_gamma);
	int32_t left_distance =
		GetSquaredEuclideanModularDistance(left_alpha_cell, left_gamma_cell, destination_alpha, destination_gamma);
	int32_t up_distance =
		GetSquaredEuclideanModularDistance(alpha_cell, up_gamma_cell, destination_alpha, destination_gamma);
	int32_t down_distance =
		GetSquaredEuclideanModularDistance(alpha_cell, down_gamma_cell, destination_alpha, destination_gamma);

	int32_t current_distance =
		GetSquaredEuclideanModularDistance(alpha_cell, gamma_cell, destination_alpha, destination_gamma);
	// generate a list of all next hops that are closer to the final target, ideally going to a functional approach
	// iterate through that list, up to 0, and check the shared state
	// load balance as a function of those and the distance of the nodes from the target

	// the next hops are constrained under a grid+ topology, there is a small search space of the next few hops.
	// see if the interfaces on the closer hops are central
	// initializer list
	auto distances = {std::make_tuple(1, left_distance, GetSharedState(std::get<0>(m_neighbor_ids[0]))),
					  std::make_tuple(2, down_distance, GetSharedState(std::get<0>(m_neighbor_ids[1]))),
					  std::make_tuple(3, up_distance, GetSharedState(std::get<0>(m_neighbor_ids[2]))),
					  std::make_tuple(4, right_distance, GetSharedState(std::get<0>(m_neighbor_ids[3])))};
	std::vector<std::tuple<int, int32_t, int64_t>> thresholded_distances;
	std::copy_if(distances.begin(), distances.end(), std::back_inserter(thresholded_distances),
				 [current_distance](std::tuple<int, int32_t, int64_t> i) { return std::get<1>(i) < current_distance; });

	std::sort(thresholded_distances.begin(), thresholded_distances.end(),
			  [](std::tuple<int, int32_t, int64_t> a, std::tuple<int, int32_t, int64_t> b) {
				  return (std::get<1>(a) < std::get<1>(b));
			  });

	auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
	return if_index;
	NS_ASSERT_MSG(thresholded_distances.size() != 0, "something incorrect with determinining shortest path");
	for (int i = 0; i < thresholded_distances.size(); i++)
	{
		auto if_index = m_neighbor_ids[std::get<0>(thresholded_distances[0]) - 1];
		if (GetSharedState(std::get<0>(if_index)) == 0)
		{
			return if_index;
		}
	}
	// for distance in thresholded_distances
	// consider their congestion as a binary number, 0 or nonzero
	// if 0 is congested, forward to closest.
	// if 1 is congested, forward to other.
	// if both are congested, forward to closest.

	/*if (min_distance == right_distance)
		  return m_neighbor_ids[3];
	  if (min_distance == left_distance)
		  return m_neighbor_ids[0];
	  if (min_distance == up_distance)
		  return m_neighbor_ids[2];
	  if (min_distance == down_distance)
		  return m_neighbor_ids[1];*/
}

void ArbiterNewSat::SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table)
{
	m_other_table = table;
}

int16_t ArbiterNewSat::CreateAlphaCell(double a)
{
	int16_t alpha_cell = static_cast<int16_t>(std::round(a * alpha_base / 360));
	if (alpha_cell == alpha_base)
		alpha_cell--;
	return alpha_cell;
}

int16_t ArbiterNewSat::CreateGammaCell(double g)
{
	int16_t gamma_cell = static_cast<int16_t>(std::round(g * gamma_base / 360));
	if (gamma_cell == gamma_base)
		gamma_cell--;
	return gamma_cell;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterNewSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg,
																 int32_t target_node_id)
{
	int16_t alpha_cell = CreateAlphaCell(m_alpha);
	int16_t gamma_cell = CreateGammaCell(m_gamma);

	int16_t asc_alpha_distance = GetModularDistance(alpha_cell, aa, alpha_base);
	int16_t desc_alpha_distance = GetModularDistance(alpha_cell, da, alpha_base);

	// if it requires fewer inter-orbit links to go to the ascending alpha, greedily do that
	if (asc_alpha_distance <= desc_alpha_distance)
		return DetermineInterface(alpha_cell, gamma_cell, aa, ag, target_node_id);
	else
		return DetermineInterface(alpha_cell, gamma_cell, da, dg, target_node_id);
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

	int16_t aac = CreateAlphaCell(aa);
	int16_t agc = CreateGammaCell(ag);
	int16_t dac = CreateAlphaCell(da);
	int16_t dgc = CreateGammaCell(dg);

	NS_LOG_DEBUG(aac << " " << agc << " " << dac << " " << dgc);
	NS_ASSERT_MSG(aac >= 0 && aac < alpha_base && agc >= 0 && agc < gamma_base && dac >= 0 && dac < alpha_base &&
					  dgc >= 0 && dgc < gamma_base,
				  "invalid cells");
	return ShortDecide(aac, agc, dac, dgc, target_node_id);
}

void ArbiterNewSat::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id,
										  int32_t next_if_id)
{
	NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
	m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterNewSat::SetShortParams(double alpha, double gamma)
{
	NS_LOG_DEBUG(m_node_id << ": alpha - " << alpha << " - gamma - " << gamma);
	m_alpha = alpha;
	m_gamma = gamma;
}

std::string ArbiterNewSat::StringReprOfForwardingState()
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
