#include "colour.h"
#include "..\math\math.h"
#include "..\io\xml.h"
#include "..\concurrency\mrswLock.h"
#include "..\concurrency\scopedMRSWLock.h"
#include "..\containers\list.h"
#include "..\containers\map.h"
#include "..\types\name.h"
#include "..\types\string.h"
#include "..\other\parserUtils.h"

//

struct RegisteredColour
{
	Name name;
	Colour colour;
};
Array<RegisteredColour>* registeredColours = nullptr;
Concurrency::MRSWLock* registeredColoursLock;

#define INIT_COLOUR(name) \
	register_colour(Name(TXT(#name)), Colour::name);

void RegisteredColours::initialise_static()
{
	an_assert(!registeredColoursLock);
	an_assert(!registeredColours);
	registeredColoursLock = new Concurrency::MRSWLock();
	registeredColours = new Array<RegisteredColour>();

	INIT_COLOUR(black);
	INIT_COLOUR(blackCold);
	INIT_COLOUR(blackWarm);
	INIT_COLOUR(grey);
	INIT_COLOUR(greyLight);
	INIT_COLOUR(white);
	INIT_COLOUR(whiteCold);
	INIT_COLOUR(whiteWarm);
	INIT_COLOUR(red);
	INIT_COLOUR(green);
	INIT_COLOUR(greenDark);
	INIT_COLOUR(blue);
	INIT_COLOUR(purple);
	INIT_COLOUR(magenta);
	INIT_COLOUR(yellow);
	INIT_COLOUR(orange);
	INIT_COLOUR(cyan);
	INIT_COLOUR(mint);
	INIT_COLOUR(gold);

	INIT_COLOUR(zxBlack);
	INIT_COLOUR(zxBlue);
	INIT_COLOUR(zxRed);
	INIT_COLOUR(zxMagenta);
	INIT_COLOUR(zxGreen);
	INIT_COLOUR(zxCyan);
	INIT_COLOUR(zxYellow);
	INIT_COLOUR(zxWhite);
	//
	INIT_COLOUR(zxBlackBright);
	INIT_COLOUR(zxBlueBright);
	INIT_COLOUR(zxRedBright);
	INIT_COLOUR(zxMagentaBright);
	INIT_COLOUR(zxGreenBright);
	INIT_COLOUR(zxCyanBright);
	INIT_COLOUR(zxYellowBright);
	INIT_COLOUR(zxWhiteBright);

	INIT_COLOUR(c64Black);
	INIT_COLOUR(c64White);
	INIT_COLOUR(c64Red);
	INIT_COLOUR(c64Cyan);
	INIT_COLOUR(c64Violet);
	INIT_COLOUR(c64Green);
	INIT_COLOUR(c64Blue);
	INIT_COLOUR(c64Yellow);
	INIT_COLOUR(c64Orange);
	INIT_COLOUR(c64Brown);
	INIT_COLOUR(c64LightRed);
	INIT_COLOUR(c64Grey1);
	INIT_COLOUR(c64Grey2);
	INIT_COLOUR(c64LightGreen);
	INIT_COLOUR(c64LightBlue);
	INIT_COLOUR(c64Grey3);
}

void RegisteredColours::close_static()
{
	delete_and_clear(registeredColours);
	delete_and_clear(registeredColoursLock);
}

#define UPDATE_COLOUR(name) \
	DEFINE_STATIC_NAME(name); \
	if (_name == NAME(name)) \
	{ \
		Colour::name = _colour; \
	}

void RegisteredColours::register_colour(Name const & _name, Colour const & _colour)
{
	an_assert(registeredColours);
	an_assert(registeredColoursLock);

	Concurrency::ScopedMRSWLockWrite lock(*registeredColoursLock);

	bool exists = false;
	for_every(registeredColour, *registeredColours)
	{
		if (registeredColour->name == _name)
		{
			registeredColour->colour = _colour;
			exists = true;
			break;
		}
	}
	if (!exists)
	{
		RegisteredColour rc;
		rc.name = _name;
		rc.colour = _colour;
		registeredColours->push_back(rc);
	}

	UPDATE_COLOUR(black);
	UPDATE_COLOUR(blackCold);
	UPDATE_COLOUR(blackWarm);
	UPDATE_COLOUR(grey);
	UPDATE_COLOUR(greyLight);
	UPDATE_COLOUR(white);
	UPDATE_COLOUR(whiteCold);
	UPDATE_COLOUR(whiteWarm);
	UPDATE_COLOUR(red);
	UPDATE_COLOUR(green);
	UPDATE_COLOUR(greenDark);
	UPDATE_COLOUR(blue);
	UPDATE_COLOUR(purple);
	UPDATE_COLOUR(magenta);
	UPDATE_COLOUR(yellow);
	UPDATE_COLOUR(orange);
	UPDATE_COLOUR(cyan);
	UPDATE_COLOUR(mint);
	UPDATE_COLOUR(gold);
	
	UPDATE_COLOUR(zxBlack);
	UPDATE_COLOUR(zxBlue);
	UPDATE_COLOUR(zxRed);
	UPDATE_COLOUR(zxMagenta);
	UPDATE_COLOUR(zxGreen);
	UPDATE_COLOUR(zxCyan);
	UPDATE_COLOUR(zxYellow);
	UPDATE_COLOUR(zxWhite);
	//
	UPDATE_COLOUR(zxBlackBright);
	UPDATE_COLOUR(zxBlueBright);
	UPDATE_COLOUR(zxRedBright);
	UPDATE_COLOUR(zxMagentaBright);
	UPDATE_COLOUR(zxGreenBright);
	UPDATE_COLOUR(zxCyanBright);
	UPDATE_COLOUR(zxYellowBright);
	UPDATE_COLOUR(zxWhiteBright);
	
	UPDATE_COLOUR(c64Black);
	UPDATE_COLOUR(c64White);
	UPDATE_COLOUR(c64Red);
	UPDATE_COLOUR(c64Cyan);
	UPDATE_COLOUR(c64Violet);
	UPDATE_COLOUR(c64Green);
	UPDATE_COLOUR(c64Blue);
	UPDATE_COLOUR(c64Yellow);
	UPDATE_COLOUR(c64Orange);
	UPDATE_COLOUR(c64Brown);
	UPDATE_COLOUR(c64LightRed);
	UPDATE_COLOUR(c64Grey1);
	UPDATE_COLOUR(c64Grey2);
	UPDATE_COLOUR(c64LightGreen);
	UPDATE_COLOUR(c64LightBlue);
	UPDATE_COLOUR(c64Grey3);
}

Colour RegisteredColours::get_colour(Name const & _name)
{
	an_assert(registeredColours);
	an_assert(registeredColoursLock);

	Concurrency::ScopedMRSWLockRead lock(*registeredColoursLock);

	for_every(registeredColour, *registeredColours)
	{
		if (registeredColour->name == _name)
		{
			return registeredColour->colour;
		}
	}
	return Colour::none;
}

Colour RegisteredColours::get_colour(String const & _name)
{
	an_assert(registeredColours);
	an_assert(registeredColoursLock);

	Concurrency::ScopedMRSWLockRead lock(*registeredColoursLock);

	for_every(registeredColour, *registeredColours)
	{
		if (registeredColour->name.to_string() == _name)
		{
			return registeredColour->colour;
		}
	}
	return Colour::none;
}

//

Colour const Colour::none		(0.000f, 0.000f, 0.000f, 0.000f);
Colour const Colour::full		(1.000f, 1.000f, 1.000f, 1.000f);

Colour Colour::black			(0.000f, 0.000f, 0.000f, 1.000f);
Colour Colour::blackCold		(0.013f, 0.019f, 0.045f, 1.000f);
Colour Colour::blackWarm		(0.045f, 0.013f, 0.019f, 1.000f);
Colour Colour::grey				(0.500f, 0.500f, 0.500f, 1.000f);
Colour Colour::greyLight		(0.750f, 0.750f, 0.750f, 1.000f);
Colour Colour::white			(1.000f, 1.000f, 1.000f, 1.000f);
Colour Colour::whiteCold		(0.968f, 0.975f, 1.000f, 1.000f);
Colour Colour::whiteWarm		(1.000f, 0.968f, 0.975f, 1.000f);
Colour Colour::red				(1.000f, 0.000f, 0.000f, 1.000f);
Colour Colour::green			(0.000f, 1.000f, 0.000f, 1.000f);
Colour Colour::greenDark		(0.000f, 0.500f, 0.000f, 1.000f);
Colour Colour::blue				(0.000f, 0.000f, 1.000f, 1.000f);
Colour Colour::purple			(1.000f, 0.000f, 1.000f, 1.000f);
Colour Colour::magenta			(1.000f, 0.000f, 1.000f, 1.000f);
Colour Colour::yellow			(1.000f, 1.000f, 0.000f, 1.000f);
Colour Colour::orange			(1.000f, 0.392f, 0.000f, 1.000f);
Colour Colour::cyan				(0.078f, 0.855f, 0.855f, 1.000f);
Colour Colour::mint				(0.711f, 0.977f, 0.859f, 1.000f);
Colour Colour::gold				(1.000f, 0.843f, 0.000f, 1.000f);

Colour Colour::zxBlack			(0.000f, 0.000f, 0.000f, 1.000f);
Colour Colour::zxBlue			(0.000f, 0.000f, 0.800f, 1.000f);
Colour Colour::zxRed			(0.800f, 0.000f, 0.000f, 1.000f);
Colour Colour::zxMagenta		(0.800f, 0.000f, 0.800f, 1.000f);
Colour Colour::zxGreen			(0.000f, 0.800f, 0.000f, 1.000f);
Colour Colour::zxCyan			(0.000f, 0.800f, 0.800f, 1.000f);
Colour Colour::zxYellow			(0.800f, 0.800f, 0.000f, 1.000f);
Colour Colour::zxWhite			(0.800f, 0.800f, 0.800f, 1.000f);
//
Colour Colour::zxBlackBright	(0.000f, 0.000f, 0.000f, 1.000f);
Colour Colour::zxBlueBright		(0.000f, 0.000f, 1.000f, 1.000f);
Colour Colour::zxRedBright		(1.000f, 0.000f, 0.000f, 1.000f);
Colour Colour::zxMagentaBright	(1.000f, 0.000f, 1.000f, 1.000f);
Colour Colour::zxGreenBright	(0.000f, 1.000f, 0.000f, 1.000f);
Colour Colour::zxCyanBright		(0.000f, 1.000f, 1.000f, 1.000f);
Colour Colour::zxYellowBright	(1.000f, 1.000f, 0.000f, 1.000f);
Colour Colour::zxWhiteBright	(1.000f, 1.000f, 1.000f, 1.000f);

Colour Colour::c64Black			(0.000f, 0.000f, 0.000f, 1.000f);
Colour Colour::c64White			(1.000f, 1.000f, 1.000f, 1.000f);
Colour Colour::c64Red			(0.533f, 0.000f, 0.000f, 1.000f);
Colour Colour::c64Cyan			(0.667f, 1.000f, 0.933f, 1.000f);
Colour Colour::c64Violet		(0.800f, 0.267f, 0.800f, 1.000f);
Colour Colour::c64Green			(0.000f, 0.800f, 0.333f, 1.000f);
Colour Colour::c64Blue			(0.000f, 0.000f, 0.667f, 1.000f);
Colour Colour::c64Yellow		(0.933f, 0.933f, 0.467f, 1.000f);
Colour Colour::c64Orange		(0.867f, 0.533f, 0.333f, 1.000f);
Colour Colour::c64Brown			(0.400f, 0.267f, 0.000f, 1.000f);
Colour Colour::c64LightRed		(1.000f, 0.467f, 0.467f, 1.000f);
Colour Colour::c64Grey1			(0.200f, 0.200f, 0.200f, 1.000f);
Colour Colour::c64Grey2			(0.467f, 0.467f, 0.467f, 1.000f);
Colour Colour::c64LightGreen	(0.667f, 1.000f, 0.400f, 1.000f);
Colour Colour::c64LightBlue		(0.000f, 0.533f, 1.000f, 1.000f);
Colour Colour::c64Grey3			(0.733f, 0.733f, 0.733f, 1.000f);

Colour::Colour(Vector3 const & _rgb, float _a)
: r(_rgb.x)
, g(_rgb.y)
, b(_rgb.z)
, a(_a)
{
}

Colour::Colour(Vector4 const & _rgba)
: r(_rgba.x)
, g(_rgba.y)
, b(_rgba.z)
, a(_rgba.w)
{
}

String Colour::to_string() const
{
	return String::printf(TXT("r:%.3f g:%.3f b:%.3f a:%.3f"), r, g, b, a);
}

Vector3 Colour::to_vector3() const
{
	return Vector3(r, g, b);
}

Vector4 Colour::to_vector4() const
{
	return Vector4(r, g, b, a);
}

Colour Colour::random_rgb()
{
	// TODO random
	return Colour(((float)(rand() % 256)) / 255.0f, ((float)(rand() % 256)) / 255.0f, ((float)(rand() % 256)) / 255.0f);
}

bool Colour::load_from_xml(IO::XML::Node const * _node, tchar const * _rAttr, tchar const * _gAttr, tchar const * _bAttr, tchar const * _aAttr)
{
	if (!_node)
	{
		return false;
	}
	String const text = _node->get_text();
	float modulate = _node->get_float_attribute(TXT("modulate"), 1.0f);
	if (!text.is_empty())
	{
		if (!load_from_string(text))
		{
			error_loading_xml(_node, TXT("colour \"%S\" not recognised"), text.to_char());
			todo_important(TXT("colour \"%S\" not recognised"), text.to_char());
			return false;
		}
		r *= modulate;
		g *= modulate;
		b *= modulate;
		r += modulate * _node->get_float_attribute(_rAttr, 0.0f);
		g += modulate * _node->get_float_attribute(_gAttr, 0.0f);
		b += modulate * _node->get_float_attribute(_bAttr, 0.0f);
	}
	else
	{
		r = modulate * _node->get_float_attribute(_rAttr, r / max(0.0001f, modulate));
		g = modulate * _node->get_float_attribute(_gAttr, g / max(0.0001f, modulate));
		b = modulate * _node->get_float_attribute(_bAttr, b / max(0.0001f, modulate));
	}
	a = _node->get_float_attribute(_aAttr, a);
	return true;
}

bool Colour::load_from_xml(IO::XML::Attribute const * _attr)
{
	return load_from_string(_attr->get_as_string());
}

bool Colour::save_to_xml_attribute(IO::XML::Node* _node, tchar const* _attr) const
{
	_node->set_attribute(_attr, String::printf(TXT("%.3f %.3f %.3f %.3f"), r, g, b, a));
	return true;
}

bool Colour::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode, tchar const * _rAttr, tchar const * _gAttr, tchar const * _bAttr, tchar const * _aAttr)
{
	if (!_node)
	{
		return false;
	}
	if (IO::XML::Node const * child = _node->first_child_named(_childNode))
	{
		return load_from_xml(child, _rAttr, _gAttr, _bAttr, _aAttr);
	}
	return false;
}

bool Colour::parse_from_string(String const & _string)
{
	if (parse_from_string_internal(_string))
	{
		return true;
	}
	error(TXT("can't parse \"%S\""), _string.to_char());
	return false;
}

bool Colour::parse_from_string_internal(String const & _string)
{
	Colour parsed = RegisteredColours::get_colour(_string);
	if (parsed != Colour::none || _string == TXT("none"))
	{
		*this = parsed;
		return true;
	}
	return false;
}

bool Colour::load_from_string(String const & _string)
{
	if (parse_from_string_internal(_string))
	{
		return true;
	}
	List<String> tokens;
	_string.split(String::space(), tokens);
	int eIdx = 0;
	for_every(token, tokens)
	{
		if (eIdx >= 4)
		{
			error(TXT("too many parameters for a colour"));
			return false;
		}
		if (token->is_empty())
		{
			continue;
		}
		float & e = access_element(eIdx);
		e = ParserUtils::parse_float(*token, e);
		++eIdx;
	}
	if (eIdx == 1)
	{
		b = g = r; // get all from r, monochrome
		return true;
	}
	if (eIdx >= 3)
	{
		return true;
	}
	error(TXT("too little parameters for a colour"));
	return false;
}

bool Colour::load_from_xml_child_or_attr(IO::XML::Node const * _node, tchar const * _attrOrChild)
{
	if (auto* attr = _node->get_attribute(_attrOrChild))
	{
		float modulate = _node->get_float_attribute(TXT("modulate"), 1.0f);
		bool result = load_from_string(attr->get_as_string());
		r *= modulate;
		g *= modulate;
		b *= modulate;
		return result;
	}
	else
	{
		return load_from_xml_child_node(_node, _attrOrChild);
	}
}

bool Colour::save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode, tchar const * _rAttr, tchar const * _gAttr, tchar const * _bAttr, tchar const * _aAttr) const
{
	if (auto* node = _node->add_node(_childNode))
	{
		node->set_float_attribute(_rAttr, r);
		node->set_float_attribute(_gAttr, g);
		node->set_float_attribute(_bAttr, b);
		node->set_float_attribute(_aAttr, a);
	}
	return true;
}

Colour Colour::lerp_smart(float _bWeight, Colour const& _a, Colour const& _b)
{
	// how much each colour should influence rgb
	float aw = _a.a * (1.0f - _bWeight);
	float bw = _b.a * _bWeight;
	// normalise influence/weight
	float sumw = aw + bw;
	float w = 0.5f;
	if (sumw != 0.0f)
	{
		// this is how we actually blend/lerp between two
		w = bw / sumw;
	}
	Colour mixedRGB = lerp(w, _a, _b);

	Colour mixed = mixedRGB;
	mixed.a = _a.a * (1.0f - _bWeight) + _bWeight * _b.a;
	return mixed;
}
