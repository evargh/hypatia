#include "dhpb-laser-net-device-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(DHPBLaserNetDeviceHeader);

DHPBLaserNetDeviceHeader::DHPBLaserNetDeviceHeader ()
{
}

DHPBLaserNetDeviceHeader::~DHPBLaserNetDeviceHeader ()
{
}

void 
DHPBLaserNetDeviceHeader::SetFlow (uint32_t gid)
{
    m_gid = gid;
}

uint32_t 
DHPBLaserNetDeviceHeader::GetFlow ()
{
    return m_gid;
}

void 
DHPBLaserNetDeviceHeader::SetQueueDistance (uint64_t qlen, uint32_t distance)
{
    m_queue_distance = qlen * uint64_t(distance);
}

uint64_t
DHPBLaserNetDeviceHeader::GetQueueDistance ()
{
    return m_queue_distance;
}

TypeId
DHPBLaserNetDeviceHeader::GetTypeId ()
{  
  static TypeId tid = TypeId ("ns3::DHPBLaserNetDeviceHeader")
    .SetParent<Header> ()
    .SetGroupName ("Link")
    .AddConstructor<DHPBLaserNetDeviceHeader> ()
  ;
  return tid;
}

TypeId
DHPBLaserNetDeviceHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

void
DHPBLaserNetDeviceHeader::Print (std::ostream &os) const
{
    os << "my packet";
}

uint32_t
DHPBLaserNetDeviceHeader::GetSerializedSize (void) const
{
    // 2 u64s
    return 2*8;
}

void
DHPBLaserNetDeviceHeader::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    i.WriteHtonU64(m_queue_distance);
    i.WriteHtonU64(m_gid);
}
    
uint32_t
DHPBLaserNetDeviceHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    m_queue_distance = i.ReadNtohU64();
    m_gid = i.ReadNtohU64();

    return GetSerializedSize();
}

}
