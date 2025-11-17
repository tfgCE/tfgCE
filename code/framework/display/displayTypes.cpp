#include "displayTypes.h"

#include "..\..\core\types\string.h"

using namespace Framework;

//

DisplayHAlignment::Type DisplayHAlignment::parse(String const & _string, DisplayHAlignment::Type _default)
{
	if (_string == TXT("left"))		return DisplayHAlignment::Left;
	if (_string == TXT("right"))	return DisplayHAlignment::Right;
	if (_string == TXT("centre"))	return DisplayHAlignment::Centre;
	if (_string.is_empty())		return _default;
	error(TXT("DisplayHAlignment \"%S\" not recognised, using \"left\""), _string.to_char());
	return DisplayHAlignment::Left;
}

//

DisplayVAlignment::Type DisplayVAlignment::parse(String const & _string, DisplayVAlignment::Type _default)
{
	if (_string == TXT("top"))		return DisplayVAlignment::Top;
	if (_string == TXT("bottom"))	return DisplayVAlignment::Bottom;
	if (_string == TXT("centre"))	return DisplayVAlignment::Centre;
	if (_string.is_empty())		return _default;
	error(TXT("DisplayVAlignment \"%S\" not recognised, using \"top\""), _string.to_char());
	return DisplayVAlignment::Top;
}

//

Vector2 DisplaySelectorDir::to_vector2(DisplaySelectorDir::Type _dir)
{
	Vector2 inDir(0.0f, 0.0f);
	if (_dir == DisplaySelectorDir::Up) inDir = Vector2(0.0f, 1.0f);
	if (_dir == DisplaySelectorDir::Down) inDir = Vector2(0.0f, -1.0f);
	if (_dir == DisplaySelectorDir::Left ||
		_dir == DisplaySelectorDir::Prev ||
		_dir == DisplaySelectorDir::PrevInLineAbove) inDir = Vector2(-1.0f, 0.0f);
	if (_dir == DisplaySelectorDir::Right ||
		_dir == DisplaySelectorDir::Next ||
		_dir == DisplaySelectorDir::NextInLineBelow) inDir = Vector2(1.0f, 0.0f);
	return inDir;
}

//

DisplayCoordinates::Type DisplayCoordinates::parse(String const & _value, Type _default)
{
	if (_value == TXT("char")) return Char;
	if (_value == TXT("pixel")) return Pixel;
	if (!_value.is_empty())
	{
		error(TXT("display coordinates \"%S\" not recognised"), _value.to_char());
	}
	return _default;
}

tchar const * DisplayCoordinates::to_char(Type _value)
{
	if (_value == Char) return TXT("char");
	if (_value == Pixel) return TXT("pixel");
	return TXT("??");
}
