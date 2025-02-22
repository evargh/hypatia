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

#ifndef ARBITER_SHORT_SAT_H
#define ARBITER_SHORT_SAT_H

#include "ns3/abort.h"
#include "ns3/arbiter-satnet.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/udp-header.h"
#include <tuple>
#include <mutex>

namespace ns3
{

class ArbiterShortSat : public ArbiterSatnet
{
  public:
	static const int32_t CELL_SCALING_FACTOR = 5;

	static TypeId GetTypeId(void);

	// Constructor for single forward next-hop forwarding state
	ArbiterShortSat(Ptr<Node> this_node, NodeContainer nodes,
					std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
					std::shared_ptr<std::vector<int64_t>> sdfs, std::shared_ptr<std::mutex> sdfsm,
					std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd);

	// Single forward next-hop implementation
	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(int32_t source_node_id, int32_t target_node_id,
																		 ns3::Ptr<const ns3::Packet> pkt,
																		 ns3::Ipv4Header const &ipHeader,
																		 bool is_socket_request_for_source_ip);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	// These are useful for satellites, which only have one time-varying coordinate under SHORT
	void SetShortParams(double raan, double anomaly);
	// Static routing table
	std::string StringReprOfForwardingState();

	std::tuple<int32_t, int32_t, int32_t> ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg);

	void SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table);
	void SetSharedState(int64_t val);
	int64_t GetSharedState(size_t loc);

  private:
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t alpha_cell, int16_t gamma_cell,
															 int16_t destination_alpha, int16_t destination_gamma);

	int16_t CreateAlphaCell(double a);
	int16_t CreateGammaCell(double g);

	int8_t IncreaseInterface(int16_t a, int16_t b, int16_t base);
	int16_t GetModularDistance(int16_t a, int16_t b, int16_t base);
	bool VerifyInRange(int16_t alpha_cell, int16_t gamma_cell, int16_t destination_alpha, int16_t destination_gamma);

	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_next_hop_list;

	std::vector<std::tuple<double, double, double, double>> m_other_table;
	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_neighbor_ids;

	double m_alpha;
	double m_gamma;
	int64_t num_orbits;
	int64_t num_satellites_per_orbit;

	int16_t alpha_base;
	int16_t gamma_base;

	double left_neighbor_gamma_difference;
	double right_neighbor_gamma_difference;

	std::shared_ptr<std::vector<int64_t>> shared_data_for_satellites;
	std::shared_ptr<std::mutex> shared_data_for_satellites_mutex;
};

} // namespace ns3

#endif // ARBITER_SHORT_SAT_H
