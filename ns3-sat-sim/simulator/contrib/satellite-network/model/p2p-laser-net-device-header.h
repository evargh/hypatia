/**
 * The point of this code is to allow for simple primitives for communication between L2 devices
 * This can be integrated with the current code by:
 *      Creating a new Send function that is set on a Scheduler. This function enqueues messages that need to be sent to the transmission queue periodically, which are designed as zero-size Packets with this header attached
 *      Modifying the Receive function to be able to identify such packets (by some generic header) and then process their contents
 *
 * Alternatively, the design space can resemble something more like ECN, where packets are modified by the sender to include information about congestion. This can be integrated with the current code by:
 *      Making it so that Send adds a wrapper around the packet, putting it into some structure where the actual data packet is wrapped with a bit of extra data. However, this will require modification of MTU, and potentially reduction of goodput per-packet.
 *
 *      To work with PPP, we use protocol number 8037 since it's unassigned by the IANA
 *
 *      LBRA-CP: ants
 *      NCMCR: sending new packets (RREQs and RREPs)
 *      SALB: state updating messages
 *      SLSR: global link-state transmissions
 *      TLR: step II - notify neighbours about color
 *      DBPR: ultimately a distributed routing algorithm with queue management
 **/

#ifndef P2P_LASER_NET_DEVICE_HEADER_H
#define P2P_LASER_NET_DEVICE_HEADER_H

#include <cstring>
#include <stdint.h>
#include "ns3/header.h"

namespace ns3 {
// based off of the udp header
// for this prototype, i will implement everything as though we only have to track queue fullness (32-bit integer)
// obviously, this can be expanded 
// we can implement a checksum later, or potentially source "addresses". I'm not sure how to generalize operation into templates, since we need serialization
class P2PLaserNetDeviceHeader : public Header
{
public:
    P2PLaserNetDeviceHeader ();
    ~P2PLaserNetDeviceHeader ();

    void SetQueueSize (uint32_t qs);
    uint32_t GetQueueSize ();
    static TypeId GetTypeId ();

    virtual TypeId GetInstanceTypeId (void) const;
    virtual void Print (std::ostream &os) const;
    virtual uint32_t GetSerializedSize (void) const;
    virtual void Serialize (Buffer::Iterator start) const;
    virtual uint32_t Deserialize (Buffer::Iterator start);
        
private:
    uint32_t m_queueSize;
};
}

#endif

