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

#ifndef ARBITER_SHORT_HELPER
#define ARBITER_SHORT_HELPER

#include <mutex>
#include "ns3/ipv4-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/ipv4-arbiter-routing.h"
#include "ns3/arbiter-single-forward.h"
#include "ns3/arbiter-short-sat.h"
#include "ns3/abort.h"

namespace ns3
{

class ArbiterShortHelper
{
  public:
	// APPROXIMATE WGS72 VALUES
	const double EARTH_ORBIT_TIME_NS = 86400000000000;
	const int32_t APPROXIMATE_EARTH_RADIUS_M = 6371000;

	ArbiterShortHelper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

  protected:
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> InitialEmptyForwardingState();
	double m_satelliteInclination;

	/*
	 * Loads all the SHORT floating-point coordinate data into memory
	 *
	 * \param t timestamp for calculating all information relative to the epoch
	 */
	void UpdateOrbitalParams(int64_t t);

	/*
	 * Updates satellite connections to ground stations, based on the (truncated) fstate file
	 *
	 * \param t timestamp for calculating all information relative to the epoch
	 */
	void UpdateForwardingState(int64_t t);

	/*
	 * Normalizes the coordinate system between latitude and longitude and SHORT. Useful when ground stations were
	 * assigned floating-point SHORT coordinates.
	 */
	void SetCoordinateSkew();

	/*
	 * Generates the list of satellite neighbors for satellite i
	 *
	 * \param i satellite index
	 * \return vector of <neighbor id, egress interface to neighbor, neighbor's ingress interface from me>, indexed such
	 * that:
	 *    \neighbor in adjacent orbit with smaller RAAN is at index 0
	 *    \neighbor in same orbit with lesser anomaly is at index 1
	 *    \neighbor in same orbit with larger anomaly is at index 2
	 *    \neighbor in adjacent orbit with larger RAAN is at index 3
	 */
	std::vector<std::tuple<int32_t, int32_t, int32_t>> CreateOutboundInterfaceList(int32_t i);

	/* *** LEGACY ***
	 * Converts cartesian coordinates to floating-point SHORT coordinates, useful for ground stations.
	 *
	 * \param cartesian NS-3 cartesian coordinates in space, normalized to the Earth
	 * \return Tuple of two sets of alpha/gamma coordinates. Indices 0,1 denote one pair, indices 2,3 denote the other
	 * pair. This is useful because a ground station may be accessible by two satellites, one ascending and one
	 * descending
	 */

	std::tuple<double, double, double, double> CartesianToShort(Vector3D cartesian);

	// Parameters
	Ptr<BasicSimulation> m_basicSimulation;
	NodeContainer m_nodes;
	double m_coordinateSkew_deg;
	int64_t m_dynamicStateUpdateIntervalNs;

	int64_t m_num_orbits;
	int64_t m_satellites_per_orbit;
	std::vector<Ptr<ArbiterShortSat>> m_sat_arbiters;
	std::vector<Ptr<ArbiterSingleForward>> m_gs_arbiters;
	// *** LEGACY ***: this data structure stores the SHORT floating point coordinates for all ground stations
	std::vector<std::tuple<double, double, double, double>> m_other_table;

	// this data structure stores the periodically-updated SHORT floating point coordinates for all satellites.
	std::vector<std::tuple<double, double>> satellite_positions_short;
	// this data structure stores a vector of satellites (and their positions) that are reachable from ground station i,
	// where i is the index of the parent vector
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> adjacent_satellite_table;
};

} // namespace ns3

#endif /* ARBITER_SHORT_HELPER */
