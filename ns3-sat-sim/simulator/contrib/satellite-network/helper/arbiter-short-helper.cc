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

#include "arbiter-short-helper.h"

namespace ns3
{
NS_LOG_COMPONENT_DEFINE("ArbiterShortHelper");

ArbiterShortHelper::ArbiterShortHelper(Ptr<BasicSimulation> basicSimulation, NodeContainer nodes)
{
	std::cout << "SETUP SINGLE FORWARDING ROUTING" << std::endl;
	m_basicSimulation = basicSimulation;
	m_nodes = nodes;

	// Read in initial forwarding state
	std::cout << "  > Create initial single forwarding state" << std::endl;
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state =
		InitialEmptyForwardingState();
	basicSimulation->RegisterTimestamp("Create initial single forwarding state");

	// Set the routing arbiters
	std::cout << "  > Setting the routing arbiter on each node" << std::endl;
	for (size_t i = 0; i < m_nodes.GetN(); i++)
	{
		Ptr<ArbiterShort> arbiter = CreateObject<ArbiterShort>(m_nodes.Get(i), m_nodes, initial_forwarding_state[i]);
		m_arbiters.push_back(arbiter);
		m_nodes.Get(i)->GetObject<Ipv4>()->GetRoutingProtocol()->GetObject<Ipv4ShortRouting>()->SetArbiter(arbiter);
	}
	basicSimulation->RegisterTimestamp("Setup routing arbiter on each node");

	// Load first forwarding state
	m_dynamicStateUpdateIntervalNs =
		parse_positive_int64(m_basicSimulation->GetConfigParamOrFail("dynamic_state_update_interval_ns"));
	std::cout << "  > Forward state update interval: " << m_dynamicStateUpdateIntervalNs << "ns" << std::endl;
	std::cout << "  > Perform first forwarding state load for t=0" << std::endl;
	UpdateForwardingState(0);
	basicSimulation->RegisterTimestamp("Create initial single forwarding state");

	SetCoordinateSkew();
	basicSimulation->RegisterTimestamp("Extract the geographic position of the first satellite");

	UpdateOrbitalParams(0);
	basicSimulation->RegisterTimestamp("Load RAAN and anomaly into routing algorithms");

	SetGSParams();
	basicSimulation->RegisterTimestamp("Assign ground stations to zones");

	std::cout << std::endl;
}

std::tuple<double, double, double, double> ArbiterShortHelper::CartesianToShort(Vector3D cartesian)
{
	Vector2D latlon = Vector2D(std::asin(cartesian.z / ArbiterShortHelper::APPROXIMATE_EARTH_RADIUS_M) * 180.0 / pi,
							   std::atan2(cartesian.y, cartesian.x) * 180.0 / pi);

	// use the formulas given in the paper:
	//  sin (latitude) = -sin (inclination) * sin (gamma)
	//  tan (longitude - alpha) = cos (inclination) * tan (- gamma)

	// in order to generate gamma, you perform -asin(sin(lat)/sin(inclination))
	double first_gamma_term = std::sin(latlon.x * pi / 180);
	double second_gamma_term = std::sin(m_satelliteInclination * pi / 180);
	// some ground stations can be north/south of the highest/lowest orbit, meaning that they dont have an index.
	// Instead, they are given a gamma of 90 if in the nortern hemisphere, and a gamma of 270 if in the southern
	// hemisphere
	double gamma;
	if (first_gamma_term > second_gamma_term)
	{
		gamma = 0;
	}
	else
	{
		gamma = -std::asin(first_gamma_term / second_gamma_term) * 180 / pi;
	}

	// alpha can use the formula provided in the paper, but it requires recentering the geographic coordinate system to
	// align with the TLE's use of the first point of aries at the epoch time this can be extracted directly from some
	// of the satellite class's built-in sgp4 methods
	//
	// these numbers are generally right, but there needs to be more refinement regarding which numbers are used. right
	// now, all of them are generated and i need a quadrant-based way to determine which pairs are correct for the
	// downward and upwards patterns
	double first_alpha_term = latlon.y - m_coordinateSkew_deg;
	double second_alpha_term = std::cos(m_satelliteInclination * pi / 180);
	double third_alpha_term = std::tan(-gamma * pi / 180);
	double alpha_asc =
		std::fmod(180 + (first_alpha_term + std::atan(second_alpha_term * third_alpha_term) * 180 / pi), 360);
	double alpha_desc = (first_alpha_term - std::atan(second_alpha_term * third_alpha_term) * 180 / pi);

	return std::make_tuple(alpha_asc, alpha_desc, std::fmod(180 + gamma, 360), std::fmod(360 - gamma, 360));
}

void ArbiterShortHelper::SetCoordinateSkew()
{
	// because satgenpy generates tles such that the RAAN is always 0 and the inclination is always 0 for hte first
	// satellite, we can use that here to if the method of TLE generation changes, this will have to change. we could
	// generate a "null" TLE and use SGP4 to read the latitude and longitude of that
	Ptr<SatellitePositionMobilityModel> first_mm = m_nodes.Get(0)->GetObject<SatellitePositionMobilityModel>();
	if (first_mm == nullptr)
	{
		NS_ASSERT_MSG(false, "no mm");
	}
	m_coordinateSkew_deg = first_mm->GetSatellite()->GetGeographicPosition(first_mm->GetSatellite()->GetTleEpoch()).y;
}

void ArbiterShortHelper::SetGSParams()
{
	for (uint32_t current_node_id = m_nodes.GetN() - NUM_GROUND_STATIONS; current_node_id < m_nodes.GetN();
		 current_node_id++)
	{
		Ptr<MobilityModel> mm = m_nodes.Get(current_node_id)->GetObject<MobilityModel>();
		if (mm != nullptr)
		{
			std::tuple<double, double, double, double> pos = CartesianToShort(mm->GetPosition());
			NS_LOG_DEBUG(current_node_id << ": " << std::get<0>(pos) << " " << std::get<1>(pos) << " "
										 << std::get<2>(pos) << " " << std::get<3>(pos));
		}
	}
}

void ArbiterShortHelper::UpdateOrbitalParams(int64_t t)
{
	// Filename
	std::ostringstream res;
	res << m_basicSimulation->GetRunDir() << "/";
	res << m_basicSimulation->GetConfigParamOrFail("satellite_network_dir") << "/tles.txt";
	std::string tle_filename = res.str();

	// Check that the file exists
	if (!file_exists(tle_filename))
	{
		throw std::runtime_error("File tles.txt does not exist.");
	}

	// Open file
	std::string title_line, line1, line2;
	std::ifstream tle_file(tle_filename);
	if (tle_file)
	{
		// throw away the first line
		getline(tle_file, title_line);
		while (getline(tle_file, title_line) && getline(tle_file, line1) && getline(tle_file, line2))
		{
			// title line isn't strictly formatted, so just split on space and parse the result as an integer
			int32_t current_node_id = std::stoi(title_line.substr(title_line.find(" ")));
			// line 1 doesn't have anything useful for us
			// line 2 has mean motion (revs per day) which can be converted into orbital period in nanoseconds
			// this is columns 53-64
			// line 2 has RAAN and mean anomaly, which are columns 18-26 and 44-52
			// it also has inclination, which are columns 9-16
			// TODO: determine why I need to shift the substring indices here
			double satellite_alpha = std::stod(line2.substr(17, 8)) - 2 * pi * t / EARTH_ORBIT_TIME_NS;
			double satellite_orbital_period = (1 / std::stod(line2.substr(52, 12))) * 60 * 60 * 1000000000;
			double satellite_gamma = std::stod(line2.substr(43, 8)) + 2 * pi * t / satellite_orbital_period;
			m_satelliteInclination = std::stod(line2.substr(8, 8));

			m_arbiters.at(current_node_id)->SetShortParams(satellite_alpha, satellite_gamma);
		}
	}
	else
	{
		throw std::runtime_error("File tles.txt could not be read.");
	}
	// Given that this code will only be used with satellite networks, this is okay-ish,
	// but it does create a very tight coupling between the two -- technically this class
	// can be used for other purposes as well
	if (!parse_boolean(m_basicSimulation->GetConfigParamOrDefault("satellite_network_force_static", "false")))
	{
		// Plan the next update
		int64_t next_update_ns = t + m_dynamicStateUpdateIntervalNs;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs())
		{
			Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs), &ArbiterShortHelper::UpdateOrbitalParams,
								this, next_update_ns);
		}
	}
}

