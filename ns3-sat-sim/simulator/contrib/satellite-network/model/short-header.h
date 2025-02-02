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

#ifndef SHORT_HEADER_H
#define SHORT_HEADER_H

#include <cstring>
#include <tuple>
#include <stdint.h>
#include "ns3/header.h"
#include <cmath>

namespace ns3
{
/**
 * Based off of the UDP Header
 * We can implement a checksum later, or potentially source "addresses"
 * We have a diverse set of headers we might want to test,
 * so making this use templates/generics would need more thought
 *
 * At the
 */
class ShortHeader : public Header
{
  public:
	ShortHeader();
	~ShortHeader();

	// SetCoordinates is only called by the source end host ground station
	void SetCoordinates(double aa, double ag, double da, double dg, int64_t num_orbits, int64_t sats_per_orbit);
	// GetCoordinates may be called by any node peeking at the L3 header
	std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> GetCoordinates();

	static TypeId GetTypeId();

	virtual TypeId GetInstanceTypeId(void) const;
	virtual void Print(std::ostream &os) const;
	virtual uint32_t GetSerializedSize(void) const;
	virtual void Serialize(Buffer::Iterator start) const;
	virtual uint32_t Deserialize(Buffer::Iterator start);

  private:
	uint16_t m_ascending_alpha;
	uint16_t m_ascending_gamma;
	uint16_t m_descending_alpha;
	uint16_t m_descending_gamma;
};
} // namespace ns3

#endif /*SHORT_HEADER*/
