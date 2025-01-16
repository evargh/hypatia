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

#ifndef ARBITER_DHPB_H
#define ARBITER_DHPB_H

#include "ns3/arbiter-satnet.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/hash.h"
#include "ns3/abort.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"

namespace ns3
{

class ArbiterDhpb : public ArbiterSatnet
{
public:
	const static int32_t NUM_GROUND_STATIONS = 100;
	const static int32_t APPROXIMATE_EARTH_RADIUS_M = 6371000;
	const static int32_t APPROXIMATE_EARTH_EQUATORIAL_CIRCUMFERENCE = 2 * pi * APPROXIMATE_EARTH_RADIUS_M;
	static TypeId GetTypeId(void);

	// Constructor for single forward next-hop forwarding state
	ArbiterDhpb(Ptr<Node> this_node, NodeContainer nodes,
							std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list);

	// Single forward next-hop implementation
	std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(int32_t source_node_id, int32_t target_node_id,
																																			 ns3::Ptr<const ns3::Packet> pkt,
																																			 ns3::Ipv4Header const &ipHeader,
																																			 bool is_socket_request_for_source_ip);

	// Updating of forward state
	void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

	// Static routing table
	std::string StringReprOfForwardingState();

	// In refactored code, there could be an "ArbiterDynamic" superclass that requires inheritors to define how forwarding
	// state should be mutated.

	// For handling DBPR things
	void SetDestinationSatelliteList(std::array<std::set<int32_t>, NUM_GROUND_STATIONS> *dsl);

	void AddQueueDistance(int32_t target_node_id);
	uint64_t GetQueueDistance(int32_t target_node_id);
	void ReduceQueueDistance(int32_t target_node_id);
	void ModifyDistanceLookup(int32_t target_node_id, uint32_t distance);

	std::pair<uint64_t, uint32_t> GetQueueDistances(uint32_t gid);

	void SetNeighborQueueDistance(int32_t neighbor_node_id, uint32_t my_interface_id, uint32_t remote_node_id,
																uint64_t neighbor_queueing_distance, uint32_t flow_id);

private:
	// this function inverts the cartesian MobilityModule coordinates to generate latitude and longitude
	// so that satellites and ground stations can have comparable coordinates
	// thisisn't a precise inverse of the WGS72 standard, since it approximates the earth as a sphere
	// and not a flattened ellipsoid
	Vector3D CartesianToGeodetic(Vector3D cartesian);
	bool CheckIfInRectangle(Vector3D forwardpos, Vector3D srcpos, Vector3D destpos);
	void GetNeighborInfo();

	double ValidateForwardHeuristic(int32_t source_node_id, int32_t target_node_id, int32_t forward_node_id);

	Ptr<Satellite> GetClosestSatellite(std::set<int32_t> *node_id_set);
	Ptr<Satellite> GetFarthestSatellite(std::set<int32_t> *node_id_set);

	int32_t GSLNodeIdToGSLIndex(int32_t id);
	int32_t GSLIndexToGSLNodeId(int32_t id);

	std::tuple<int32_t, int32_t, int32_t> GetForward(int32_t source_node_id, int32_t target_node_id,
																									 int32_t snapshot_forward_node_id);

	std::vector<std::tuple<int32_t, int32_t, int32_t>> m_next_hop_list;

	std::array<uint64_t, NUM_GROUND_STATIONS> m_queueing_distances;
	std::array<std::array<uint64_t, NUM_GROUND_STATIONS>, 4> m_neighbor_queueing_distances;
	std::array<int32_t, 4> m_neighbor_ids;
	std::array<uint32_t, 4> m_neighbor_interfaces;
	std::array<uint32_t, NUM_GROUND_STATIONS> m_distance_lookup_array;
	std::array<std::set<int32_t>, NUM_GROUND_STATIONS> m_destination_satellite_list;

	Ptr<Satellite> ExtractSatellite(int32_t node_id);
};

} // namespace ns3

#endif // ARBITER_DHPB_H
