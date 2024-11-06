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
        std::vector<std::tuple<int32_t, int32_t, int32_t>> next_hop_list,
        double inclination_angle,
        int32_t num_orbits
) : ArbiterSatnet(this_node, nodes)
{
    m_next_hop_list = next_hop_list;
    m_queueing_distances.fill(0);
    m_neighbor_ids.fill(0);
    for(auto& item : m_neighbor_queueing_distances) {
        item.fill(0);
    }
    m_inclination_angle = inclination_angle;
    m_num_orbits = num_orbits;
    m_satellites_per_orbit = (m_nodes.GetN() - 100)/num_orbits;
    NS_ASSERT((m_nodes.GetN() - 100) % num_orbits == 0);
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

bool ArbiterSingleForward::CheckIfInRectangle(
    Vector3D forwardpos,
    Vector3D srcpos, 
    Vector3D destpos
) {
    // if the two longitudes have the same sign, it's easier to check validity (their rectangle must span exclusively the same hemisphere)
    // this works for 0 but may have issues at exactly 180, need to test

    if (srcpos.y * destpos.y >= 0) {
      if (forwardpos.x >= std::min(srcpos.x, destpos.x) && forwardpos.x <= std::max(srcpos.x, destpos.x)) {
        if (forwardpos.y >= std::min(srcpos.y, destpos.y) && forwardpos.y <= std::max(srcpos.y, destpos.y)) {
          return true;
        }
      } 
    }
    else {
      // if the two longitudes have different signs, more work needs to be done
      // determine if the shortest path goes through the null longitude, or through the opposite longitude
      bool through_0 = (std::abs(destpos.y) + std::abs(destpos.y) <= 180);
      // if the shortest path is through the null longitude, then we can repeat the same test
      if (through_0) {
        if (forwardpos.x >= std::min(srcpos.x, destpos.x) && forwardpos.x <= std::max(srcpos.x, destpos.x)) {
          if (forwardpos.y >= std::min(srcpos.y, destpos.y) && forwardpos.y <= std::max(srcpos.y, destpos.y)) {
	    return true;
	  }
	} 
      }
      else {
        // if the shortest path is through the 180 longitude, then we take the complement of the previous test
	// (can see why by multiplying the coordinate system by negative 1)
	if (forwardpos.y <= std::min(srcpos.y, destpos.y) || forwardpos.y >= std::max(srcpos.y, destpos.y)) {
	  if (forwardpos.x >= std::min(srcpos.x, destpos.x) && forwardpos.x <= std::max(srcpos.x, destpos.x)) {
	    return true;
	  }
	} 
      }
    }
    return false;
}
// TODO: can make this into a function on iterators
Ptr<Satellite> ArbiterSingleForward::GetClosestSatellite(std::set<int32_t> *node_id_set) {
    double min_distance = 100000000000;
    int32_t min_node = -1;
    Ptr<MobilityModel> my_satellite_mobility_model = m_nodes.Get(m_node_id)->GetObject<MobilityModel>();
    if (node_id_set->empty()) {
        return nullptr;
    }

    for(auto it = node_id_set->begin(); it != node_id_set->end(); it++) {
       Ptr<MobilityModel> n = m_nodes.Get(*it)->GetObject<MobilityModel>();
       double dist = my_satellite_mobility_model->GetDistanceFrom(n);
       if (dist <= min_distance) {
           min_distance = dist;
           min_node = *it;
       }
    }
    // previous code should verify that there are destination satellites
    NS_ASSERT(min_node != -1);
    return ExtractSatellite(min_node);
}

Ptr<Satellite> ArbiterSingleForward::GetFarthestSatellite(std::set<int32_t> *node_id_set) {
    double max_distance = 0;
    int32_t max_node = -1;
    Ptr<MobilityModel> my_satellite_mobility_model = m_nodes.Get(m_node_id)->GetObject<MobilityModel>();
    if (node_id_set->empty()) {
        return nullptr;
    }

    for(auto it = node_id_set->begin(); it != node_id_set->end(); it++) {
       Ptr<MobilityModel> n = m_nodes.Get(*it)->GetObject<MobilityModel>();
       double dist = my_satellite_mobility_model->GetDistanceFrom(n);
       if (dist >= max_distance) {
           max_distance = dist;
           max_node = *it;
       }
    }
    // previous code should verify that there are destination satellites
    NS_ASSERT(max_node != -1);
    return ExtractSatellite(max_node);
}

int32_t ArbiterSingleForward::GSLNodeIdToGSLIndex(int32_t id) {
    return id - m_nodes.GetN() + NUM_GROUND_STATIONS;
}

int32_t ArbiterSingleForward::GSLIndexToGSLNodeId(int32_t id) {
    return m_nodes.GetN() - NUM_GROUND_STATIONS + id;
}

bool ArbiterSingleForward::ValidateForwardInRectangle(
        int32_t source_node_id,
        int32_t target_node_id,
        int32_t forward_node_id
) {
    // if the destination node is unreachable, don't bother with this
    if (forward_node_id == -2 || forward_node_id == -1) {
      return false;
    }
    // create a rectangle using the source node's mobility information and the target node's mobility information
    // then log if our next hop actually violates that "permitted region"
    Ptr<Satellite> forward_satellite = ExtractSatellite(forward_node_id);
    auto current_time = forward_satellite->GetTleEpoch() + Simulator::Now();
   
    // gets WGS84 coordinates of satellite
    Vector3D forwardpos = forward_satellite->GetGeographicPosition(current_time);

    // gets furthest destination satellite and closest source satellite to this position
    // TODO: this is just a heuristic for now and needs closer examination
    Ptr<Satellite> source_satellite = GetClosestSatellite(&m_destination_satellite_list.at(GSLNodeIdToGSLIndex(source_node_id)));
    Ptr<Satellite> destination_satellite = GetFarthestSatellite(&m_destination_satellite_list.at(GSLNodeIdToGSLIndex(target_node_id)));

    // gets WGS72 coordinates of ground stations, which need to be converted to WGS84
    // the underlying ground stations is not accessible from the node--it's only used by the topology
    // as a result, the only publically accessible data is the cartesian coordinate, which needs to be converted
    Vector3D srcpos = CartesianToGeodetic(m_nodes.Get(source_node_id)->GetObject<MobilityModel>()->GetPosition());
    Vector3D destpos = CartesianToGeodetic(m_nodes.Get(target_node_id)->GetObject<MobilityModel>()->GetPosition());
     
    return CheckIfInRectangle(forwardpos, srcpos, destpos);
}    

void ArbiterSingleForward::GetNeighborInfo() {
    uint32_t num_interfaces = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetNInterfaces();
    // interfaces 1,2,3,4 are point to pont
    // 0 is loopback, 5 is gsl
    for (uint32_t i = 1; i < num_interfaces; i++) {
        Ptr<PointToPointLaserNetDevice> p2p = m_nodes.Get(m_node_id)->GetObject<Ipv4>()->GetNetDevice(i)->GetObject<PointToPointLaserNetDevice>();
        if (p2p != 0) {
            m_neighbor_ids.at(i-1) = p2p->GetDestinationNode()->GetId();
            m_neighbor_interfaces.at(i-1) = p2p->GetRemoteIf();
        }
    }
    // iterate through interfaces (0 is loopback)
}

std::tuple<int32_t, int32_t, int32_t> ArbiterSingleForward::TopologySatelliteNetworkDecide(
        int32_t source_node_id,
        int32_t target_node_id,
        Ptr<const Packet> pkt,
        Ipv4Header const &ipHeader,
        bool is_request_for_source_ip_so_no_next_header
) {
    // each time this is run, get all net devices and determine nodes and interfaces. set the member arrays appropriately
    NS_ASSERT(m_nodes.GetN() > ArbiterSingleForward::NUM_GROUND_STATIONS);
    NS_LOG_DEBUG("destination: " << target_node_id << " source: " << source_node_id << " me: " << m_node_id);
    // if this arbiter is runing on a ground station, we use snapshot routing
    // the source can occasionally be a satellite in case of ICMP messages. if that's the case, use snapshot routing
    if (m_node_id < static_cast<int32_t>(m_nodes.GetN() - NUM_GROUND_STATIONS) && source_node_id >= static_cast<int32_t>(m_nodes.GetN() - NUM_GROUND_STATIONS)) {  
      GetNeighborInfo();
      int32_t forward_node_id = std::get<0>(m_next_hop_list[target_node_id]); 
       
      // if the forward node is the target node, then this is irrelevant
      if(forward_node_id != target_node_id) {
        // TODO: better logging
        // at the moment, this routing algorithm doesn't seem to scale well. the meandering initial packets result in TTL expiration messages coming from ICMP. those TTL messages are marked with the source of a satellite and the destination of the initial ground station, which is fine: until this ICMP message itself gets dropped. then the source and destination are both satellites, and this algorithm doesn't know how to route between satellites, leading to errors
        // presumably this is also an issue for base Hypatia, but never came up
        // as a result, if the destination is a satellite, return a failed address
        if(target_node_id <= 1583) {
          return std::make_tuple(-1, 0, 0);
        }
         
        std::array<std::tuple<uint64_t, int8_t, bool>, 4> distance_my_interface_region;
        for(int8_t i = 0; i < 4; i++) {
            bool in_region = ValidateForwardInRectangle(source_node_id, target_node_id, m_neighbor_ids.at(i));
            distance_my_interface_region.at(i) = std::make_tuple(
                                             m_neighbor_queueing_distances.at(i).at(GSLNodeIdToGSLIndex(target_node_id)),
                                             i,
                                             in_region
            );
        }
        std::sort(distance_my_interface_region.begin(), distance_my_interface_region.end(), 
                  [](std::tuple<uint64_t, int8_t, bool> a,
                  std::tuple<uint64_t, int8_t, bool> b)
                  {
                      return std::get<0>(a) < std::get<0>(b);
                  });
        // find the best interface within the permitted region
        for(auto elem : distance_my_interface_region) {
            if(std::get<2>(elem)) {
                int32_t optimal_outbound_interface = std::get<1>(elem);
		if(m_neighbor_ids.at(optimal_outbound_interface) != forward_node_id) {
		  NS_LOG_DEBUG("forwarding mismatch");
		}
		else {
		  NS_LOG_DEBUG("forwarding match");
		}
                return std::make_tuple(m_neighbor_ids.at(optimal_outbound_interface), optimal_outbound_interface+1, m_neighbor_interfaces.at(optimal_outbound_interface));
            }
        }
        // if no interface is within the permitted region, just send to the best interface
        NS_LOG_DEBUG("no interface found within permitted region");
        int32_t optimal_outbound_interface = std::get<1>(distance_my_interface_region.at(0));
	if(m_neighbor_ids.at(optimal_outbound_interface) != forward_node_id) {
            NS_LOG_DEBUG("forwarding mismatch");
	}
        else {
	    NS_LOG_DEBUG("forwarding match");
	}
        return std::make_tuple(m_neighbor_ids.at(optimal_outbound_interface), optimal_outbound_interface+1, m_neighbor_interfaces.at(optimal_outbound_interface)); 
      } 
    }
    return m_next_hop_list[target_node_id];
}

void ArbiterSingleForward::AddQueueDistance(int32_t target_node_id) {
    NS_ASSERT(GSLNodeIdToGSLIndex(target_node_id) < (int32_t)m_queueing_distances.size()); 
    m_queueing_distances.at(GSLNodeIdToGSLIndex(target_node_id)) += 1;
}

uint64_t ArbiterSingleForward::GetQueueDistance(int32_t target_node_id) {
    NS_ASSERT(GSLNodeIdToGSLIndex(target_node_id) < (int32_t)m_queueing_distances.size());
    return m_queueing_distances.at(GSLNodeIdToGSLIndex(target_node_id)) * m_distance_lookup_array.at(GSLNodeIdToGSLIndex(target_node_id));
}

void ArbiterSingleForward::ReduceQueueDistance(int32_t target_node_id) {
    NS_ASSERT(GSLNodeIdToGSLIndex(target_node_id) < (int32_t)m_queueing_distances.size());
    // TODO: reintroduce this assertion
    // very infrequently, this assertion is violated. some nodes do not have "addqueuedistance" called before this is called, even
    // though they are receiving traffic just like every other node. namely traffic directed towards ground station 20 passing through node 2
    // I think this is due to how satellites generate ICMP messages
    //NS_ASSERT(m_queueing_distances.at(GSLNodeIdToGSLIndex(target_node_id)) > 0);
    if (m_queueing_distances.at(GSLNodeIdToGSLIndex(target_node_id)) > 0) {
      m_queueing_distances.at(GSLNodeIdToGSLIndex(target_node_id)) -= 1;
    } 
    else {
      NS_LOG_DEBUG("attempted to reduce 0-length distance queue");
    }
}
std::pair<std::array<uint64_t, ArbiterSingleForward::NUM_GROUND_STATIONS>*, std::array<uint32_t, ArbiterSingleForward::NUM_GROUND_STATIONS>*>
ArbiterSingleForward::GetQueueDistances() {
    return std::make_pair(&m_queueing_distances, &m_distance_lookup_array);
}

void ArbiterSingleForward::SetNeighborQueueDistance(
    int32_t neighbor_node_id, 
    uint32_t my_interface_id,
    uint32_t remote_interface_id,
    std::array<uint64_t, NUM_GROUND_STATIONS> *neighbor_queueing_distance
) {
    NS_LOG_DEBUG(m_node_id << " interface: " << my_interface_id);
    m_neighbor_queueing_distances.at(my_interface_id) = *neighbor_queueing_distance; 
    m_neighbor_ids.at(my_interface_id) = neighbor_node_id;
    m_neighbor_interfaces.at(my_interface_id) = remote_interface_id;
}

void ArbiterSingleForward::SetSingleForwardState(int32_t target_node_id, int32_t next_node_id, int32_t own_if_id, int32_t next_if_id) {
    NS_ABORT_MSG_IF(next_node_id == -2 || own_if_id == -2 || next_if_id == -2, "Not permitted to set invalid (-2).");
    m_next_hop_list[target_node_id] = std::make_tuple(next_node_id, own_if_id, next_if_id);
}

void ArbiterSingleForward::ModifyDistanceLookup(int32_t target_node_id, uint32_t distance) {
    NS_ASSERT(target_node_id >= 0);
    NS_ASSERT(GSLNodeIdToGSLIndex(target_node_id) < (int32_t)m_queueing_distances.size());
    m_distance_lookup_array.at(GSLNodeIdToGSLIndex(target_node_id)) = distance;
}

void ArbiterSingleForward::SetDestinationSatelliteList(std::array<std::set<int32_t>, ArbiterSingleForward::NUM_GROUND_STATIONS> *dsl) {
    m_destination_satellite_list = *dsl;
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
