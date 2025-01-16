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

#ifndef DHPB_LASER_NET_DEVICE_HEADER_H
#define DHPB_LASER_NET_DEVICE_HEADER_H

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
class DHPBLaserNetDeviceHeader : public Header
{
public:
    DHPBLaserNetDeviceHeader ();
    ~DHPBLaserNetDeviceHeader ();

    void SetQueueDistance (uint64_t qlen, uint32_t distance);
    uint64_t GetQueueDistance ();
    void SetFlow (uint32_t gid);
    uint32_t GetFlow ();

    static TypeId GetTypeId ();

    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
        
private:
    uint64_t m_queue_distance;
    uint64_t m_gid;
};
}

#endif /*DHPB_NET_DEVICE_HEADER*/

