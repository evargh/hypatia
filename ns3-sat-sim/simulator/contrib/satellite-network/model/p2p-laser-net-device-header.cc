#include "p2p-laser-net-device-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(P2PLaserNetDeviceHeader);

P2PLaserNetDeviceHeader::P2PLaserNetDeviceHeader ()
    : m_queueSize (0)
{
}

P2PLaserNetDeviceHeader::~P2PLaserNetDeviceHeader ()
{
    // I like how the UDP header uses magic values, but I have not settled on a way to make that integrate well with user-defined queue sizes
    m_queueSize = 0;
}

void 
P2PLaserNetDeviceHeader::SetQueueSize (uint32_t qs)
{
    m_queueSize = qs;
}

uint32_t
P2PLaserNetDeviceHeader::GetQueueSize ()
{
    return m_queueSize;
}

TypeId
P2PLaserNetDeviceHeader::GetTypeId ()
{  
  static TypeId tid = TypeId ("ns3::P2PLaserNetDeviceHeader")
    .SetParent<Header> ()
    .SetGroupName ("Link")
    .AddConstructor<P2PLaserNetDeviceHeader> ()
  ;
  return tid;
}

TypeId
P2PLaserNetDeviceHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

void
P2PLaserNetDeviceHeader::Print (std::ostream &os) const
{
    os << "queue size: " << m_queueSize;
}

uint32_t
P2PLaserNetDeviceHeader::GetSerializedSize (void) const
{
    // right now, it's just a u32
    return 4;
}

void
P2PLaserNetDeviceHeader::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;

    i.WriteHtonU32 (m_queueSize);
}
    
uint32_t
P2PLaserNetDeviceHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;

    m_queueSize = i.ReadNtohU32 ();

    return GetSerializedSize();
}

}
