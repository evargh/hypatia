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
DHPBLaserNetDeviceHeader::SetQueueDistances (std::array<uint64_t, 100>* qlens, std::array<uint32_t, 100>* distances)
{
    for(size_t i = 0; i < 100; i++) {
        m_queue_distances.at(i) = qlens->at(i) * uint64_t(distances->at(i));
    }
}

std::array<uint64_t, 100>* 
DHPBLaserNetDeviceHeader::GetQueueDistances ()
{
    return &m_queue_distances;
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
    // 100 u64s
    return 100*8;
}

void
DHPBLaserNetDeviceHeader::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    for(auto item : m_queue_distances) {
        i.WriteHtonU64(item);
    }
}
    
uint32_t
DHPBLaserNetDeviceHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    // fixed size transmission, so we don't need to send any preparatory information
    
    for(size_t j = 0; j < 100; j++) {
        m_queue_distances.at(j) = i.ReadNtohU64();
    }

    return GetSerializedSize();
}

}
