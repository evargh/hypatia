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

#ifndef ARBITER_SINGLE_FORWARD_H
#define ARBITER_SINGLE_FORWARD_H

#include "ns3/arbiter-satnet.h"
#include "ns3/topology-satellite-network.h"
#include "ns3/hash.h"
#include "ns3/abort.h"
#include "ns3/ipv4-header.h"
#include "ns3/udp-header.h"
#include "ns3/tcp-header.h"


namespace ns3 {

class ArbiterSingleForward : public ArbiterSatnet
{
public:
    const static size_t NUM_GROUND_STATIONS = 100;
    const static size_t APPROXIMATE_EARTH_RADIUS_M = 6371000;   
    static TypeId GetTypeId (void);

    // Constructor for single forward next-hop forwarding state
    ArbiterSingleForward(
            Ptr<Node> this_node,
            NodeContainer nodes,
            std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list
    );

    // Single forward next-hop implementation
    std::tuple<int32_t, int32_t, int32_t> TopologySatelliteNetworkDecide(
            int32_t source_node_id,
            int32_t target_node_id,
            ns3::Ptr<const ns3::Packet> pkt,
            ns3::Ipv4Header const &ipHeader,
            bool is_socket_request_for_source_ip
    );

    // Updating of forward state
    void SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id);

    // Static routing table
    std::string StringReprOfForwardingState();

    // Previously, ArbiterSingleForward had no publically-callable function that could
    // dynamically change how it forwards packets. This function is intended to permit that
    // In refactored code, there could be an "ArbiterDynamic" class with a virtual function definition
    // that requires inheritors to define how forwarding state should be mutated.
    
    // For handling DBPR things
    void AddQueueDistance(size_t gs);
    uint64_t GetQueueDistance(size_t gs);
    void ReduceQueueDistance(size_t gs);
    void ModifyDistanceLookup(int32_t target_node_id, uint32_t distance);
    
    std::array<uint64_t, NUM_GROUND_STATIONS>* GetQueueDistances();
    
    void SetNeighborQueueDistance(size_t neighbor_id, std::array<uint64_t, NUM_GROUND_STATIONS> *neighbor_queueing_distance);

private:
    // this function inverts the cartesian MobilityModule coordinates to generate latitude and longitude
    // so that satellites and ground stations can have comparable coordinates
    // thisisn't a precise inverse of the WGS72 standard, since it approximates the earth as a sphere
    // and not a flattened ellipsoid. I performed a few checks and the error is on a smaller order than
    // the distance between satellites
    Vector3D CartesianToGeodetic(Vector3D cartesian);

    std::vector<std::tuple<int32_t, int32_t, int32_t>> m_next_hop_list;

    std::array<uint64_t, NUM_GROUND_STATIONS> m_queueing_distances;
    std::array<std::array<uint64_t, NUM_GROUND_STATIONS>, 4> m_neighbor_queueing_distances;
    std::array<uint32_t, NUM_GROUND_STATIONS> m_distance_lookup_array;

    Ptr<Satellite> ExtractSatellite(int32_t node_id);
};

}

#endif //ARBITER_SINGLE_FORWARD_H
