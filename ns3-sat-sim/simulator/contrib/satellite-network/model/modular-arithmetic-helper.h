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

inline int32_t GetSquaredEuclideanModularDistance(int16_t x_1, int16_t y_1, int16_t x_2, int16_t y_2, int16_t base_x,
												  int16_t base_y)
{
	int16_t x_dist = GetModularDistance(x_1, x_2, base_x);
	int16_t y_dist = GetModularDistance(y_1, y_2, base_y);
	return static_cast<int32_t>(x_dist) * static_cast<int32_t>(x_dist) +
		   static_cast<int32_t>(y_dist) * static_cast<int32_t>(y_dist);
}

inline int8_t IncreaseToTarget(int16_t a, int16_t b, int16_t base)
{
	int16_t distance = std::abs(a - b);
	if (a == b)
		return 0;

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
