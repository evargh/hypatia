#ifndef MODULAR_ARITHMETIC_HELPER_H
#define MODULAR_ARITHMETIC_HELPER_H

#include <cstdint>
#include <cmath>

namespace ModularArithmeticHelper
{

/**
 * Get the distance between a and b in the cyclic group with size base.
 *
 * \param a point a
 * \param b point b
 * \param base the size of the cyclic group
 * \return the distance between a and b if the operation was successful (always true actually)
 */
inline int16_t GetModularDistance(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	return distance < base - distance ? distance : base - distance;
}

/**
 * Determine whether a+1 is closer to b in the cyclic group, or if a-1 is closer to b in the cyclic group
 *
 * \param a point a
 * \param b point b
 * \param base the size of the cyclic group
 * \return an integer denoting whether to increase or decrease from a to b (0 if a==b, 1 if a+1 is closer to b, -1 if
 * a-1 is closer to b)
 */
inline int8_t IncreaseToTarget(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	if (a == b)
		return 0;

	// in this edge case, always increase
	if (distance == base - distance)
	{
		return 1;
	}

	if (a < b)
	{
		if (distance < base - distance)
			return 1;
		else
			return -1;
	}
	else
	{
		if (distance < base - distance)
			return -1;
		else
			return 1;
	}
}
} // namespace ModularArithmeticHelper

#endif // MODULAR_ARITHMETIC_HELPER_H
