#include "runSetup.h"

#include "game.h"
#include "gameConfig.h"
#include "..\library\gameDefinition.h"
#include "..\library\gameModifier.h"

#include "..\roomGenerators\roomGenerationInfo.h"

#include "..\..\framework\debug\testConfig.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

void RunSetup::read_into_this()
{
	RoomGenerationInfo const& rgi = TeaForGodEmperor::RoomGenerationInfo::get();

	Tags const& generationTags = rgi.get_test_generation_tags().get(rgi.get_requested_generation_tags());

	generationTags.do_for_every_tag([this](Tag const& _tag)
	{
		if (GameModifier::get_all_game_modifiers().does_contain(_tag.get_tag()))
		{
			if (_tag.get_relevance_as_int() == 0)
			{
				activeGameModifiers.remove_tag(_tag.get_tag());
			}
			else
			{
				activeGameModifiers.set_tag(_tag.get_tag());
			}
		}
	});

	//
	//
	//

	post_read_into_this();
}

void RunSetup::post_read_into_this()
{
	// custom alterations
}

void RunSetup::update_rgi_requested_generation_tags(Tags const & activeGameModifiers)
{
	auto& rgi = TeaForGodEmperor::RoomGenerationInfo::access();
	auto& rgiRequestedGenerationTags = rgi.access_requested_generation_tags();

	auto allGameModifiers = GameModifier::get_all_game_modifiers();
	// remove all tags
	for_every(gm, allGameModifiers)
	{
		rgiRequestedGenerationTags.remove_tag(*gm);
	}

	// set all tags
	for_every(gm, allGameModifiers)
	{
		rgiRequestedGenerationTags.set_tag(*gm, activeGameModifiers.get_tag_as_int(*gm) ? 1 : 0);
	}
}

void RunSetup::update_using_this()
{
	// always try to start with full setup
	GameDefinition::setup_difficulty_using_chosen();

	GameModifier::apply(activeGameModifiers);

	update_rgi_requested_generation_tags(activeGameModifiers);
}

bool RunSetup::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	{
		Tags gmTags;
		gmTags.load_from_xml(_node, TXT("gameModifiers"));
		activeGameModifiers.clear();
		gmTags.do_for_every_tag([this](Tag const& _tag)
		{
			activeGameModifiers.set_tag(_tag.get_tag());
		});
	}

	return result;
}

bool RunSetup::load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName)
{
	bool result = true;
	for_every(node, _node->children_named(_childName))
	{
		result &= load_from_xml(node);
	}
	return result;
}

bool RunSetup::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	{
		_node->set_string_to_child(TXT("gameModifiers"), activeGameModifiers.to_string(true));
	}
	return result;
}

bool RunSetup::save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName) const
{
	bool result = true;

	auto* node = _node->add_node(_childName);
	result &= save_to_xml(node);

	return result;
}
