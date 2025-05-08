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

// NOTE: THIS CLASS WAS WRITTEN IN A FEW DAYS DUE TO LAST-MINUTE REQUIREMENTS. AS A RESULT, IT HAS NOT BEEN EXHAUSTIVELY
// TESTED. ITS DESIGN IS ALSO RELATIVELY COMPLEX, SINCE ITS SYSTEM MODEL HAD TO BE ADAPTED TO HYPATIA. I ENCOURAGE UNIT
// TESTING AND DEEPER EVALUATION OF CORRECTNESS
class ArbiterElbSat : public ArbiterShortSat
{
  public:
	static TypeId GetTypeId(void);

	// theta is a parameter in ELB. I follow the value used by the paper
	static constexpr double ELB_THETA_S = 0.2;
	static constexpr double MAX_PROPAGATION_DELAY_SECONDS = 0.007;

	typedef std::tuple<NeighborCoordContainer::Direction, int32_t, std::tuple<int8_t, double>> distance_element;

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
	 * \param isl_qsize buffer capacity of ISLs
	 * \param gsl_qsize buffer capacity of GSLs
	 * \param elb_update_interval time in nanoseconds for how often elb checks its buffers
	 */
	ArbiterElbSat(Ptr<Node> this_node, NodeContainer nodes,
				  std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
				  std::shared_ptr<std::vector<std::tuple<int8_t, double>>> sdfs,
				  std::shared_ptr<std::vector<std::mutex>> sdfsm,
				  std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd,
				  int64_t isl_qsize, int64_t gsl_qsize, int64_t elb_update_interval);

	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
		int32_t source_node_id, int32_t target_node_id, ns3::Ptr<const ns3::Packet> pkt,
		ns3::Ipv4Header const &ipHeader, bool is_request_for_source_ip_so_no_next_header);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	/*
	 * Unlike INNER and SHORT, ShortDecide here considers the first and last nodes that would be used when forwarding
	 * traffic on the shortest path between the source and destination ground stations. While I think this can be
	 * replaced, I don't believe it should change the results significantly.
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
	 * Set the state (free, fairly busy, busy) of the current node, as well as a chi (load balancing) value (if in the
	 * busy state)
	 *
	 * \param val tuple of <index representing state, chi> where index representing state:
	 *  \0: free
	 *  \1: fairly busy
	 *  \2: busy
	 */
	void SetSharedState(std::tuple<int8_t, double> val);

	/*
	 * Get the state (free, fairly busy, busy) of the node with id loc, as well as a chi (load balancing) value (if in
	 * the busy state)
	 *
	 * \param loc index of the node
	 * \return val tuple of <index representing state, chi> where index representing state:
	 *  \0: free
	 *  \1: fairly busy
	 *  \2: busy
	 */
	std::tuple<int8_t, double> GetSharedState(size_t loc);

	/*
	 * When an ISL or GSL transmits a packet, this function is called to track the number of bytes sent by that
	 * interface
	 *
	 * \param the id of the interface:
	 *  \0: Interface to the equivalent node in adjacent orbit with smaller RAAN
	 *  \1: Interface to the node in the same orbit with smaller anomaly
	 *  \2: Interface to the node in the same orbit with larger anomaly
	 *  \3: Interface to the equivalent node in adjacent orbit with larger RAAN
	 *  \4: Interface to any ground stations
	 */
	void IncrementTxCounter(uint32_t interface_id);

	/*
	 * When an ISL or GSL transmits a packet, this function is called to track the number of bytes received by that
	 * interface
	 *
	 * \param the id of the interface:
	 *  \0: Interface to the equivalent node in adjacent orbit with smaller RAAN
	 *  \1: Interface to the node in the same orbit with smaller anomaly
	 *  \2: Interface to the node in the same orbit with larger anomaly
	 *  \3: Interface to the equivalent node in adjacent orbit with larger RAAN
	 *  \4: Interface to any ground stations
	 */
	void IncrementRxCounter(uint32_t interface_id);

	/*
	 * TODO: This function was written quickly in the week before turning in the thesis. As a result, its correctness
	 * and adherence to spec needs to be reviewed, since it went through various changes in order to adapt the ELB
	 * system model to Hypatia. I have annotated some problematic code I've seen while re-reading the code—please check
	 * correctness.
	 *
	 * This function is called periodically by the helper function in order to determine the state of each ISL. Note
	 * that, while ELB considers the state of the satellite as a whole, this implementation instead considers each
	 * interface. It then uses the state of the busiest interface as the state of the satellite as a whole.
	 */
	void UpdateELBState();

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
	 * This is similar to INNER's ShortDecide. However, it is currently not used. It tests which satellite adjacent to a
	 * ground station is closest to another satellite with coordinates coords.
	 *
	 * \param adjacent_satellites, a container of all satellites that are reachable from a tested ground station
	 * \param coords, the inter- and intra-orbit coordinates in the grid graph for the node being tested
	 */
	std::tuple<int32_t, std::tuple<int16_t, int16_t>> ExtractClosestTuple(
		std::vector<std::tuple<int32_t, std::tuple<int16_t, int16_t>>> adjacent_satellites,
		std::tuple<int16_t, int16_t> coords);

	/*
	 * Clears the memory which tracks how much data enters and exits each interface
	 */
	void FlushCounters();

	// the function of this container is the same as in INNER and DHBP, but instead contains state and chi for each
	// neighbor node
	std::shared_ptr<std::vector<std::tuple<int8_t, double>>> shared_data_for_satellites;
	std::shared_ptr<std::vector<std::mutex>> shared_data_for_satellites_mutex;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;
	// this 2d vector indexed by (i,j) stores the information of the first satellite on the shortest path between ground
	// station i and ground station j
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> *source_satellite_per_flow;

	int64_t m_isl_queue_size_packets, m_gsl_queue_size_packets;
	double elb_update_interval_s;

	// stores the amount of received and sent bytes on each interface
	// TODO: this design has lead to some technical debt while I was exploring implementation methods. Review
	// UpdateELBState and IncrementRXCounter for more
	std::vector<std::tuple<double, double, double>> interface_ingress_egress_counters;
	int32_t m_o_counter, m_is_counter, m_it_counter;

	// this random variable is compared to chi for probabilistic load balancing
	Ptr<UniformRandomVariable> chi_compare;

	// this boolean speeds up the ELB evaluation
	bool m_changed;
};

} // namespace ns3

#endif // ARBITER_ELB_SAT_H