std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> ArbiterShortHelper::InitialEmptyForwardingState()
{
	std::vector<std::vector<std::tuple<int32_t, int32_t, int32_t>>> initial_forwarding_state;
	for (size_t i = 0; i < m_nodes.GetN(); i++)
	{
		std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list;
		for (size_t j = 0; j < m_nodes.GetN(); j++)
		{
			next_hop_list.push_back(std::make_tuple(-2, -2, -2)); // -2 indicates an invalid entry
		}
		initial_forwarding_state.push_back(next_hop_list);
	}
	return initial_forwarding_state;
}

void ArbiterShortHelper::UpdateForwardingState(int64_t t)
{
	// Filename
	std::ostringstream res;
	res << m_basicSimulation->GetRunDir() << "/";
	res << m_basicSimulation->GetConfigParamOrFail("satellite_network_routes_dir") << "/fstate_" << t << ".txt";
	std::string fstate_filename = res.str();

	// Check that the file exists
	if (!file_exists(fstate_filename))
	{
		throw std::runtime_error(format_string("File %s does not exist.", fstate_filename.c_str()));
	}

	// Open file
	std::string line;
	std::ifstream fstate_file(fstate_filename);
	if (fstate_file)
	{

		// Go over each line
		size_t line_counter = 0;
		while (getline(fstate_file, line))
		{

			// Split on ,
			std::vector<std::string> comma_split = split_string(line, ",", 6);

			// Retrieve identifiers
			int64_t current_node_id = parse_positive_int64(comma_split[0]);
			int64_t target_node_id = parse_positive_int64(comma_split[1]);
			int64_t next_hop_node_id = parse_int64(comma_split[2]);
			int64_t my_if_id = parse_int64(comma_split[3]);
			int64_t next_if_id = parse_int64(comma_split[4]);

			// Check the node identifiers
			NS_ABORT_MSG_IF(current_node_id < 0 || current_node_id >= m_nodes.GetN(), "Invalid current node id.");
			NS_ABORT_MSG_IF(target_node_id < 0 || target_node_id >= m_nodes.GetN(), "Invalid target node id.");
			NS_ABORT_MSG_IF(next_hop_node_id < -1 || next_hop_node_id >= m_nodes.GetN(), "Invalid next hop node id.");

			// Drops are only valid if all three values are -1
			NS_ABORT_MSG_IF(!(next_hop_node_id == -1 && my_if_id == -1 && next_if_id == -1) &&
								!(next_hop_node_id != -1 && my_if_id != -1 && next_if_id != -1),
							"All three must be -1 for it to signify a drop.");

			// Check the interfaces exist
			NS_ABORT_MSG_UNLESS(
				my_if_id == -1 ||
					(my_if_id >= 0 && my_if_id + 1 < m_nodes.Get(current_node_id)->GetObject<Ipv4>()->GetNInterfaces()),
				"Invalid current interface");
			NS_ABORT_MSG_UNLESS(
				next_if_id == -1 ||
					(next_if_id >= 0 &&
					 next_if_id + 1 < m_nodes.Get(next_hop_node_id)->GetObject<Ipv4>()->GetNInterfaces()),
				"Invalid next hop interface");

			// Node id and interface id checks are only necessary for non-drops
			if (next_hop_node_id != -1 && my_if_id != -1 && next_if_id != -1)
			{

				// It must be either GSL or ISL
				bool source_is_gsl = m_nodes.Get(current_node_id)
										 ->GetObject<Ipv4>()
										 ->GetNetDevice(1 + my_if_id)
										 ->GetObject<GSLNetDevice>() != 0;
				bool source_is_isl = m_nodes.Get(current_node_id)
										 ->GetObject<Ipv4>()
										 ->GetNetDevice(1 + my_if_id)
										 ->GetObject<PointToPointLaserNetDevice>() != 0;
				NS_ABORT_MSG_IF((!source_is_gsl) && (!source_is_isl), "Only GSL and ISL network devices are supported");

				// If current is a GSL interface, the destination must also be a GSL interface
				NS_ABORT_MSG_IF(source_is_gsl && m_nodes.Get(next_hop_node_id)
														 ->GetObject<Ipv4>()
														 ->GetNetDevice(1 + next_if_id)
														 ->GetObject<GSLNetDevice>() == 0,
								"Destination interface must be attached to a GSL network device");

				// If current is a p2p laser interface, the destination must match exactly its counter-part
				NS_ABORT_MSG_IF(source_is_isl && m_nodes.Get(next_hop_node_id)
														 ->GetObject<Ipv4>()
														 ->GetNetDevice(1 + next_if_id)
														 ->GetObject<PointToPointLaserNetDevice>() == 0,
								"Destination interface must be an ISL network device");
				if (source_is_isl)
				{
					Ptr<NetDevice> device0 = m_nodes.Get(current_node_id)
												 ->GetObject<Ipv4>()
												 ->GetNetDevice(1 + my_if_id)
												 ->GetObject<PointToPointLaserNetDevice>()
												 ->GetChannel()
												 ->GetDevice(0);
					Ptr<NetDevice> device1 = m_nodes.Get(current_node_id)
												 ->GetObject<Ipv4>()
												 ->GetNetDevice(1 + my_if_id)
												 ->GetObject<PointToPointLaserNetDevice>()
												 ->GetChannel()
												 ->GetDevice(1);
					Ptr<NetDevice> other_device = device0->GetNode()->GetId() == current_node_id ? device1 : device0;
					NS_ABORT_MSG_IF(other_device->GetNode()->GetId() != next_hop_node_id,
									"Next hop node id across does not match");
					NS_ABORT_MSG_IF(other_device->GetIfIndex() != 1 + next_if_id,
									"Next hop interface id across does not match");
				}
			}

			// Add to forwarding state
			m_arbiters.at(current_node_id)
				->SetSingleForwardState(target_node_id, next_hop_node_id,
										1 + my_if_id,  // Skip the loop-back interface
										1 + next_if_id // Skip the loop-back interface
				);

			// Next line
			line_counter++;
		}

		// Close file
		fstate_file.close();
	}
	else
	{
		throw std::runtime_error(format_string("File %s could not be read.", fstate_filename.c_str()));
	}

	// Given that this code will only be used with satellite networks, this is okay-ish,
	// but it does create a very tight coupling between the two -- technically this class
	// can be used for other purposes as well
	if (!parse_boolean(m_basicSimulation->GetConfigParamOrDefault("satellite_network_force_static", "false")))
	{

		// Plan the next update
		int64_t next_update_ns = t + m_dynamicStateUpdateIntervalNs;
		if (next_update_ns < m_basicSimulation->GetSimulationEndTimeNs())
		{
			Simulator::Schedule(NanoSeconds(m_dynamicStateUpdateIntervalNs), &ArbiterShortHelper::UpdateForwardingState,
								this, next_update_ns);
		}
	}
}

} // namespace ns3
