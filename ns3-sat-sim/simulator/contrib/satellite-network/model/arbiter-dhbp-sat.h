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
	// OPTIMIZED is a boolean which switches between an optimization of DHBP, which delivered better performance, and
	// the original specification of DHBP, which performed poorly.
	const bool OPTIMIZED = true;

	typedef std::tuple<NeighborCoordContainer::Direction, int32_t, int64_t> distance_element;

	/*
	 * \param this_node smart pointer to the node that owns this arbiter
	 * \param nodes container of the other nodes
	 * \param next_hop_list arbiters require an initialized list of interfaces on initialization
	 * \param n_o number of orbits
	 * \param s_p_o number of satellites per orbit
	 * \param sdfs data structure for shared data (abstracts satellite communication)
	 * \param sdfsm data structure for mutexes
	 * \param neighbor_ids table of the interfaces of this nodes neighbors
	 * \param lngd phasing of the equivalent satellite in the orbit with less RAAN
	 * \param rngd phasing of the equivalent satellite in the orbit with greater RAAN
	 */
	ArbiterDhbpSat(Ptr<Node> this_node, NodeContainer nodes,
				   std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
				   std::shared_ptr<std::vector<std::vector<int64_t>>> sdfs,
				   std::shared_ptr<std::vector<std::mutex>> sdfsm,
				   std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd);

	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
		int32_t source_node_id, int32_t target_node_id, ns3::Ptr<const ns3::Packet> pkt,
		ns3::Ipv4Header const &ipHeader, bool is_request_for_source_ip_so_no_next_header);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	/*
	 * Unlike INNER and SHORT, ShortDecide here considers the first and last nodes that would be used when forwarding
	 * traffic on the shortest path between the source and destination ground stations. Unlike ELB, this has to be
	 * changed more carefully—the current implementation is useful for running the unoptimized version of DHBP
	 *
	 * \param source_satellite_data a tuple with <source satellite id, <inter-orbit coordinate, intra-orbit
	 * coordinate>>
	 * \param destination_satellite_data a tuple with <destination satellite id, <inter-orbit coordinate, intra-orbit
	 * coordinate>>
	 * \param source_node_id the id of the ground station being routed from
	 * \param target_node_id the id of the ground station being routed to
	 *
	 * \return interface information on the next-hop node
	 */
	std::tuple<int32_t, int32_t, int32_t> ShortDecide(
		std::tuple<int32_t, std::tuple<int16_t, int16_t>> source_satellite_data,
		std::tuple<int32_t, std::tuple<int16_t, int16_t>> destination_satellite_data, int32_t source_node_id,
		int32_t target_node_id);

	void SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table);

	/*
	 * This satellite reads a 2d vector created by the helper class: the 2d vector, indexed by (i,j), stores the id and
	 * floating-point SHORT coordinates of the first satellite on the shortest path between ground station i and ground
	 * station j
	 *
	 * \param table the data structure described above
	 */
	void SetSourceSatelliteTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *table);

	/*
	 * When an ISL or GSL transmits a packet, this function is called to track the resulting queue occupation. The
	 * source and destination ids represent a flow, which DHBP hashes and tracks.
	 *
	 * \param source_node_id the id of the source ground station
	 * \param target_node_id the id of the destination ground station
	 */
	void IncreaseQueue(int32_t source_node_id, int32_t target_node_id);

	/*
	 * When an ISL or GSL transmits a packet, this function is called to track the resulting queue occupation. The
	 * source and destination ids represent a flow, which DHBP hashes and tracks.
	 *
	 * \param source_node_id the id of the source ground station
	 * \param target_node_id the id of the destination ground station
	 */
	void DecreaseQueue(int32_t source_node_id, int32_t target_node_id);

	/*
	 * This function gets the amount of data for a flow (denoted by the source and target ids) for node with id
	 * neighbor_id.
	 *
	 * \param neighbor_id the id of the satellite whose buffers are being checked
	 * \param source_node_id the id of the source ground station
	 * \param target_node_id the id of the destination ground station
	 * \return the number of packets belonging to that flow currently buffered at the satellite with index neighbor_id
	 */
	int64_t GetQueueSizeForFlow(int32_t neighbor_id, int32_t source_node_id, int32_t target_node_id);

  private:
	/*
	 * Based on the destination node's coordinates, returns the interface connecting to the node on the best path to
	 * that destination
	 *
	 * \param source_alpha UNUSED, would denote the inter-orbit coordinate of the source node
	 * \param source_gamma UNUSED, would denote the intra-orbit coordinate of the source node
	 * \param destination_alpha The inter-orbit coordinate of the destination node
	 * \param destination_gamma The intra-orbit coordinate of the destination node
	 * \param source_node_id UNUSED, would denote the id of the source ground station (not necessarily the node with the
	 * source inter- and intra-orbit coordinates)
	 * \param target_node_id the id of the destination ground station (not necessarily the node with these destination
	 * inter- and intra-orbit coordinates)
	 * \return tuple denoting the interface to use that puts you on the best path, as determined by this routing
	 * algorithm
	 */
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t source_alpha, int16_t source_gamma,
															 int16_t destination_alpha, int16_t destination_gamma,
															 int32_t source_node_id, int32_t target_node_id);

	/*
	 * This function uses a bit-interleaving algorithm to uniquely "hash" the source node and target node combination
	 *
	 * \param source_node_id the id of the source ground station
	 * \param target_node_id the id of the destination ground station
	 * \return hashed value that combines the source and target node ids
	 */
	int64_t GetFlowMapping(int32_t source_node_id, int32_t target_node_id);

	std::shared_ptr<std::vector<std::vector<int64_t>>> shared_data_for_satellites;
	std::shared_ptr<std::vector<std::mutex>> shared_data_for_satellites_mutex;
	// this data structure stores a list of satellites adjacent to ground station i, where i indexes this container
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;
	// this 2d vector indexed by (i,j) stores the information of the first satellite on the shortest path between ground
	// station i and ground station j
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *source_satellite_per_flow;
};

} // namespace ns3

#endif // ARBITER_DHBP_SAT_H
