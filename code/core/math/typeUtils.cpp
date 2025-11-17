#include "typeUtils.h"

#include "..\other\parserUtils.h"
#include "..\random\random.h"

//

float floatUtils::get_random(Random::Generator & _generator, float const & _min, float const & _max)
{
	return _generator.get_float(_min, _max);
}

float floatUtils::parse_from(String const & _string, float const & _defValue)
{
	return ParserUtils::parse_float(_string, _defValue);
}

//

int intUtils::get_random(Random::Generator & _generator, int const & _min, int const & _max)
{
	return _generator.get_int_from_range(_min, _max);
}

int intUtils::parse_from(String const & _string, int const & _defValue)
{
	return ParserUtils::parse_int(_string, _defValue);
}

//
