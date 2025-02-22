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
								 std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd,
								 double rngd)
	: ArbiterSatnet(this_node, nodes)
{
	m_next_hop_list = next_hop_list;
	num_orbits = n_o;
	num_satellites_per_orbit = s_p_o;
	m_neighbor_ids = neighbor_ids;

	alpha_base = num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR;
	gamma_base = num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR;

	left_neighbor_gamma_difference = lngd;
	right_neighbor_gamma_difference = rngd;

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

int8_t ArbiterShortSat::IncreaseInterface(int16_t a, int16_t b, int16_t base)
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

bool ArbiterShortSat::VerifyInRange(int16_t alpha_cell, int16_t gamma_cell, int16_t destination_alpha,
									int16_t destination_gamma)

{
	return GetModularDistance(alpha_cell, destination_alpha, alpha_base) <= ArbiterShortSat::CELL_SCALING_FACTOR &&
		   GetModularDistance(gamma_cell, destination_gamma, gamma_base) <= ArbiterShortSat::CELL_SCALING_FACTOR;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::DetermineInterface(int16_t alpha_cell, int16_t gamma_cell,
																		  int16_t destination_alpha,
																		  int16_t destination_gamma)
{
	// the right orbit will always be CELL_SCALING_FACTOR units to the right of the current satellite, modulo the
	// number of alpha cells
	int16_t right_alpha_cell = (alpha_cell + ArbiterShortSat::CELL_SCALING_FACTOR) % alpha_base;
	// the left orbit will always be CELL_SCALING_FACTOR units to the left of the current satellite, modulo the
	// number of alpha cells
	int16_t left_alpha_cell = (alpha_cell + (num_orbits - 1) * ArbiterShortSat::CELL_SCALING_FACTOR) % alpha_base;

	int16_t up_gamma_cell = (gamma_cell + ArbiterShortSat::CELL_SCALING_FACTOR) % (gamma_base);
	int16_t down_gamma_cell =
		(gamma_cell + (num_satellites_per_orbit - 1) * ArbiterShortSat::CELL_SCALING_FACTOR) % (gamma_base);

	// if i am within a certain distance, but cant reach the cell, see where the cell is relative to me.
	// if its right and above, both right and up may be valid. if both are, do right
	// if its left and above, both left and up may be valid. if both are, do up
	// if its left and below, both left and down may be valid. if both are, do left
	// if its right and below, both down and right may be valid. if both are, do right

	if (VerifyInRange(alpha_cell, gamma_cell, destination_alpha, destination_gamma))
	{
		int16_t right_gamma_cell = CreateGammaCell(m_gamma + right_neighbor_gamma_difference);
		int16_t left_gamma_cell = CreateGammaCell(m_gamma + left_neighbor_gamma_difference);

		if (IncreaseInterface(alpha_cell, destination_alpha, alpha_base) == 0)
		{
			// if its directly up, it may be that the right interface is closer due to
			// phasing. as a result, we counterintuitively route right instead of directly up in order to
			// hit all options. if right fails, then it's taken further up again
			if (IncreaseInterface(gamma_cell, destination_gamma, gamma_base) == 0)
			{
				NS_ASSERT_MSG(false, "Dropped inaccessible ground station: " << m_node_id << " a: " << destination_alpha
																			 << " g: " << destination_gamma);
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
	// capture whether the right or left orbit is closer
	bool right_shorter = GetModularDistance(right_alpha_cell, destination_alpha, alpha_base) <
						 GetModularDistance(alpha_cell, destination_alpha, alpha_base);

	bool left_shorter = GetModularDistance(left_alpha_cell, destination_alpha, alpha_base) <
						GetModularDistance(alpha_cell, destination_alpha, alpha_base);

	if (right_shorter)
		return m_neighbor_ids[3]; // go right (interface 4)
	else if (left_shorter)
		return m_neighbor_ids[0]; // go left (interface 1)
	else if (!right_shorter && !left_shorter)
	{
		// if neither option is strictly closer, then we just use gamma
		bool up_shorter = GetModularDistance(up_gamma_cell, destination_gamma, gamma_base) <
						  GetModularDistance(gamma_cell, destination_gamma, gamma_base);

		bool down_shorter = GetModularDistance(down_gamma_cell, destination_gamma, gamma_base) <
							GetModularDistance(gamma_cell, destination_gamma, gamma_base);

		if (up_shorter)
			return m_neighbor_ids[2]; // go up (interface 3)
		else if (down_shorter)
			return m_neighbor_ids[1]; // go down (interface 2)
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

void ArbiterShortSat::SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table)
{
	m_other_table = table;
}

int16_t ArbiterShortSat::CreateAlphaCell(double a)
{
	int16_t alpha_cell = static_cast<int16_t>(std::round(a * alpha_base / 360));
	if (alpha_cell == alpha_base)
		alpha_cell--;
	return alpha_cell;
}

int16_t ArbiterShortSat::CreateGammaCell(double g)
{

	int16_t gamma_cell = static_cast<int16_t>(std::round(g * gamma_base / 360));
	if (gamma_cell == gamma_base)
		gamma_cell--;
	return gamma_cell;
}

std::tuple<int32_t, int32_t, int32_t> ArbiterShortSat::ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg)
{
	int16_t alpha_cell = CreateAlphaCell(m_alpha);

	int16_t gamma_cell = CreateGammaCell(m_gamma);

	int16_t asc_alpha_distance = GetModularDistance(alpha_cell, aa, alpha_base);
	int16_t desc_alpha_distance = GetModularDistance(alpha_cell, da, gamma_base);

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
	return ShortDecide(aac, agc, dac, dgc);
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
