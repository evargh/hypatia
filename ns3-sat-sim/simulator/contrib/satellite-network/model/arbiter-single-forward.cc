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

#include "arbiter-single-forward.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("ArbiterSingleForward");
NS_OBJECT_ENSURE_REGISTERED (ArbiterSingleForward);
TypeId ArbiterSingleForward::GetTypeId (void)
{
    static TypeId tid = TypeId ("ns3::ArbiterSingleForward")
            .SetParent<ArbiterSatnet> ()
            .SetGroupName("BasicSim")
    ;
    return tid;
}

ArbiterSingleForward::ArbiterSingleForward(
        Ptr<Node> this_node,
        NodeContainer nodes,
        std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list
) : ArbiterSatnet(this_node, nodes)
{
    m_next_hop_list = next_hop_list;
    m_queueing_distances.fill(0);
    for(auto& item : m_neighbor_queueing_distances) {
        item.fill(0);
    } 
}

Vector3D 
ArbiterSingleForward::CartesianToGeodetic(Vector3D cartesian) {
    return Vector3D(std::asin(cartesian.z/ArbiterSingleForward::APPROXIMATE_EARTH_RADIUS_M)*180.0/pi, std::atan2(cartesian.y, cartesian.x)*180.0/pi, 0); 
}  

Ptr<Satellite>
ArbiterSingleForward::ExtractSatellite(int32_t node_id) {
    SatellitePositionHelperValue storage;
    m_nodes.Get(node_id)->GetObject<MobilityModel>()->GetAttribute("SatellitePositionHelper", storage);
    return storage.Get().GetSatellite();
}

std::tuple<int32_t, int32_t, int32_t> ArbiterSingleForward::TopologySatelliteNetworkDecide(
        int32_t source_node_id,
        int32_t target_node_id,
        Ptr<const Packet> pkt,
        Ipv4Header const &ipHeader,
        bool is_request_for_source_ip_so_no_next_header
) {
    // create a rectangle using the source node's mobility information and the target node's mobility information
    // see if we can call getgeographicposition directly
    // then log if our next hop actually violates that "permitted region"
    // Vector3D.x is the longitudinal position, Vector3D.y is the latitude, Vector3D.z is the height
    int32_t forward_node_id = std::get<0>(m_next_hop_list[target_node_id]); 
    
    // combine the epoch time with the simulator time
    // JulianDate
    Ptr<Satellite> forward_satellite = ExtractSatellite(forward_node_id);
    auto current_time = forward_satellite->GetTleEpoch() + Simulator::Now();
    
    // gets WGS84 coordinates of satellite
    Vector3D forwardpos = forward_satellite->GetGeographicPosition(current_time);
    // gets WGS72 coordinates of ground stations, which need to be converted to WGS84
    // the underlying ground stations is not accessible from the node--it's only used by the topology
    // as a result, the only publically accessible data is the cartesian coordinate, which needs to be converted
    Vector3D srcpos = CartesianToGeodetic(m_nodes.Get(source_node_id)->GetObject<MobilityModel>()->GetPosition());
    Vector3D destpos = CartesianToGeodetic(m_nodes.Get(target_node_id)->GetObject<MobilityModel>()->GetPosition());
    
    // if the two longitudes have the same sign, it's easier to check validity (their rectangle must span exclusively the same hemisphere)
    // this works for 0 but may have issues at exactly 180, need to test
    bool in_rectangle = false;
    
    if (srcpos.x * destpos.x >= 0) {
        if (forwardpos.x >= std::min(srcpos.x, destpos.x) && forwardpos.x <= std::max(srcpos.x, destpos.x)) {
            if (forwardpos.y >= std::min(srcpos.y, destpos.y) && forwardpos.y <= std::max(srcpos.y, destpos.y)) {
               in_rectangle = true;
            }
        } 
    }
    else {
        // if the two longitudes have different signs, more work needs to be done
	// determine if the shortest path goes through the null longitude, or through the opposite longitude
	bool through_0 = (std::abs(srcpos.x) + std::abs(srcpos.y) <= 180);
        // if the shortest path is through the null longitude, then we can repeat the same test
        if (through_0) {
	    if (forwardpos.x >= std::min(srcpos.x, destpos.x) && forwardpos.x <= std::max(srcpos.x, destpos.x)) {
		if (forwardpos.y >= std::min(srcpos.y, destpos.y) && forwardpos.y <= std::max(srcpos.y, destpos.y)) {
		    in_rectangle = true;
		}
            } 
        }
        else {
            // if the shortest path is through the 180 longitude, then we take the complement of the previous test
            // (can see why by multiplying the coordinate system by negative 1
	    if (forwardpos.x <= std::min(srcpos.x, destpos.x) || forwardpos.x >= std::max(srcpos.x, destpos.x)) {
		if (forwardpos.y >= std::min(srcpos.y, destpos.y) && forwardpos.y <= std::max(srcpos.y, destpos.y)) {
		    in_rectangle = true;
		}
            } 
        }
    }
    if (!in_rectangle) {
        NS_LOG_DEBUG("next hop not in permitted region--could be due to inclination");
    }    
    return m_next_hop_list[target_node_id];
}

void ArbiterSingleForward::AddQueueDistance(size_t gs) {
    NS_ASSERT(gs < m_queueing_distances.size());
    m_queueing_distances.at(gs) += m_distance_lookup_array.at(gs);
}

uint64_t ArbiterSingleForward::GetQueueDistance(size_t gs) {
    NS_ASSERT(gs < m_queueing_distances.size());
    return m_queueing_distances.at(gs);
}

void ArbiterSingleForward::ReduceQueueDistance(size_t gs) {
    NS_ASSERT(gs < m_queueing_distances.size()); 
    m_queueing_distances.at(gs) = uint64_t(std::max(int64_t(m_queueing_distances.at(gs)) - int64_t(m_distance_lookup_array.at(gs)), int64_t(0)));
}

std::array<uint64_t, ArbiterSingleForward::NUM_GROUND_STATIONS>* ArbiterSingleForward::GetQueueDistances() {
    return &m_queueing_distances;
}

void ArbiterSingleForward::SetNeighborQueueDistance(size_t neighbor_id, std::array<uint64_t, NUM_GROUND_STATIONS> *neighbor_queueing_distance) {
    // memberwise copy, maybe a memcpy would be better?
    NS_LOG_DEBUG(m_node_id << " interface: " << neighbor_id);
    m_neighbor_queueing_distances.at(neighbor_id) = *neighbor_queueing_distance; 
}

void ArbiterSingleForward::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id) {
    NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
    m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterSingleForward::ModifyDistanceLookup(int32_t target_node_id, uint32_t distance) {
    // judging by previous code, target_node_id cannot be negative
    if(target_node_id >= 0) {
      NS_ASSERT((uint32_t)target_node_id < m_queueing_distances.size());
      m_distance_lookup_array.at(target_node_id) = distance;
    }
}

std::string ArbiterSingleForward::StringReprOfForwardingState() {
    std::ostringstream res;
    res << "Single-forward state of node " << m_node_id << std::endl;
    for (size_t i = 0; i < m_nodes.GetN(); i++) {
        res << "  -> " << i << ": (" << std::get<0>(m_next_hop_list[i]) << ", "
            << std::get<1>(m_next_hop_list[i]) << ", "
            << std::get<2>(m_next_hop_list[i]) << ")" << std::endl;
    }
    return res.str();
}

}
