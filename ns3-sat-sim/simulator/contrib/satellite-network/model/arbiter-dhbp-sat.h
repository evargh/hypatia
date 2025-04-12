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

#ifndef ARBITER_DHBP_SAT_H
#define ARBITER_DHBP_SAT_H

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

class ArbiterDhbpSat : public ArbiterShortSat
{
  public:
	static TypeId GetTypeId(void);
	const bool OPTIMIZED = true;

	typedef std::tuple<NeighborCoordContainer::Direction, int32_t, int64_t> distance_element;

	// Constructor for single forward next-hop forwarding state
	ArbiterDhbpSat(Ptr<Node> this_node, NodeContainer nodes,
				   std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
				   std::shared_ptr<std::vector<std::vector<int64_t>>> sdfs,
				   std::shared_ptr<std::vector<std::mutex>> sdfsm,
				   std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd);

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

	void IncreaseQueue(int32_t source_node_id, int32_t target_node_id);
	void DecreaseQueue(int32_t source_node_id, int32_t target_node_id);
	int64_t GetQueueSizeForFlow(int32_t neighbor_id, int32_t source_node_id, int32_t target_node_id);

  private:
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t source_alpha, int16_t source_gamma,
															 int16_t destination_alpha, int16_t destination_gamma,
															 int32_t source_node_id, int32_t target_node_id);

	std::tuple<int32_t, std::tuple<int16_t, int16_t>> ExtractClosestTuple(
		std::vector<std::tuple<int32_t, std::tuple<int16_t, int16_t>>> adjacent_satellites,
		std::tuple<int16_t, int16_t> coords);
	std::tuple<int32_t, std::tuple<int16_t, int16_t>> ExtractClosestTupleCreatingRectangle(
		std::vector<std::tuple<int32_t, std::tuple<int16_t, int16_t>>> adjacent_satellites,
		std::tuple<int16_t, int16_t> destination_coords, std::tuple<int16_t, int16_t> my_coords);
	// std::tuple<int8_t, int8_t, int8_t, int8_t> CompareSourceDest(int16_t source_alpha, int16_t source_gamma,
	//															 int16_t destination_alpha, int16_t destination_gamma);
	// bool CheckIfInRectangle(NeighborCoordContainer::Direction d, int16_t source_alpha, int16_t source_gamma,
	//						int16_t destination_alpha, int16_t destination_gamma);
	int32_t GSLNodeIdToGSLIndex(int32_t id);
	int32_t GSLIndexToGSLNodeId(int32_t id);
	int64_t GetFlowMapping(int32_t source_node_id, int32_t target_node_id);

	std::shared_ptr<std::vector<std::vector<int64_t>>> shared_data_for_satellites;
	std::shared_ptr<std::vector<std::mutex>> shared_data_for_satellites_mutex;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *source_satellite_per_flow;
};

} // namespace ns3

#endif // ARBITER_DHBP_SAT_H
