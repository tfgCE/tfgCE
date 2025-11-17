#include "math.h"

#include "..\io\xml.h"
#include "..\other\parserUtils.h"

#include "..\types\string.h"
#include "..\containers\list.h"

bool Vector2::load_from_xml(IO::XML::Node const * _node, tchar const * _xAttr, tchar const * _yAttr, bool _keepExistingValues)
{
	if (!_node)
	{
		return false;
	}
	_xAttr = _xAttr ? _xAttr : TXT("x");
	_yAttr = _yAttr ? _yAttr : TXT("y");
	if (! _keepExistingValues &&
		(_node->has_attribute(_xAttr) ||
		 _node->has_attribute(_yAttr)))
	{
		*this = Vector2::zero;
	}
	x = _node->get_float_attribute(_xAttr, x);
	y = _node->get_float_attribute(_yAttr, y);
	return true;
}

bool Vector2::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr, bool _keepExistingValues)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _xAttr, _yAttr, _keepExistingValues);
	}
	return false;
}

bool Vector2::load_from_string(String const & _string)
{
	List<String> tokens;
	_string.split(String::space(), tokens);
	int eIdx = 0;
	for_every(token, tokens)
	{
		if (token->is_empty())
		{
			continue;
		}
		float & e = access_element(eIdx);
		e = ParserUtils::parse_float(*token, e);
		++eIdx;
		if (eIdx >= 2)
		{
			return true;
		}
	}
	return false;
}

bool Vector2::save_to_xml(IO::XML::Node * _node, tchar const * _xAttr, tchar const * _yAttr) const
{
	bool result = true;

	_node->set_float_attribute(_xAttr, x);
	_node->set_float_attribute(_yAttr, y);

	return result;
}

bool Vector2::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _xAttr, tchar const * _yAttr) const
{
	return save_to_xml(_node->add_node(_childNode), _xAttr, _yAttr);
}

bool Vector2::serialise(Serialiser & _serialiser)
{
	bool result = true;
	result &= serialise_data(_serialiser, x);
	result &= serialise_data(_serialiser, y);
	return result;
}

bool Vector2::calc_intersection(Vector2 const & _a, Vector2 const & _aDir, Vector2 const & _b, Vector2 const & _bDir, OUT_ Vector2 & _result)
{
	Vector2 aDir = _aDir.normal();
	Vector2 bDir = _bDir.normal();

	float dotPerp = Vector2::dot_perp(aDir, bDir);
	if (abs(dotPerp) <= 0.0001f)
	{
		// parallel
		return false;
	}

	Vector2 diff = _b - _a;
	float diffDotPerpBDir = Vector2::dot_perp(diff, bDir);
	float t = diffDotPerpBDir / dotPerp;
	_result = _a + t * aDir;

	return true;
}
