#include "enums.h"

#include "types/string.h"

#include "math/math.h"

#define M_2_MI 0.000621371f
#define M_2_KM 0.001f
#define M_2_YD 1.0936133f
#define M_2_FT 3.280839895f
#define M_2_INCH 39.3700787f
#define INCH_PER_FT 12

#define EXTRA_SPACE(def) _extraSpace.get(def)? TXT(" ") : TXT("")

#define SIGN (_distInMeters < 0.0f? TXT("-") : (_distInMeters > 0.0f? (String::does_contain(_additonalFormat, TXT("+"))? TXT("+") : TXT("")) : TXT("")))

float MeasurementSystem::to_kilometers(float _distInMeters)
{
	return _distInMeters * M_2_KM;
}

float MeasurementSystem::to_feet(float _distInMeters)
{
	return _distInMeters * M_2_FT;
}

float MeasurementSystem::to_yards(float _distInMeters)
{
	return _distInMeters * M_2_YD;
}

float MeasurementSystem::to_miles(float _distInMeters)
{
	return _distInMeters * M_2_MI;
}

String MeasurementSystem::as_feet_inches(float _distInMeters, Optional<bool> const & _extraSpace, tchar const* _additonalFormat)
{
	int inchesWhole = TypeConversions::Normal::f_i_closest(abs(_distInMeters) * M_2_INCH);
	int inches = mod(inchesWhole, INCH_PER_FT);
	int feet = (inchesWhole - inches) / INCH_PER_FT;
	if (inches != 0)
	{
		return String::printf(TXT("%S%i'%S%i\""), SIGN, feet, EXTRA_SPACE(true), inches);
	}
	else
	{
		return String::printf(TXT("%S%i%S'"), SIGN, feet, EXTRA_SPACE(true));
	}
}

String MeasurementSystem::as_feet(float _distInMeters, Optional<bool> const & _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	float inFeet = abs(_distInMeters) * M_2_FT;
	String format = String::printf(TXT("%S%%.%if%%Sft"), SIGN, _decimals.get(0));
	return String::printf(format.to_char(), abs(inFeet), EXTRA_SPACE(true));
}

String MeasurementSystem::as_yards(float _distInMeters, Optional<bool> const & _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	float inYards = abs(_distInMeters) * M_2_YD;
	String format = String::printf(TXT("%S%%.%if%%Syd"), SIGN, _decimals.get(0));
	return String::printf(format.to_char(), abs(inYards), EXTRA_SPACE(true));
}

String MeasurementSystem::as_feet_speed(float _speedInMetersPerSecond, Optional<bool> const & _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	return as_feet(_speedInMetersPerSecond, _extraSpace, _additonalFormat, _decimals) + TXT("/s");
}

String MeasurementSystem::as_inches(float _distInMeters, Optional<bool> const& _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	float inInches = abs(_distInMeters) * M_2_INCH;
	String format = String::printf(TXT("%S%%.%if%%S''"), SIGN, _decimals.get(0));
	return String::printf(format.to_char(), inInches, EXTRA_SPACE(false));
}

String MeasurementSystem::as_meters(float _distInMeters, Optional<bool> const& _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	String format = String::printf(TXT("%S%%.%if%%Sm"), SIGN, _decimals.get(2));
	return String::printf(format.to_char(), abs(_distInMeters), EXTRA_SPACE(true));
}

String MeasurementSystem::as_meters_speed(float _speedInMetersPerSecond, Optional<bool> const& _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	return as_meters(_speedInMetersPerSecond, _extraSpace, _additonalFormat, _decimals) + TXT("/s");
}

String MeasurementSystem::as_centimeters(float _distInMeters, Optional<bool> const& _extraSpace, tchar const* _additonalFormat, Optional<int> const& _decimals)
{
	String format = String::printf(TXT("%S%%.%if%%Scm"), SIGN, _decimals.get(0));
	return String::printf(format.to_char(), abs(_distInMeters) * 100.0f, EXTRA_SPACE(true));
}

tchar const * MeasurementSystem::to_char(Type _type)
{
	if (_type == Imperial) return TXT("imperial");
	if (_type == Metric) return TXT("metric");
	error(TXT("unknown measurement system"));
	return TXT("metric");
}

MeasurementSystem::Type MeasurementSystem::parse(String const & _string)
{
	if (_string == TXT("imperial")) return Imperial;
	if (_string == TXT("metric")) return Metric;
	error(TXT("unknown measurement system \"%S\""), _string.to_char());
	return Metric;
}
