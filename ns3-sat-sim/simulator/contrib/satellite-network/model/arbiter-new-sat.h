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

#ifndef ARBITER_NEW_SAT_H
#define ARBITER_NEW_SAT_H

#include "ns3/abort.h"
#include "ns3/arbiter-short-sat.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/topology-satellite-network.h"
#include <tuple>
#include <mutex>

namespace ns3
{

class ArbiterNewSat : public ArbiterShortSat
{
  public:
	static const int32_t MINIMUM_FULLNESS_THRESHOLD = 30;
	static const int64_t MINIMUM_FULLNESS_INTERVAL_NS = 100000000;
	static const int32_t LINK_QUEUE_SIZE = 120;
	static const int64_t LINK_BANDWIDTH = 10000000;
	static constexpr float INTER_ORBIT_PROPAGATION_DELAY_SECONDS = 0.003;
	static constexpr float INTRA_ORBIT_PROPAGATION_DELAY_SECONDS = 0.007;

	// direction, direction, is in range, fastpath, normal_distance, propagation delay to this neighbor based on hops +
	// standing queue delay, propagation delay to destination based on hops
	typedef std::tuple<NeighborCoordContainer::Direction, NeighborCoordContainer::Direction, bool, bool, float, float>
		distance_element;
	static TypeId GetTypeId(void);

	// Constructor for single forward next-hop forwarding state
	ArbiterNewSat(Ptr<Node> this_node, NodeContainer nodes,
				  std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
				  std::shared_ptr<std::vector<int64_t>> sdfs, std::shared_ptr<std::vector<std::mutex>> sdfsm,
				  std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids,
				  std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> neighbor_neighbors, double lngd,
				  double rngd);

	// Single forward next-hop implementation
	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
		int32_t source_node_id, int32_t target_node_id, ns3::Ptr<const ns3::Packet> pkt,
		ns3::Ipv4Header const &ipHeader, bool is_request_for_source_ip_so_no_next_header);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	std::tuple<int32_t, int32_t, int32_t> ShortDecide(std::vector<std::tuple<int16_t, int16_t>> adjacent_satellites,
													  int32_t target_node_id);
	void SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table);

	void SetSharedState(int64_t val);
	int64_t GetSharedState(size_t loc);

  private:
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t destination_alpha, int16_t destination_gamma,
															 int32_t target_node_id);
	std::vector<distance_element> PopulateDistances(int32_t current_hops, int16_t destination_alpha,
													int16_t destination_gamma);

	void SetInterfaceCongestionBits();
	float GetEstimatedPropagationDelay(int32_t horizontal_hops, int32_t vertical_hops);

	std::array<int64_t, 4> m_interface_congestion_timer;
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> m_neighbor_neighbors;
	std::shared_ptr<std::vector<int64_t>> shared_data_for_satellites;
	std::shared_ptr<std::vector<std::mutex>> shared_data_for_satellites_mutex;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;
};

} // namespace ns3

#endif // ARBITER_NEW_SAT_H
