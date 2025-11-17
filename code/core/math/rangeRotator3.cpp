#include "math.h"

#include "..\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

String RangeRotator3::to_string(int _decimals, tchar const * _suffix) const
{
	return String::printf(TXT("pitch:%S  yaw:%S  roll:%S"), pitch.to_string(_decimals, _suffix).to_char(), yaw.to_string(_decimals, _suffix).to_char(), roll.to_string(_decimals, _suffix).to_char());
}

bool RangeRotator3::load_from_xml(IO::XML::Node const * _node, tchar const * _attrX, tchar const * _attrY, tchar const * _attrZ)
{
	return pitch.load_from_xml(_node, _attrX) &&
		   yaw.load_from_xml(_node, _attrY) &&
		   roll.load_from_xml(_node, _attrZ);
}

bool RangeRotator3::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, tchar const * _pitchAttr, tchar const * _yawAttr, tchar const * _rollAttr)
{
	bool result = true;

	for_every(node, _node->children_named(_childName))
	{
		result &= pitch.load_from_xml(node, _pitchAttr);
		result &= yaw.load_from_xml(node, _yawAttr);
		result &= roll.load_from_xml(node, _rollAttr);
	}

	return result;
}

bool RangeRotator3::load_from_string(String const & _string)
{
	bool result = true;

	List<String> tokens;
	_string.split(String(TXT(";")), tokens);
	if (tokens.get_size() == 3)
	{
		pitch.load_from_string(tokens[0]);
		yaw.load_from_string(tokens[1]);
		roll.load_from_string(tokens[2]);
	}
	else if (tokens.get_size() == 1)
	{
		if (!tokens[0].is_empty())
		{
			pitch.load_from_string(tokens[0]);
			yaw = pitch;
			roll = pitch;
		}
	}
	else
	{
		error(TXT("could not parse \"%S\" to RangeRotator3"), _string.to_char());
		result = false;
	}

	return result;
}