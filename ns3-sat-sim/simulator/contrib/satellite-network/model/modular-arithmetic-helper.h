#ifndef MODULAR_ARITHMETIC_HELPER_H
#define MODULAR_ARITHMETIC_HELPER_H

#include <cstdint>
#include <cmath>

namespace ModularArithmeticHelper
{
inline int16_t GetModularDistance(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	return distance < base - distance ? distance : base - distance;
}

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
