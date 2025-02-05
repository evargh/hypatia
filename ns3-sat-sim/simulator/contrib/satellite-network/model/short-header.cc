#include "short-header.h"
#include "arbiter-short-sat.h"

namespace ns3
{

NS_OBJECT_ENSURE_REGISTERED(ShortHeader);

ShortHeader::ShortHeader()
{
}

ShortHeader::~ShortHeader()
{
}

void ShortHeader::SetCoordinates(double aa, double ag, double da, double dg, int64_t num_orbits, int64_t sats_per_orbit)
{
	// IEEE-754 has no error with -1
	if (aa == -1 && ag == -1 && da == -1 && dg == -1)
	{
		m_ascending_alpha = 65535;
		m_ascending_gamma = 65535;
		m_descending_alpha = 65535;
		m_descending_gamma = 65535;
	}
	else
	{
		NS_ASSERT_MSG(aa >= 0 && aa <= 360 && ag >= 0 && ag <= 360 && da >= 0 && da <= 360 && dg >= 0 && dg <= 360,
					  "invalid short coordinates");
		// clean up

		m_ascending_alpha =
			static_cast<uint16_t>(std::round((aa * num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR) / 360));
		if (m_ascending_alpha == num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR)
			m_ascending_alpha--;

		m_ascending_gamma =
			static_cast<uint16_t>(std::round((ag * sats_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR) / 360));
		if (m_ascending_gamma == sats_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR)
			m_ascending_gamma--;

		m_descending_alpha =
			static_cast<uint16_t>(std::round((da * num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR) / 360));
		if (m_descending_alpha == num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR)
			m_descending_alpha--;

		m_descending_gamma =
			static_cast<uint16_t>(std::round((dg * sats_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR) / 360));
		if (m_descending_gamma == sats_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR)
			m_descending_gamma--;

		NS_ASSERT_MSG(
			m_ascending_alpha >= 0 && m_ascending_alpha < num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR &&
				m_ascending_gamma >= 0 && m_ascending_gamma < sats_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR &&
				m_descending_alpha >= 0 && m_descending_alpha < num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR &&
				m_descending_gamma >= 0 && m_descending_gamma < sats_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR,
			"math error");
	}
}

std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> ShortHeader::GetCoordinates()
{
	return std::make_tuple(m_ascending_alpha, m_ascending_gamma, m_descending_alpha, m_descending_gamma);
}

TypeId ShortHeader::GetTypeId()
{
	static TypeId tid =
		TypeId("ns3::ShortHeader").SetParent<Header>().SetGroupName("Link").AddConstructor<ShortHeader>();
	return tid;
}

TypeId ShortHeader::GetInstanceTypeId() const
{
	return GetTypeId();
}

void ShortHeader::Print(std::ostream &os) const
{
	os << "Short Header";
}

uint32_t ShortHeader::GetSerializedSize(void) const
{
	// 4 u16s
	return 4 * 2;
}

void ShortHeader::Serialize(Buffer::Iterator start) const
{
	Buffer::Iterator i = start;
	i.WriteHtonU16(m_ascending_alpha);
	i.WriteHtonU16(m_ascending_gamma);
	i.WriteHtonU16(m_descending_alpha);
	i.WriteHtonU16(m_descending_gamma);
}

uint32_t ShortHeader::Deserialize(Buffer::Iterator start)
{
	Buffer::Iterator i = start;
	m_ascending_alpha = i.ReadNtohU16();
	m_ascending_gamma = i.ReadNtohU16();
	m_descending_alpha = i.ReadNtohU16();
	m_descending_gamma = i.ReadNtohU16();

	return GetSerializedSize();
}

} // namespace ns3
