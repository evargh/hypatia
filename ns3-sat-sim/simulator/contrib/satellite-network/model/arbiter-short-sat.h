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
#include "modular-arithmetic-helper.h"
#include <tuple>
#include <array>

namespace ns3
{

// TODO: rename integer-valued alpha and gamma to something else, since the conflation between SHORT alpha and gamma,
// and inter-orbit and intra-orbit alpha and gamma coordinates, may be confusing (the former are floating-point, while
// the latter are integer-valued)
//
class ArbiterShortSat : public ArbiterSatnet
{
  public:
	// CELL_SCALING_FACTOR is a legacy parameter that how the floating-point SHORT coordinates are converted into
	// cyclic-group integer coordinates. Setting it to 1 means that there is a 1:1 correspondence between the index of a
	// satellite in its orbit and its inter-orbit and intra-orbit coordinates
	//
	// 9 is a legacy value related to how SHORT routing used to function. It has also been tested with other non-zero
	// numbers, but not with 1
	static const int32_t CELL_SCALING_FACTOR = 9;

	static TypeId GetTypeId(void);

	// Constructor for SHORT
	ArbiterShortSat(Ptr<Node> this_node, NodeContainer nodes,
					std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
					std::vector<std::tuple<int32_t, int32_t, int32_t>> neighbor_ids, double lngd, double rngd);

	// Determines forwarding for SHORT
	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(int32_t source_node_id, int32_t target_node_id,
																		 ns3::Ptr<const ns3::Packet> pkt,
																		 ns3::Ipv4Header const &ipHeader,
																		 bool is_socket_request_for_source_ip);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	/*
	 * This function reads the TLE-based values at simulation initialization of each satellite and interfaces with the
	 * NeighborCoordContainer to store these values into state as floating-pont SHORT coordinates, alpha and gamma
	 *
	 * \param raan Right Ascension of the Ascending Node, which is converted into the alpha coordinate
	 * \param anomaly Mean Anomaly at the epoch, which is converted into the gamma coordinate
	 */
	void SetShortParams(double raan, double anomaly);

	// Static routing table
	std::string StringReprOfForwardingState();

	/*
	 * This function looks at a list of satellites, and chooses the satellite that is the closest to this satellite in
	 * terms of hopcount. It then calls DetermineInterface to return the interface that connects to the satellite to
	 * forward to.
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

	/**
	 * NeighborCoordContainer is a scoped class that belongs to all classes that inherit from SHORT
	 *
	 * It is responsible for providing all hopcount and toroid-grid-graph-related operations for a satellite network.
	 */
  protected:
	class NeighborCoordContainer
	{
	  public:
		// this enum is an index to the underlying container of coordinates
		// It allows for the satellite that owns this class to keep track of the coordinates of its neighboring
		// satellites with an intuitive interface.
		// UP denotes the satellite with an increased anomaly within the same orbit
		// DOWN denotes the satellite with a decreased anomaly within the same orbit
		// RIGHT denotes the equivalent satellite in the adjacent orbit with a higher RAAN
		// LEFT denotes the equivalent satellite in the adjacent orbit with a lower RAAN
		//
		// SELF denotes the owner satellite, in case those coordinates are required
		enum Direction
		{
			SELF,
			LEFT,
			DOWN,
			UP,
			RIGHT
		};

		/*
		 * Based on a nodes coordinates in the SHORT space, this function updates a node's knowledge of its neighbor's
		 * coordinates
		 *
		 * \param m_lngd degrees phasing between the equivalent satellite in the orbit with smaller RAAN relative to the
		 * current satellite
		 * \param m_rngd degrees phasing between the equivalent satellite in the orbit with larger RAAN relative to the
		 * current satellite
		 * \param m_num_orbits number of orbits in the constellation
		 * \param m_num_satellites_per_orbit number of satellites per orbit
		 */
		NeighborCoordContainer(double lngd, double rngd, int64_t num_orbits, int64_t num_satellites_per_orbit);

		/*
		 * Based on a nodes coordinates in the SHORT space, this function updates a node's knowledge of its neighbor's
		 * coordinates
		 *
		 * \param alpha The alpha coordinate as calculated by the helper function
		 * \param gamma The gamma coordinate as calculated by the helper function
		 */
		void UpdateCoords(double alpha, double gamma);

		/* *** LEGACY ***
		 * Part of SHORT is allowing satellites to be aware of when they were "within range" of a ground station. This
		 * function does that by communicating whether an adjacent satellite is 'minimal distance' away from a
		 * destination (alpha, gamma) coordinate
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_alpha The alpha coordinate of the destination ground station
		 * \param destination_gamma The gamma coordinate of the destination ground station
		 * \return true if the node in that direction is 'minimal distance' away from the destination coordinates
		 */
		bool VerifyInRange(Direction d, int16_t destination_alpha, int16_t destination_gamma);

