#include "platformerTile.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\video\colourClashSprite.h"

#ifdef AN_MINIGAME_PLATFORMER

using namespace Platformer;

REGISTER_FOR_FAST_CAST(Tile);

LIBRARY_STORED_DEFINE_TYPE(Tile, minigamePlatformerTile);

Tile::Tile(::Framework::Library * _library, ::Framework::LibraryName const & _name)
: base(_library, _name)
{
}

Tile::~Tile()
{
}

int Tile::calc_stairs_at(int _x, int _step, int _tileWidth) const
{
	if (stairsUp == 1)
	{
		_x = _x - (mod(_x, _step));
		return _x + _step;
	}
	if (stairsUp == -1)
	{
		_x = _x - (mod(_x, _step));
		return _tileWidth - _x;
	}
	return 0;
}

bool Tile::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	foregroundColourOverride.load_from_xml(_node->first_child_named(TXT("foregroundColour")));
	backgroundColourOverride.load_from_xml(_node->first_child_named(TXT("backgroundColour")));
	if (_node->first_child_named(TXT("brightColour")))
	{
		brightColourOverride = Colour::white;
	}
	if (_node->first_child_named(TXT("noBrightColour")) ||
		_node->first_child_named(TXT("notBrightColour")) ||
		_node->first_child_named(TXT("dimmColour")))
	{
		brightColourOverride = Colour::black;
	}

	String ttCh = _node->get_string_attribute(TXT("char"), String(TXT("#")));
	frameStep = _node->get_int_attribute_or_from_child(TXT("frameStep"), frameStep);
	if (_node->has_attribute(TXT("sprite")))
	{
		::Framework::UsedLibraryStored<::Framework::ColourClashSprite> tile;
		result &= tile.load_from_xml(_node, TXT("sprite"), _lc);
		frames.push_back(tile);
	}
	else
	{
		for_every(tpNode, _node->children_named(TXT("frame")))
		{
			::Framework::UsedLibraryStored<::Framework::ColourClashSprite> tile;
			result &= tile.load_from_xml(tpNode, TXT("sprite"), _lc);
			frames.push_back(tile);
		}
	}
	if (_node->has_attribute_or_child(TXT("foreground")))
	{
		foreground = true;
	}
	if (_node->has_attribute_or_child(TXT("item")))
	{
		isItem = true;
	}
	if (_node->has_attribute_or_child(TXT("blocking")))
	{
		blocks = true;
	}
	if (_node->has_attribute_or_child(TXT("notBlocking")))
	{
		blocks = false;
	}
	if (_node->has_attribute_or_child(TXT("canStepOnto")))
	{
		canStepOnto = true;
	}
	if (_node->has_attribute_or_child(TXT("cantStepOnto")) ||
		_node->has_attribute_or_child(TXT("cannotStepOnto")))
	{
		canStepOnto = false;
	}
	if (_node->has_attribute_or_child(TXT("background")))
	{
		blocks = false;
		canStepOnto = false;
	}
	if (_node->has_attribute_or_child(TXT("kills")))
	{
		kills = true;
	}

	if (_node->has_attribute_or_child(TXT("stairsUpRight")))
	{
		stairsUp = 1;
	}
	if (_node->has_attribute_or_child(TXT("stairsUpLeft")))
	{
		stairsUp = -1;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("stairs")))
	{
		stairsUp = 0;
		if (String::compare_icase(attr->get_as_string(), TXT("upRight")) ||
			String::compare_icase(attr->get_as_string(), TXT("up right")) ||
			String::compare_icase(attr->get_as_string(), TXT("/")))
		{
			stairsUp = 1;
		}
		if (String::compare_icase(attr->get_as_string(), TXT("upLeft")) ||
			String::compare_icase(attr->get_as_string(), TXT("up left")) ||
			String::compare_icase(attr->get_as_string(), TXT("\\")))
		{
			stairsUp = -1;
		}
	}
	if (_node->has_attribute_or_child(TXT("autoMovementRight")))
	{
		autoMovement = 1;
	}
	if (_node->has_attribute_or_child(TXT("autoMovementLeft")))
	{
		autoMovement = -1;
	}
	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("autoMovement")))
	{
		autoMovement = 0;
		if (String::compare_icase(attr->get_as_string(), TXT("right")) ||
			String::compare_icase(attr->get_as_string(), TXT("}")))
		{
			autoMovement = 1;
		}
		if (String::compare_icase(attr->get_as_string(), TXT("left")) ||
			String::compare_icase(attr->get_as_string(), TXT("{")))
		{
			autoMovement = -1;
		}
	}

	// auto
	if (blocks || autoMovement)
	{
		canStepOnto = true;
	}
	if (stairsUp)
	{
		canStepOnto = true;
		blocks = false;
	}

	return result;
}

bool Tile::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(tp, frames)
	{
		if (tp->is_name_valid())
		{
			result &= tp->prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve);
		}
	}

	return result;
}

#endif
