#include "arbiter-short-sat.h"

namespace ns3
{

ArbiterShortSat::NeighborCoordContainer::NeighborCoordContainer(double lngd, double rngd, int64_t num_orbits,
																int64_t num_satellites_per_orbit)
	: m_lngd(lngd), m_rngd(rngd), m_num_orbits(num_orbits), m_num_satellites_per_orbit(num_satellites_per_orbit)
{
	m_alpha_base = num_orbits * ArbiterShortSat::CELL_SCALING_FACTOR;
	m_gamma_base = num_satellites_per_orbit * ArbiterShortSat::CELL_SCALING_FACTOR;

	for (auto &i : m_coords)
	{
		i = std::tuple<double, double>(-1, -1);
	}
}

bool ArbiterShortSat::NeighborCoordContainer::VerifyInRange(Direction d, int16_t destination_alpha,
															int16_t destination_gamma)

{
	int16_t alpha_cell = CreateAlphaCell(std::get<0>(m_coords.at(d)));
	int16_t gamma_cell = CreateGammaCell(std::get<1>(m_coords.at(d)));
	return VerifyInRange(std::make_tuple(alpha_cell, gamma_cell), destination_alpha, destination_gamma);
}

bool ArbiterShortSat::NeighborCoordContainer::VerifyInRange(std::tuple<int16_t, int16_t> coords,
															int16_t destination_alpha, int16_t destination_gamma)

{
	return ModularArithmeticHelper::GetModularDistance(std::get<0>(coords), destination_alpha, m_alpha_base) <=
			   ArbiterShortSat::CELL_SCALING_FACTOR &&
		   ModularArithmeticHelper::GetModularDistance(std::get<1>(coords), destination_gamma, m_gamma_base) <=
			   ArbiterShortSat::CELL_SCALING_FACTOR;
}

void ArbiterShortSat::NeighborCoordContainer::UpdateCoords(double alpha, double gamma)
{
	m_coords.at(SELF) = std::tuple<double, double>(alpha, gamma);
	m_coords.at(LEFT) = TransformByDirection(LEFT, alpha, gamma);
	m_coords.at(DOWN) = TransformByDirection(DOWN, alpha, gamma);
	m_coords.at(UP) = TransformByDirection(UP, alpha, gamma);
	m_coords.at(RIGHT) = TransformByDirection(RIGHT, alpha, gamma);
}

std::tuple<double, double> ArbiterShortSat::NeighborCoordContainer::TransformByDirection(Direction d, double alpha,
																						 double gamma)
{
	switch (d)
	{
	case LEFT:
		return std::make_tuple(std::fmod(alpha + (m_num_orbits - 1) * 360.0 / m_num_orbits, 360),
							   std::fmod(gamma + m_lngd + 360, 360));
	case DOWN:
		return std::make_tuple(
			alpha, std::fmod(gamma + (m_num_satellites_per_orbit - 1) * 360.0 / m_num_satellites_per_orbit, 360));
	case UP:
		return std::make_tuple(alpha, std::fmod(gamma + 360.0 / m_num_satellites_per_orbit, 360));
	case RIGHT:
		return std::make_tuple(std::fmod(alpha + 360.0 / m_num_orbits, 360), std::fmod(gamma + m_rngd + 360, 360));
	default:
		NS_ASSERT_MSG(false, "supplied wrong direction to TransformByDirection");
		return std::make_tuple(0, 0);
	}
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetHopcount(Direction d, int16_t destination_alpha,
															 int16_t destination_gamma)
{
	return GetHopcount(GetCoords(d), destination_alpha, destination_gamma);
}

std::tuple<int16_t, int16_t> ArbiterShortSat::NeighborCoordContainer::GetHopcountTuple(Direction d,
																					   int16_t destination_alpha,
																					   int16_t destination_gamma)
{
	return GetHopcountTuple(GetCoords(d), destination_alpha, destination_gamma);
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetHopcount(std::tuple<int16_t, int16_t> coords,
															 int16_t destination_alpha, int16_t destination_gamma)
{
	int16_t horizontal, vertical;
	std::tie(horizontal, vertical) = GetHopcountTuple(coords, destination_alpha, destination_gamma);
	return horizontal + vertical;
}

std::tuple<int16_t, int16_t> ArbiterShortSat::NeighborCoordContainer::GetHopcountTuple(
	std::tuple<int16_t, int16_t> coords, int16_t destination_alpha, int16_t destination_gamma)
{
	// more approximate, since we don't have the underlying gamma
	int16_t coordinate_alpha = std::get<0>(coords);
	int16_t coordinate_gamma = std::get<1>(coords);

	int16_t horizontal_distance = GetAlphaModularDistance(coordinate_alpha, destination_alpha);
	int16_t horizontal_hops = horizontal_distance / ArbiterShortSat::CELL_SCALING_FACTOR;
	int8_t alpha_direction = CheckIfAlphaIncrease(coordinate_alpha, destination_alpha);
	if (alpha_direction == 0)
	{
		return std::make_tuple(horizontal_distance, GetGammaModularDistance(coordinate_gamma, destination_gamma));
	}
	else if (alpha_direction == 1)
	{
		// as long as the inter-orbit phasing is larger than our gamma resolution, then there shouldnt be a problem with
		// this rounding
		// the accumulated phase aliasing to something else isnt an issue for starlink unless you go over half the globe
		double gamma_after_increase =
			std::fmod(double(coordinate_gamma) * 360.0 / (m_gamma_base) + m_rngd * double(horizontal_hops) + 360, 360);
		NS_ASSERT_MSG(0 <= gamma_after_increase && gamma_after_increase <= 360, "gamma was: " << gamma_after_increase);
		return std::make_tuple(horizontal_distance,
							   GetGammaModularDistance(CreateGammaCell(gamma_after_increase), destination_gamma));
	}
	else if (alpha_direction == -1)
	{
		double gamma_after_increase =
			std::fmod(double(coordinate_gamma) * 360.0 / (m_gamma_base) + m_lngd * double(horizontal_hops) + 360, 360);
		NS_ASSERT_MSG(0 <= gamma_after_increase && gamma_after_increase <= 360, "gamma was: " << gamma_after_increase);
		return std::make_tuple(horizontal_distance,
							   GetGammaModularDistance(CreateGammaCell(gamma_after_increase), destination_gamma));
	}
	NS_ASSERT_MSG(false, "direction check failed");
	return std::make_tuple(-1, -1);
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetAlphaModularDistance(Direction d, int16_t destination_alpha)
{

	return ModularArithmeticHelper::GetModularDistance(CreateAlphaCell(std::get<0>(m_coords.at(d))), destination_alpha,
													   m_alpha_base);
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetAlphaModularDistance(int16_t coordinate_alpha,
																		 int16_t destination_alpha)
{

	return ModularArithmeticHelper::GetModularDistance(coordinate_alpha, destination_alpha, m_alpha_base);
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetGammaModularDistance(Direction d, int16_t destination_gamma)
{
	return ModularArithmeticHelper::GetModularDistance(CreateGammaCell(std::get<1>(m_coords.at(d))), destination_gamma,
													   m_gamma_base);
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetGammaModularDistance(int16_t coordinate_gamma,
																		 int16_t destination_gamma)
{

	return ModularArithmeticHelper::GetModularDistance(coordinate_gamma, destination_gamma, m_gamma_base);
}

int8_t ArbiterShortSat::NeighborCoordContainer::CheckIfAlphaIncrease(Direction d, int16_t destination_alpha)
{
	return ModularArithmeticHelper::IncreaseToTarget(CreateAlphaCell(std::get<0>(m_coords.at(d))), destination_alpha,
													 m_alpha_base);
}

int8_t ArbiterShortSat::NeighborCoordContainer::CheckIfAlphaIncrease(int16_t source_alpha, int16_t destination_alpha)
{
	return ModularArithmeticHelper::IncreaseToTarget(source_alpha, destination_alpha, m_alpha_base);
}

int8_t ArbiterShortSat::NeighborCoordContainer::CheckIfGammaIncrease(int16_t source_gamma, int16_t destination_gamma)
{
	return ModularArithmeticHelper::IncreaseToTarget(source_gamma, destination_gamma, m_gamma_base);
}

int8_t ArbiterShortSat::NeighborCoordContainer::CheckIfGammaIncrease(Direction d, int16_t destination_gamma)
{
	return ModularArithmeticHelper::IncreaseToTarget(CreateGammaCell(std::get<1>(m_coords.at(d))), destination_gamma,
													 m_gamma_base);
}

int32_t ArbiterShortSat::NeighborCoordContainer::GetSquaredEuclideanModularDistance(Direction d,
																					int16_t destination_alpha,
																					int16_t destination_gamma)
{
	return ModularArithmeticHelper::GetSquaredEuclideanModularDistance(
		CreateAlphaCell(std::get<0>(m_coords.at(d))), CreateGammaCell(std::get<1>(m_coords.at(d))), destination_alpha,
		destination_gamma, m_alpha_base, m_gamma_base);
}

int32_t ArbiterShortSat::NeighborCoordContainer::GetSquaredEuclideanModularDistance(std::tuple<int16_t, int16_t> c,
																					int16_t destination_alpha,
																					int16_t destination_gamma)
{
	return ModularArithmeticHelper::GetSquaredEuclideanModularDistance(
		std::get<0>(c), std::get<1>(c), destination_alpha, destination_gamma, m_alpha_base, m_gamma_base);
}

std::tuple<int16_t, int16_t> ArbiterShortSat::NeighborCoordContainer::GetCoords(Direction d)
{
	std::tuple<int16_t, int16_t> return_tuple =
		std::make_tuple(CreateAlphaCell(std::get<0>(m_coords.at(d))), CreateGammaCell(std::get<1>(m_coords.at(d))));
	return return_tuple;
}

std::tuple<int16_t, int16_t> ArbiterShortSat::NeighborCoordContainer::GetCoordsFromSequence(std::vector<Direction> &d)
{
	// for each direction, get those coords
	// then apply the appropriate transformation
	std::tuple<double, double> position = m_coords.at(SELF);
	for (auto elem : d)
	{
		position = TransformByDirection(elem, std::get<0>(position), std::get<1>(position));
	}
	NS_ASSERT_MSG(0 <= std::get<0>(position) && std::get<0>(position) <= 360, "alpha off as " << std::get<0>(position));
	NS_ASSERT_MSG(0 <= std::get<1>(position) && std::get<1>(position) <= 360, "gamma off as " << std::get<1>(position));
	return std::make_tuple(CreateAlphaCell(std::get<0>(position)), CreateGammaCell(std::get<1>(position)));
	// return m_coords.at(d);
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetAlphaBase()
{
	return m_alpha_base;
}

int16_t ArbiterShortSat::NeighborCoordContainer::GetGammaBase()
{
	return m_gamma_base;
}

int16_t ArbiterShortSat::NeighborCoordContainer::CreateAlphaCell(double a)
{
	double alpha_mod = std::fmod(a + 360.0, 360);
	int16_t alpha_cell = static_cast<int16_t>(std::round(alpha_mod * m_alpha_base / 360));
	if (alpha_cell == m_alpha_base)
		alpha_cell--;

	NS_ASSERT_MSG(alpha_cell >= 0 && alpha_cell < m_alpha_base, "invalid cells");
	return alpha_cell;
}

int16_t ArbiterShortSat::NeighborCoordContainer::CreateGammaCell(double g)
{
	double gamma_mod = std::fmod(g + 360.0, 360);
	int16_t gamma_cell = static_cast<int16_t>(std::round(gamma_mod * m_gamma_base / 360));
	if (gamma_cell == m_gamma_base)
		gamma_cell--;
	NS_ASSERT_MSG(gamma_cell >= 0 && gamma_cell < m_gamma_base, "invalid cells");
	return gamma_cell;
}

} // namespace ns3