		// This function generalizes the previous function to arbitrary coordinates
		bool VerifyInRange(std::tuple<int16_t, int16_t> c, int16_t destination_alpha, int16_t destination_gamma);

		/*
		 * This function calculates the distance, relative to the coordinate space defined by SHORT for the toroidal
		 * grid graph, between an adjacent satellite and a destination coordinate alpha. Recall that alpha denotes
		 * inter-orbital distance
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_alpha The alpha coordinate of the destination node
		 */

		int16_t GetAlphaModularDistance(Direction d, int16_t destination_alpha);

		/*
		 * This function calculates the distance, relative to the coordinate space defined by SHORT for the toroidal
		 * grid graph, between an adjacent satellite and a destination coordinate gamma. Recall that gamma denotes
		 * intra-orbital distance
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_gamma The gamma coordinate of the destination node
		 */
		int16_t GetGammaModularDistance(Direction d, int16_t destination_gamma);

		// These functions generalize the previous functions to arbitrary coordinates
		int16_t GetAlphaModularDistance(int16_t coordinate_alpha, int16_t destination_alpha);
		int16_t GetGammaModularDistance(int16_t coordinate_gamma, int16_t destination_gamma);

		/*
		 * This function calculates whether to forward to an "increased" satellite on the SHORT coordinate space, or a
		 * "decreased" satellite on the SHORT coordinate space, based on the destination coordinate
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_alpha The alpha coordinate of the destination node
		 */
		int8_t CheckIfAlphaIncrease(Direction d, int16_t destination_alpha);

		/*
		 * This function calculates whether to forward to an "increased" satellite on the SHORT coordinate space, or a
		 * "decreased" satellite on the SHORT coordinate space, based on the destination coordinate
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_gamma The gamma coordinate of the destination node
		 */
		int8_t CheckIfGammaIncrease(Direction d, int16_t destination_gamma);

		// These functions generalize the previous functions to arbitrary coordinates
		int8_t CheckIfAlphaIncrease(int16_t source_alpha, int16_t destination_alpha);
		int8_t CheckIfGammaIncrease(int16_t source_gamma, int16_t destination_gamma);

		/*
		 * These functions are currently slightly misleading—they do not calculate pure hopcount, but rather a distance
		 * metric with a linear relationship to hopcount. This is for legacy reasons and should be changed. ELB, SHORT,
		 * and DHBP only consider relative hopcounts. However, INNER considers absolute hopcount, and therefore performs
		 * its own normalization.
		 *
		 * That wrinkle notwithstanding, this function calculates the hopcount between an adjacent node and a
		 * destination coordinate pair
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_alpha The alpha coordinate of the destination node
		 * \param destination_gamma The gamma coordinate of the destination node
		 * \return scaled number of hops between the chosen node and the destination coordinate
		 */
		int16_t GetHopcount(Direction d, int16_t destination_alpha, int16_t destination_gamma);

		/*
		 * Same as GetHopcount, but returns the numbers of inter-orbit and intra-orbit hops, instead of a total number
		 * of hops
		 *
		 * \param d The direction of the node being tested relative to the current node
		 * \param destination_alpha The alpha coordinate of the destination node
		 * \param destination_gamma The gamma coordinate of the destination node
		 * \return tuple of (number of inter-orbit hops, number of intra-orbit hops) between the chosen node and the
		 * destination coordinate
		 */
		std::tuple<int16_t, int16_t> GetHopcountTuple(Direction d, int16_t destination_alpha,
													  int16_t destination_gamma);

		// These two functions generalize the previous functions for arbitrary coordinates
		int16_t GetHopcount(std::tuple<int16_t, int16_t> coords, int16_t destination_alpha, int16_t destination_gamma);
		std::tuple<int16_t, int16_t> GetHopcountTuple(std::tuple<int16_t, int16_t> coords, int16_t destination_alpha,
													  int16_t destination_gamma);

		/*
		 * Returns the size of the inter-orbit cyclic group.
		 *
		 * \return size of the inter-orbit cyclic group
		 */
		int16_t GetAlphaBase();

		/*
		 * Returns the size of the intra-orbit cyclic group.
		 *
		 * \return size of the intra-orbit cyclic group
		 */
		int16_t GetGammaBase();

