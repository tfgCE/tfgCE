#include "platformerCharacter.h"

#include "..\..\library\library.h"
#include "..\..\library\usedLibraryStored.inl"
#include "..\..\video\colourClashSprite.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\other\parserUtils.h"

#ifdef AN_MINIGAME_PLATFORMER

using namespace Platformer;

REGISTER_FOR_FAST_CAST(Character);

LIBRARY_STORED_DEFINE_TYPE(Character, minigamePlatformerCharacter);

Character::Character(::Framework::Library * _library, ::Framework::LibraryName const & _name)
: base(_library, _name)
{
}

Character::~Character()
{
}

#define LOAD_SPRITE_INTO_ARRAY(_nodeName, _array) \
	for_every(containerNode, _node->children_named(_nodeName)) \
	{ \
		for_every(node, containerNode->children_named(TXT("frame"))) \
		{ \
			::Framework::UsedLibraryStored<::Framework::ColourClashSprite> sprite; \
			if (sprite.load_from_xml(node, TXT("sprite"), _lc)) \
			{ \
				_array.push_back(sprite); \
			} \
			else \
			{ \
				result = false; \
			} \
		} \
	}

#define LOAD_SPRITE_INTO_TWO_ARRAYS(_nodeName, _array, _array2) \
	for_every(containerNode, _node->children_named(_nodeName)) \
	{ \
		for_every(node, containerNode->children_named(TXT("frame"))) \
		{ \
			::Framework::UsedLibraryStored<::Framework::ColourClashSprite> sprite; \
			if (sprite.load_from_xml(node, TXT("sprite"), _lc)) \
			{ \
				_array.push_back(sprite); \
				_array2.push_back(sprite); \
			} \
			else \
			{ \
				result = false; \
			} \
		} \
	}

bool Character::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	width = _node->get_int_attribute_or_from_child(TXT("width"), width);
	height = _node->get_int_attribute_or_from_child(TXT("height"), height);

	step = _node->get_int_attribute_or_from_child(TXT("step"), step);

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

	if (_node->first_child_named(TXT("harmless")))
	{
		harmless = true;
	}

	for_every(node, _node->children_named(TXT("movementCollision")))
	{
		movementCollisionOffset.load_from_xml_child_node(node, TXT("offset"));
		movementCollisionBox.x.load_from_xml_child_node(node, TXT("box"), TXT("x"));
		movementCollisionBox.y.load_from_xml_child_node(node, TXT("box"), TXT("y"));
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("jumpPattern")))
	{
		String jumpPatternString = node->get_internal_text();
		List<String> tokens;
		jumpPatternString.split(String::space(), tokens);
		for_every(token, tokens)
		{
			jumpPattern.push_back(ParserUtils::parse_int(*token));
		}
	}

	LOAD_SPRITE_INTO_ARRAY(TXT("left"), leftDirSprites);
	LOAD_SPRITE_INTO_ARRAY(TXT("right"), rightDirSprites);
	LOAD_SPRITE_INTO_ARRAY(TXT("down"), downDirSprites);
	LOAD_SPRITE_INTO_ARRAY(TXT("up"), upDirSprites);
	LOAD_SPRITE_INTO_ARRAY(TXT("noDir"), noDirSprites);

	LOAD_SPRITE_INTO_TWO_ARRAYS(TXT("negative"), leftDirSprites, downDirSprites);
	LOAD_SPRITE_INTO_TWO_ARRAYS(TXT("positive"), rightDirSprites, upDirSprites);
	LOAD_SPRITE_INTO_TWO_ARRAYS(TXT("vertical"), downDirSprites, upDirSprites);
	LOAD_SPRITE_INTO_TWO_ARRAYS(TXT("horizontal"), leftDirSprites, rightDirSprites);

	return result;
}

#define PREPARE_SPRITE_ARRAY(_array) \
	for_every(sprite, _array) \
	{ \
		result &= sprite->prepare_for_game_find(_library, _pfgContext, ::Framework::LibraryPrepareLevel::Resolve); \
	}

bool Character::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	PREPARE_SPRITE_ARRAY(leftDirSprites);
	PREPARE_SPRITE_ARRAY(rightDirSprites);
	PREPARE_SPRITE_ARRAY(downDirSprites);
	PREPARE_SPRITE_ARRAY(upDirSprites);
	PREPARE_SPRITE_ARRAY(noDirSprites);

	return result;
}

::Framework::ColourClashSprite* Character::get_sprite_at(VectorInt2 const & _loc, VectorInt2 const & _dir, int _frameNo)
{
	if (_dir.x < 0 && ! leftDirSprites.is_empty() && step != 0)
	{
		int spriteLoc = _loc.x / step;
		int spriteNo = mod(spriteLoc, leftDirSprites.get_size());
		return leftDirSprites[spriteNo].get();
	}
	if (_dir.x > 0 && !rightDirSprites.is_empty() && step != 0)
	{
		int spriteLoc = _loc.x / step;
		int spriteNo = mod(spriteLoc, rightDirSprites.get_size());
		return rightDirSprites[spriteNo].get();
	}
	if (_dir.y < 0 && !downDirSprites.is_empty() && step != 0)
	{
		int spriteLoc = _loc.y / step;
		int spriteNo = mod(spriteLoc, downDirSprites.get_size());
		return downDirSprites[spriteNo].get();
	}
	if (_dir.y > 0 && !upDirSprites.is_empty() && step != 0)
	{
		int spriteLoc = _loc.y / step;
		int spriteNo = mod(spriteLoc, upDirSprites.get_size());
		return upDirSprites[spriteNo].get();
	}
	if (!noDirSprites.is_empty() && step != 0)
	{
		int spriteNo = mod(_frameNo / step, noDirSprites.get_size());
		return noDirSprites[spriteNo].get();
	}

	return nullptr;
}

#endif
