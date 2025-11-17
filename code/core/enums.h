#pragma once

#include "globalDefinitions.h"
#include "types/optional.h"

struct String;

namespace Allowance
{
	enum Type
	{
		Disallow,
		Allow,
		Force
	};
};

namespace MeasurementSystem
{
	enum Type
	{
		Metric,
		Imperial,
	};
	float to_kilometers(float _distInMeters);
	float to_feet(float _distInMeters);
	float to_yards(float _distInMeters);
	float to_miles(float _distInMeters);
	// _additionalFormat:
	//	+		will put plus if value is positive
	String as_yards(float _distInMeters, Optional<bool> const & _extraSpace = NP /* false */, tchar const * _additonalFormat = nullptr, Optional<int> const& _decimals = NP /* 0 */);
	String as_feet(float _distInMeters, Optional<bool> const & _extraSpace = NP /* false */, tchar const * _additonalFormat = nullptr, Optional<int> const& _decimals = NP /* 0 */);
	String as_feet_speed(float _speedInMetersPerSecond, Optional<bool> const & _extraSpace = NP /* false */, tchar const * _additonalFormat = nullptr, Optional<int> const& _decimals = NP /* 0 */);
	String as_feet_inches(float _distInMeters, Optional<bool> const & _extraSpace = NP /* false */, tchar const * _additonalFormat = nullptr);
	String as_inches(float _distInMeters, Optional<bool> const& _extraSpace = NP /* false */, tchar const* _additonalFormat = nullptr, Optional<int> const& _decimals = NP /* 0 */);
	String as_meters(float _distInMeters, Optional<bool> const & _extraSpace = NP /* false */, tchar const* _additonalFormat = nullptr, Optional<int> const& _decimals = NP /* 2 */);
	String as_meters_speed(float _speedInMetersPerSecond, Optional<bool> const & _extraSpace = NP /* false */, tchar const* _additonalFormat = nullptr, Optional<int> const& _decimals = NP /* 2 */);
	String as_centimeters(float _distInMeters, Optional<bool> const & _extraSpace = NP /* false */, tchar const* _additonalFormat = nullptr, Optional<int> const & _decimals = NP /* 0 */);

	tchar const * to_char(Type _type);
	Type parse(String const & _string);
};

struct ThreadPriority
{
	int type;

	enum Type
	{
		Lowest,
		Lower,
		Normal,
		Higher,
		Highest,

		MAX
	};

	static inline tchar const* to_char(Type _t)
	{
		if (_t == Lowest) return TXT("lowest");
		if (_t == Lower) return TXT("lower");
		if (_t == Normal) return TXT("normal");
		if (_t == Higher) return TXT("higher");
		if (_t == Highest) return TXT("highest");
		return TXT("normal");
	}
};
