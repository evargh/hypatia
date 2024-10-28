#include "p2p-permitted-region-header.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(P2PPermittedRegionHeader);

P2PPermittedRegionHeader::P2PPermittedRegionHeader ()
{
}

P2PPermittedRegionHeader::~P2PPermittedRegionHeader ()
{
}

void
SetRegionCoords (Ptr<MobilityModel> src, Ptr<MobilityModel> dest)
{

}

P2PPermittedRegionHeader::packed_data_t
P2PPermittedRegionHeader::GetRegionCoordsLatLong ()
{
    return  std::make_tuple(m_src_coords, m_dest_coords, m_routes_through_0);
}

TypeId
P2PPermittedRegionHeader::GetTypeId ()
{  
  static TypeId tid = TypeId ("ns3::P2PPermittedRegionHeader")
    .SetParent<Header> ()
    .SetGroupName ("Link")
    .AddConstructor<P2PPermittedRegionHeader> ()
  ;
  return tid;
}

TypeId
P2PPermittedRegionHeader::GetInstanceTypeId () const
{
    return GetTypeId ();
}

void
P2PPermittedRegionHeader::Print (std::ostream &os) const
{
    os << "my packet";
}

uint32_t
P2PPermittedRegionHeader::GetSerializedSize (void) const
{
    // 4 doubles (8 byte), one boolean (1 byte)
    return 4*8 + 1;
}

void
P2PPermittedRegionHeader::Serialize (Buffer::Iterator start) const
{
    Buffer::Iterator i = start;
    for(auto item : m_src_coords) {
        uint64_t *item_reinterpreted = reinterpret_cast<*uint64_t> *item;
        i.WriteHtonU64(*item_reinterpreted);
    }
    for(auto item : m_dest_coords) {
        uint64_t *item_reinterpreted = reinterpret_cast<*uint64_t> *item;
        i.WriteHtonU64(*item_reinterpreted);
    }
    i.WriteU8(m_routes_through_0);
}
    
uint32_t
P2PPermittedRegionHeader::Deserialize (Buffer::Iterator start)
{
    Buffer::Iterator i = start;
    // fixed size transmission, so we don't need to send any preparatory information
    
    for(size_t j = 0; j < 2; j++) {
        double *item_reinterpreted = reinterpret_cast<*double> *i.ReadNtohU64();
        m_src_coords.at(0) = *item_reinterpeted;
    }
    for(size_t j = 0; j < 2; j++) {
        double *item_reinterpreted = reinterpret_cast<*double> *i.ReadNtohU64();
        m_dest_coords.at(0) = *item_reinterpeted;
    }
    m_bool_routes_through_0 = i.ReadU8();    

    return GetSerializedSize();
}

}
