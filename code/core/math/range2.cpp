#include "math.h"

bool Range2::load_from_xml(IO::XML::Node const * _node, tchar const * _attrX, tchar const * _attrY)
{
	return x.load_from_xml(_node, _attrX) &&
		   y.load_from_xml(_node, _attrY);
}

String Range2::to_string(int _decimals, tchar const * _suffix) const
{
	return String::printf(TXT("x:%S  y:%S"), x.to_string(_decimals, _suffix).to_char(), y.to_string(_decimals, _suffix).to_char());
}

bool Range2::load_from_string(String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String(TXT(";")), tokens);
	if (tokens.get_size() >= 2)
	{
		result &= x.load_from_string(tokens[0]);
		result &= y.load_from_string(tokens[1]);
		return result;
	}
	else if (tokens.get_size() == 1)
	{
		result &= x.load_from_string(tokens[0]);
		y = x;
	}
	return result || _string.is_empty();
}
