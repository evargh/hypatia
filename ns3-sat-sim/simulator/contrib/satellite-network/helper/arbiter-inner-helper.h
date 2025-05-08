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

#ifndef ARBITER_INNER_HELPER
#define ARBITER_INNER_HELPER

#include <mutex>
#include "ns3/ipv4-routing-helper.h"
#include "ns3/basic-simulation.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/ipv4-arbiter-routing.h"
#include "ns3/arbiter-inner-sat.h"
#include "ns3/arbiter-single-forward.h"
#include "ns3/abort.h"

namespace ns3
{

class ArbiterInnerHelper
{
  public:
	// APPROXIMATE WGS72 VALUES
	const double EARTH_ORBIT_TIME_NS = 86400000000000;
	const int32_t APPROXIMATE_EARTH_RADIUS_M = 6371000;

	ArbiterInnerHelper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes);

  private:
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> InitialEmptyForwardingState();
	double m_satelliteInclination;
	void UpdateOrbitalParams(int64_t t);
	void UpdateForwardingState(int64_t t);
	void SetCoordinateSkew();
	std::vector<std::tuple<int32_t, int32_t, int32_t>> CreateOutboundInterfaceList(int32_t i);
	std::tuple<double, double, double, double> CartesianToShort(Vector3D cartesian);

	// Parameters
	Ptr<BasicSimulation> m_basicSimulation;
	NodeContainer m_nodes;
	double m_coordinateSkew_deg;
	int64_t m_dynamicStateUpdateIntervalNs;

	int64_t m_num_orbits;
	int64_t m_satellites_per_orbit;
	std::vector<Ptr<ArbiterInnerSat>> m_sat_arbiters;
	std::vector<Ptr<ArbiterSingleForward>> m_gs_arbiters;
	std::vector<std::tuple<double, double, double, double>> m_other_table;

	// TODO: the vector should be properly sized when used, which smells but will work for now

	// this data structure abstracts communication between satellites. Instead of satellites passing messages to
	// communicate state, they have shared memory to communicate state. In general, a satellite has write access to
	// state at the index corresponding to its own node id, and read access everywhere else
	std::shared_ptr<std::vector<int64_t>> shared_data_for_satellites;
	// this data structure contains mutexes for concurrent access to state
	std::shared_ptr<std::vector<std::mutex>> shared_mutex_for_satellites;
	// a pointer to this data structure is passed to satellites
	std::unique_ptr<std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>>> table_of_node;

	std::vector<std::tuple<double, double>> satellite_positions_short;
	std::vector<std::vector<std::tuple<int32_t, std::tuple<double, double>>>> adjacent_satellite_table;
};

} // namespace ns3

#endif /* ARBITER_INNER_HELPER */