		/*
		 * Convert the floating-point alpha coordinate, as created by SHORT reading satellite TLEs, to an integer-valued
		 * coordinate in the inter-orbit cyclic group
		 *
		 * \param a SHORT floating-point coordinate
		 * \return cyclic group integer-valued inter-orbit coordinate
		 */
		int16_t CreateAlphaCell(double a);

		/*
		 * Convert the floating-point gamma coordinate, as created by SHORT reading satellite TLEs, to an integer-valued
		 * coordinate in the intra-orbit cyclic group
		 *
		 * \param g SHORT floating-point coordinate
		 * \return cyclic group integer-valued intra-orbit coordinate
		 */
		int16_t CreateGammaCell(double g);

		/*
		 * Return the coordinates of a chosen adjacent satellite
		 *
		 * \param d The direction of the adjacent satellite
		 * \return (inter-orbit coordinate, intra-orbit coordinate) of the adjacent satellite
		 */
		std::tuple<int16_t, int16_t> GetCoords(Direction d);

		/*
		 * Generalizes the previous function e.g. allows you to consider the coordinates of the satellite "two orbits
		 * down"
		 *
		 * \param &d a pointer to a vector of directions
		 * \return (inter-orbit coordinate, intra-orbit coordinate) of the adjacent satellite
		 */
		std::tuple<int16_t, int16_t> GetCoordsFromSequence(std::vector<Direction> &d);

		/*
		 * Creates floating-point SHORT coordinates for adjacent satellites based on the floating-point SHORT
		 * coordinates of the current satellite, as well as the relative position of the adjacent satellite
		 *
		 * \param d The direction of the adjacent satellite
		 * \param alpha the inter-orbit floating-point SHORT coordinate of the SELF satellite
		 * \param gamma the intra-orbit floating-point SHORT coordinate of the SELF satellite
		 * \return (inter-orbit floating-point SHORT coordinate, intra-orbit floating-point SHORT coordinate) of the
		 * adjacent satellite
		 */
		std::tuple<double, double> TransformByDirection(Direction d, double alpha, double gamma);

	  private:
		// This array stores the inter-orbit (alpha) and intra-orbit (gamma) floating-point SHORT coordinates of each
		// satellite, indexed by the Direction enum.
		std::array<std::tuple<double, double>, 5> m_coords;

		/*
		 * \m_lngd: degrees phasing between the equivalent satellite in the orbit with smaller RAAN relative to the
		 * current satellite
		 * \m_rngd: degrees phasing between the equivalent satellite in the orbit with larger RAAN relative to the
		 * current satellite
		 */
		double m_lngd, m_rngd;
		/*
		 * \m_alpha_base: the size of the inter-orbit cyclic group
		 * \m_gamma_base: the size of the intra-orbit cyclic group
		 */
		int16_t m_alpha_base, m_gamma_base;
		/*
		 * \m_num_orbits: number of orbits in the constellation
		 * \m_num_satellites_per_orbit: number of satellites per orbit
		 */
		int64_t m_num_orbits, m_num_satellites_per_orbit;
	};

  protected:
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

	/* *** LEGACY ***
	 * One important part of the implementation of SHORT is the ability for satellites to communicate with ground
	 * stations based on their floating-point coordinates. This function allows for that to occur, even when satellites
	 * are not aware of whether their neighbors are able to establish actual links to ground stations
	 *
	 * NOTE THAT THIS FUNCTION IS HIGHLY COUPLED TO THE PHASING OF THE CONSTELLATION UNDER TEST. IT HAS NOT BEEN
	 * GENERALIZED TO ARBITRARY PHASING.
	 *
	 * \param destination_alpha The inter-orbit coordinate of the destination node
	 * \param destination_gamma The intra-orbit coordinate of the destination node
	 * \param target_node_id the id of the destination ground station (not necessarily the node with these destination
	 * inter- and intra-orbit coordinates)
	 * \return tuple of (number of inter-orbit hops, number of intra-orbit hops) between the chosen node and the
	 * destination coordinate
	 */
	std::tuple<int32_t, int32_t, int32_t> HandleClose(int16_t destination_alpha, int16_t destination_gamma,
													  int32_t target_node_id);

	// This data structure is taken from the single forward algorithm—it's useful for satellites communicating to
	// in-range ground stations
	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_next_hop_list;

	// This data structure stores the floating-point SHORT coordinates to satellites adjacent to ground stations
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> m_other_table;

	// This data structure stores the interface data for all neighbor satellites, used for forwarding
	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_neighbor_ids;

	int64_t num_orbits;
	int64_t num_satellites_per_orbit;

	NeighborCoordContainer neighbors;
};

} // namespace ns3

#endif // ARBITER_SHORT_SAT_H
