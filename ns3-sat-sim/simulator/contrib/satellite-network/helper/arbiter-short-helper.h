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
#include "ns3/ipv4-short-routing.h"
#include "ns3/arbiter-short-gs.h"
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

  private:
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> InitialEmptyForwardingState();
	double m_satelliteInclination;
	void UpdateOrbitalParams(int64_t t);
	void UpdateForwardingState(int64_t t);
	void SetRoutingParams();
	void SetCoordinateSkew();
	std::vector<std::tuple<int32_t, int32_t, int32_t>> CreateInterfaceList(size_t i);

	std::tuple<double, double, double, double> CartesianToShort(Vector3D cartesian);
	// Parameters
	Ptr<BasicSimulation> m_basicSimulation;
	NodeContainer m_nodes;
	double m_coordinateSkew_deg;
	int64_t m_dynamicStateUpdateIntervalNs;

	int64_t m_num_orbits;
	int64_t m_satellites_per_orbit;
	std::vector<Ptr<ArbiterShortSat>> m_sat_arbiters;
	std::vector<Ptr<ArbiterShortGS>> m_gs_arbiters;
	std::vector<std::tuple<double, double, double, double>> m_other_table;
};

} // namespace ns3

#endif /* ARBITER_SHORT_HELPER */
