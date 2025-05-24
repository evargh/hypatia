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

#ifndef ARBITER_INNER_SAT_H
#define ARBITER_INNER_SAT_H

#include "ns3/abort.h"
#include "ns3/arbiter-short-sat.h"
#include "ns3/hash.h"
#include "ns3/ipv4-header.h"
#include "ns3/topology-satellite-network.h"
#include <tuple>
#include <mutex>
#include <queue>

namespace ns3
{

/* NOTE: The following changes are worth making:
 * Making INNER not overuse paths (annotated in the code)
 *
 * The following changes are worth considering:
 * Making INNER parameter-free
 * Making INNER probabilistically load balance
 * Making INNER's propagation delay estimation resilient to different constellation setups (inter-orbit propagation
 * delay may not always be 0.003 seconds, for example)
 */
class ArbiterInnerSat : public ArbiterShortSat
{
  public:
	// this sets the neighborhood depth for INNER
	static const int16_t EXPLORATION_DEPTH = 2;

	// these are parameters for considering congestion in nodes
	static const int64_t MINIMUM_FULLNESS_INTERVAL_NS = 100000000;
	static constexpr double MINIMUM_FULLNESS_RATIO = 0.25;

	// due to the structure of the constellation (seen in the graph in the paper), the link lengths lead to very similar
	// propagation delays. these are estimated here
	static constexpr double INTER_ORBIT_PROPAGATION_DELAY_SECONDS = 0.003;
	static constexpr double INTRA_ORBIT_PROPAGATION_DELAY_SECONDS = 0.007;

	// This data structure captures relevant information for INNER forwarding:
	// \std::vector<Direction> captures the interfaces used for the partial path
	// \std::vector<int32_t> captures the node ids for the partial path
	// \int16_t depth of the current path
	// \int32_t hopcount of the total path
	// \double cost of the partial path
	// \double cost of the rest of the path
	typedef std::tuple<std::vector<NeighborCoordContainer::Direction>, std::vector<int32_t>, int16_t, int32_t, double,
					   double>
		path_element;

	static TypeId GetTypeId(void);

	/*
	 * \param this_node smart pointer to the node that owns this arbiter
	 * \param nodes container of the other nodes
	 * \param next_hop_list arbiters require an initialized list of interfaces on initialization
	 * \param n_o number of orbits
	 * \param s_p_o number of satellites per orbit
	 * \param sdfs data structure for shared data (abstracts satellite communication)
	 * \param sdfsm data structure for mutexes
	 * \param ton pointer to the data structure that allows a node to find the neighbors of other nodes
	 * \param neighbor_ids table of the interfaces of this nodes neighbors
	 * \param lngd phasing of the equivalent satellite in the orbit with less RAAN
	 * \param rngd phasing of the equivalent satellite in the orbit with greater RAAN
	 * \param qsize size of the ISL buffer in packets
	 * \param bw link bandwidth in mbps
	 */
	ArbiterInnerSat(Ptr<Node> this_node, NodeContainer nodes,
					std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
					std::shared_ptr<std::vector<int64_t>> sdfs, std::shared_ptr<std::vector<std::mutex>> sdfsm,
					std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> *ton,

					std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd,
					int64_t qsize, double bw);

	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
		int32_t source_node_id, int32_t target_node_id, ns3::Ptr<const ns3::Packet> pkt,
		ns3::Ipv4Header const &ipHeader, bool is_request_for_source_ip_so_no_next_header);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	/*
	 * This determines which satellite to route to in order to reach the ground station denoted by target_node_id. This
	 * used to override SHORT's implementation of the same function, but SHORT was now updated to perform the same
	 * function. However, this function calls INNER's version of DetermineInterface, not SHORT's version, which is why
	 * it's kept
	 *
	 * \param adjacent_satellites a vector of inter- and intra-orbit coordinate pairs
	 * \param target_node_id the id of the ground station being routed to
	 *
	 * \return interface information on the next-hop node
	 */
	std::tuple<int32_t, int32_t, int32_t> ShortDecide(std::vector<std::tuple<int16_t, int16_t>> adjacent_satellites,
													  int32_t target_node_id);

	/*
	 * This function also used to override SHORT's implementation, but now I don't believe this function needs to exist
	 * anymore
	 *
	 * \param table of node ids and floating-point SHORT coordinates, indexed by ground station id. these are read into
	 * an internal data structure
	 */
	void SetGSShortTable(std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> table);

	/*
	 * This updates the shared data structure at the index of the current node id with value `val`
	 *
	 * \param val a 4-bit denoting which interfaces are congested. bit 0 is 0 or 1 depending on whether the left
	 * interface is congested, bit 1 for the down interface, bit 2 for the up interface, bit 3 for the right interface
	 */
	void SetSharedState(int64_t val);
	/*
	 * This gets the 4-bit interface congestion value for the node with id `loc`
	 *
	 * \param loc index of the node with congestion to check
	 * \return the 4-bit value denoting which interfaces are congestion.
	 */
	int64_t GetSharedState(size_t loc);

