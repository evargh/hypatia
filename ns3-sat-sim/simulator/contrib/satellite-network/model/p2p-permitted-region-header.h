/**
 * The point of this code is to allow for simple primitives for communication between L2 devices
 * This can integrated with the PointToPointNetDevice by:
 *	Creating 0-size packets that only use this as a header. As a result, this header becomes a "packet"
 *      Prepending this header to existing traffic
 *
 * These two methods are equivalent
 *
 * To work with PPP, we use protocol number 8037 since it's unassigned by the IANA
 */

#ifndef P2P_PERMITTED_REGION_HEADER_H
#define P2P_PERMITTED_REGION_HEADER_H

#include <cstring>
#include <array>
#include <stdint.h>
#include "ns3/mobility-model.h"
#include "ns3/header.h"

namespace ns3 {
/** 
 * Based off of the UDP Header
 * We can implement a checksum later, or potentially source "addresses"
 * We have a diverse set of headers we might want to test, 
 * so making this use templates/generics would need more thought
 */ 
class P2PPermittedRegionHeader : public Header
{

static typedef packet_data_t = std::tuple<std::pair<double, double>, std::pair<double, double>, bool> 

public:
    P2PPermittedRegionHeader ();
    ~P2PPermittedRegionHeader ();

    /**
     * @brief Sets permitted region that the packet allows
     * @param src MobilityManager coordinates of the source, in ITRF format
     * @param dest MobilityManager coordinates of the destination, in ITRF format
     */
    void SetRegionCoords (Ptr<MobilityModel> src, Ptr<MobilityModel> dest);

    /**
     * @brief Gets permitted region that the packet allows in Latitude and Longitude
     * @return a tuple containing two corners of the rectangle, and whether it g
     * 	       through longitude 0
     */
    packed_data_t     
    GetRegionCoordsLatLong ();	

    static TypeId GetTypeId ();

    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
        
private:
    std::pair<double, double> m_src_coords;
    std::pair<double, double> m_dest_coords;
    bool m_routes_through_0;
};
}

#endif

