#include "gameSubDefinition.h"

#include "..\game\game.h"
#include "..\library\library.h"

#include "..\..\core\tags\tag.h"

#include "..\..\framework\library\usedLibraryStored.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(GameSubDefinition);
LIBRARY_STORED_DEFINE_TYPE(GameSubDefinition, gameSubDefinition);

GameSubDefinition::GameSubDefinition(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
, SafeObject<GameSubDefinition>(this)
{
}

GameSubDefinition::~GameSubDefinition()
{
	make_safe_object_unavailable();
}

bool GameSubDefinition::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("selection")))
	{
		selectionPlacement = node->get_float_attribute_or_from_child(TXT("placement"), selectionPlacement);
		playerProfileRequirements.load_from_xml_attribute_or_child_node(node, TXT("playerProfileRequirements"));
	}

	nameForUI.load_from_xml_child(_node, TXT("nameForUI"), _lc, TXT("nameForUI"));
	descForSelectionUI.load_from_xml_child(_node, TXT("descForSelectionUI"), _lc, TXT("descForSelectionUI"));
	descForUI.load_from_xml_child(_node, TXT("descForUI"), _lc, TXT("descForUI"));
	
	for_every(node, _node->children_named(TXT("difficultySettings")))
	{
		difficultySettings = GameSettings::IncompleteDifficulty();
		result &= difficultySettings.load_from_xml(node);
	}

	return result;
}

bool GameSubDefinition::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