  private:
	/*
	 * Based on the destination node's coordinates, returns the interface connecting to the node on the best path to
	 * that destination
	 *
	 * \param destination_alpha The inter-orbit coordinate of the destination node
	 * \param destination_gamma The intra-orbit coordinate of the destination node
	 * \param target_node_id the id of the destination ground station (not necessarily the node with these destination
	 * inter- and intra-orbit coordinates)
	 * \return tuple denoting the interface to use that puts you on the best path, as determined by this routing
	 * algorithm
	 */
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t destination_alpha, int16_t destination_gamma,
															 int32_t target_node_id);

	/*
	 * This function creates a list of partial paths to be evaluated.
	 *
	 * \param start_node_id the id of the current node (this is a legacy from when this function was performed
	 * recursively instead of iteratively, can be replaced with class member)
	 * \param max_depth_level the maximum depth of a path to be considered (this is a legacy from when this function was
	 * performed recursively instead of iteratively, can be replaced with the constant representing the same thing)
	 * \param destination_alpha the inter-orbit coordinate of the destination satellite
	 * \param destination_gamma the intra-orbit coordinate of the destination satellite
	 * \param target_node_id the id of the ground station to be routed to
	 * \return a list of `path_element`, which lists the costs, node ids, and interfaces
	 */
	std::vector<path_element> CreateViablePaths(int32_t start_node_id, int16_t max_depth_level,
												int16_t destination_alpha, int16_t destination_gamma,
												int32_t target_node_id);
	/*
	 * Based on a list of interfaces to be used, this function determines the number of hops it will take to get to a
	 * destination satellite. Recall that INNER requires each node on a path to require fewer hops to the destination
	 * than the previous node on the path
	 *
	 * \param node_id the id of the last node to be added on the path. this is the node being tested
	 * \param previous_hops the number of hops from the previous node on the path to the destination
	 * \param direction_sequence the list of interfaces used on the partial path (including the interface connecting to
	 * the node being tested)
	 * \param depth the length of the partial path being considered
	 * \param target_node_id the id of the ground station to be routed to
	 * \param destination_alpha the inter-orbit coordinate of the destination satellite
	 * \param destination_gamma the intra-orbit coordinate of the destination satellite
	 * \return an integer denoting whether or not the path is viable, and if so, any other properties of it.
	 *    \0: the node being added does not construct a viable partial path (equal or more hops to destination than
	 * previous node on the path)
	 *    \1: the node being added does construct a viable partial path (fewer hops to destination than previous node on
	 * the path)
	 *    \2: the node being added can reach the ground station directly, signaling that no further routing needs to be
	 * done
	 */
	int TestIfViableHop(int32_t node_id, int32_t previous_hops,
						std::vector<NeighborCoordContainer::Direction> *direction_sequence, int16_t depth,
						int32_t target_node_id, int16_t destination_alpha, int16_t destination_gamma);

	/*
	 * When this function is called, it measures the number of packets in the buffer. If the number of packets is over a
	 * certain limit, a timer is started. If the number of packets continues to be over that limit for a certain time,
	 * then that interface is marked as congested. If the number of packets ever drops under that limit, the timer is
	 * reset and the interface is marked as uncongested.
	 */
	void SetInterfaceCongestionBits();
	/*
	 * Estimates propagation delay based on the number of inter-orbit and intra-orbit hops. However, recall that the
	 * `horizontal_hops` and `vertical_hops` generated by the SHORT neighborhood container are linearly scaled up from
	 * the number of hops. As a result, this function corrects for that.
	 *
	 * \param horizontal_hops a (scaled) number of inter-orbit hops
	 * \param vertical_hops a (scaled) number of intra-orbit hops
	 * \return estimated propagation delay, in seconds, for the path with this many inter-orbit and intra-orbit hops
	 */
	double GetEstimatedPropagationDelay(int32_t horizontal_hops, int32_t vertical_hops);

	/*
	 * Estimates propagation delay based on the number of inter-orbit and intra-orbit hops. However, recall that the
	 * `horizontal_hops` and `vertical_hops` generated by the SHORT neighborhood container are linearly scaled up from
	 * the number of hops. As a result, this function corrects for that.
	 *
	 * \param node_sequence a pointer to a list of nodes used for the tested path
	 * \param direction_sequence a pointer to a list of interfaces used for the tested path
	 * \return estimated queueding delay, in seconds
	 */
	double GetEstimatedCongestionDelay(std::vector<int32_t> *node_sequence,
									   std::vector<NeighborCoordContainer::Direction> *direction_sequence);

	// stores timestamps for measuring how long each of the 4 isls is un/congested
	std::array<int64_t, 4> m_interface_congestion_timer;

	// the function of this container is the same as in INNER and DHBP, but instead contains flags for whether
	// interfaces are congested or not on neighboring nodes
	std::shared_ptr<std::vector<int64_t>> shared_data_for_satellites;
	std::shared_ptr<std::vector<std::mutex>> shared_data_for_satellites_mutex;

	// the table that stores the neighbors of each node, as well as their associated interfaces
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> *table_of_node;
	// the table that stores the list of satellites that are adjacent to a ground station (SHORT should probably have a
	// protected version of this member data structure to reduce redundancy)
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;

	// defined by constructor
	int64_t m_link_queue_size_packets;
	double m_link_bandwidth_mbps;
};

} // namespace ns3

#endif // ARBITER_INNER_SAT_H
