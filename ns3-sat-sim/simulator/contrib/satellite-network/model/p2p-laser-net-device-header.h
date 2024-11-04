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

#ifndef P2P_LASER_NET_DEVICE_HEADER_H
#define P2P_LASER_NET_DEVICE_HEADER_H

#include <cstring>
#include <array>
#include <stdint.h>
#include "ns3/header.h"

namespace ns3 {
/** 
 * Based off of the UDP Header
 * We can implement a checksum later, or potentially source "addresses"
 * We have a diverse set of headers we might want to test, 
 * so making this use templates/generics would need more thought
 *
 * At the
 */ 
class P2PLaserNetDeviceHeader : public Header
{
public:
    P2PLaserNetDeviceHeader ();
    ~P2PLaserNetDeviceHeader ();

    void SetQueueDistances (std::array<uint64_t, 100>* qlens, std::array<uint32_t, 100>* distances);
    std::array<uint64_t, 100>* GetQueueDistances ();
    static TypeId GetTypeId ();

    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
        
private:
    std::array<uint64_t, 100> m_queue_distances;
};
}

#endif

