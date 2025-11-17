#include "aiSocialData.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace AI;

bool SocialData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	neverAgainstAllies = _node->get_bool_attribute_or_from_child_presence(TXT("neverAgainstAllies"), neverAgainstAllies);
	sociopath = _node->get_bool_attribute_or_from_child_presence(TXT("sociopath"), sociopath);
	endearing = _node->get_bool_attribute_or_from_child_presence(TXT("endearing"), endearing);
	faction = _node->get_name_attribute_or_from_child(TXT("faction"), faction);

	warn_loading_xml_on_assert(faction.is_valid(), _node, TXT("faction not provided"));

	return result;
}

bool SocialData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return true;
}

