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

#ifndef ARBITER_ELB_SAT_H
#define ARBITER_ELB_SAT_H

#include "ns3/abort.h"
#include "ns3/arbiter-short-sat.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/tcp-header.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/udp-header.h"
#include <tuple>
#include <mutex>

namespace ns3
{

class ArbiterElbSat : public ArbiterShortSat
{
  public:
	static TypeId GetTypeId(void);

	static constexpr double ELB_THETA_S = 0.2;
	static constexpr double MAX_PROPAGATION_DELAY_SECONDS = 0.007;

	typedef std::tuple<NeighborCoordContainer::Direction, int32_t, std::tuple<int8_t, double>> distance_element;

	// Constructor for single forward next-hop forwarding state
	ArbiterElbSat(Ptr<Node> this_node, NodeContainer nodes,
				  std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
				  std::shared_ptr<std::vector<std::tuple<int8_t, double>>> sdfs,
				  std::shared_ptr<std::vector<std::mutex>> sdfsm,
				  std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd,
				  int64_t isl_qsize, int64_t gsl_qsize, int64_t elb_update_interval);

	// Single forward next-hop implementation
	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
		int32_t source_node_id, int32_t target_node_id, ns3::Ptr<const ns3::Packet> pkt,
		ns3::Ipv4Header const &ipHeader, bool is_request_for_source_ip_so_no_next_header);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	std::tuple<int32_t, int32_t, int32_t> ShortDecide(
		std::tuple<int32_t, std::tuple<int16_t, int16_t>> source_satellite_data,
		std::tuple<int32_t, std::tuple<int16_t, int16_t>> destination_satellite_data, int32_t source_node_id,
		int32_t target_node_id);

	void SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table);
	void SetSourceSatelliteTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *table);

	void SetSharedState(std::tuple<int8_t, double> val);
	std::tuple<int8_t, double> GetSharedState(size_t loc);

	void IncrementTxCounter(uint32_t interface_id);
	void IncrementRxCounter(uint32_t interface_id);

	void UpdateELBState();

  private:
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t source_alpha, int16_t source_gamma,
															 int16_t destination_alpha, int16_t destination_gamma,
															 int32_t source_node_id, int32_t target_node_id);

	std::tuple<int32_t, std::tuple<int16_t, int16_t>> ExtractClosestTuple(
		std::vector<std::tuple<int32_t, std::tuple<int16_t, int16_t>>> adjacent_satellites,
		std::tuple<int16_t, int16_t> coords);

	void FlushCounters();

	std::shared_ptr<std::vector<std::tuple<int8_t, double>>> shared_data_for_satellites;
	std::shared_ptr<std::vector<std::mutex>> shared_data_for_satellites_mutex;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *source_satellite_per_flow;

	int64_t m_isl_queue_size_packets, m_gsl_queue_size_packets;
	double elb_update_interval_s;

	std::vector<std::set<int32_t>> interface_head_uids;
	std::vector<std::tuple<double, double, double>> interface_ingress_egress_counters;
	int32_t m_o_counter, m_is_counter, m_it_counter;
	Ptr<UniformRandomVariable> chi_compare;

	bool m_changed;
};

} // namespace ns3

#endif // ARBITER_ELB_SAT_H
