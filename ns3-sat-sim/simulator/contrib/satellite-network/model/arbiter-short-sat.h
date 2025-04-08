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

class ArbiterShortSat : public ArbiterSatnet
{
  public:
	static const int32_t CELL_SCALING_FACTOR = 9;

	static TypeId GetTypeId(void);

	// Constructor for single forward next-hop forwarding state
	ArbiterShortSat(Ptr<Node> this_node, NodeContainer nodes,
					std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list, int64_t n_o, int64_t s_p_o,
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

	std::tuple<int32_t, int32_t, int32_t> ShortDecide(int16_t aa, int16_t ag, int16_t da, int16_t dg,
													  int32_t target_node_id);

	void SetGSShortTable(std::vector<std::tuple<double, double, double, double>> table);

  protected:
	class NeighborCoordContainer
	{
	  public:
		// this is meant to be an index to the underlying container of coordinates
		enum Direction
		{
			SELF,
			LEFT,
			DOWN,
			UP,
			RIGHT
		};

		NeighborCoordContainer(double lngd, double rngd, int64_t num_orbits, int64_t num_satellites_per_orbit);
		void UpdateCoords(double alpha, double gamma);
		bool VerifyInRange(Direction d, int16_t destination_alpha, int16_t destination_gamma);
		bool VerifyInRange(std::tuple<int16_t, int16_t> c, int16_t destination_alpha, int16_t destination_gamma);

		int16_t GetAlphaModularDistance(Direction d, int16_t destination_alpha);
		int16_t GetGammaModularDistance(Direction d, int16_t destination_gamma);

		int8_t CheckIfAlphaIncrease(Direction d, int16_t destination_alpha);
		int8_t CheckIfGammaIncrease(Direction d, int16_t destination_gamma);
		int8_t CheckIfAlphaIncrease(int16_t source_alpha, int16_t destination_alpha);
		int8_t CheckIfGammaIncrease(int16_t source_gamma, int16_t destination_gamma);

		int32_t GetSquaredEuclideanModularDistance(Direction d, int16_t destination_alpha, int16_t destination_gamma);
		int32_t GetSquaredEuclideanModularDistance(std::tuple<int16_t, int16_t> c, int16_t destination_alpha,
												   int16_t destination_gamma);

		int16_t GetHopcount(Direction d, int16_t destination_alpha, int16_t destination_gamma);
		std::tuple<int16_t, int16_t> GetHopcountTuple(Direction d, int16_t destination_alpha,
													  int16_t destination_gamma);
		int16_t GetHopcount(std::tuple<int16_t, int16_t> coords, int16_t destination_alpha, int16_t destination_gamma);
		std::tuple<int16_t, int16_t> GetHopcountTuple(std::tuple<int16_t, int16_t> coords, int16_t destination_alpha,
													  int16_t destination_gamma);
		int16_t GetAlphaModularDistance(int16_t coordinate_alpha, int16_t destination_alpha);
		int16_t GetGammaModularDistance(int16_t coordinate_gamma, int16_t destination_gamma);
		int16_t GetAlphaBase();
		int16_t GetGammaBase();
		int16_t CreateAlphaCell(double a);
		int16_t CreateGammaCell(double g);

		std::tuple<int16_t, int16_t> GetCoords(Direction d);
		std::tuple<int16_t, int16_t> GetCoordsFromSequence(std::vector<Direction> &d);
		std::tuple<double, double> TransformByDirection(Direction d, double alpha, double gamma);

	  private:
		std::array<std::tuple<double, double>, 5> m_coords;

		double m_lngd, m_rngd;
		int16_t m_alpha_base, m_gamma_base;
		int64_t m_num_orbits, m_num_satellites_per_orbit;
	};

  protected:
	std::tuple<int32_t, int32_t, int32_t> DetermineInterface(int16_t destination_alpha, int16_t destination_gamma,
															 int32_t target_node_id);

	std::tuple<int32_t, int32_t, int32_t> HandleClose(int16_t destination_alpha, int16_t destination_gamma,
													  int32_t target_node_id);

	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_next_hop_list;

	std::vector<std::tuple<double, double, double, double>> m_other_table;
	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_neighbor_ids;

	int64_t num_orbits;
	int64_t num_satellites_per_orbit;

	NeighborCoordContainer neighbors;
};

} // namespace ns3

#endif // ARBITER_SHORT_SAT_H
