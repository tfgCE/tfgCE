#include "aiMind.h"

#include "aiLogics.h"
#include "aiLogicData.h"
#include "aiSocialData.h"

#include "..\..\core\types\names.h"

#include "..\library\library.h"

using namespace Framework;
using namespace AI;

REGISTER_FOR_FAST_CAST(Mind);
LIBRARY_STORED_DEFINE_TYPE(Mind, aiMind);

Mind::Mind(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
{

}

Mind::~Mind()
{
	delete_and_clear(logicData);
	delete_and_clear(socialData);
}

bool Mind::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	locomotionWhenMindDisabled = LocomotionState::parse(_node->get_string_attribute(TXT("locomotionWhenMindDisabled")), locomotionWhenMindDisabled);
	navFlags.load_from_xml(_node, TXT("navFlags"));
	useLocomotion = _node->get_bool_attribute_or_from_child_presence(TXT("useLocomotion"), useLocomotion);
	useLocomotion = ! _node->get_bool_attribute_or_from_child_presence(TXT("noLocomotion"), ! useLocomotion);
	allowWallCrawling = _node->get_bool_attribute_or_from_child_presence(TXT("allowWallCrawling"), allowWallCrawling);
	allowWallCrawling = ! _node->get_bool_attribute_or_from_child_presence(TXT("allowNoWallCrawling"), ! allowWallCrawling);
	assumeWallCrawling = _node->get_bool_attribute_or_from_child_presence(TXT("assumeWallCrawling"), assumeWallCrawling);
	assumeWallCrawling = ! _node->get_bool_attribute_or_from_child_presence(TXT("assumeNoWallCrawling"), ! assumeWallCrawling);
	wallCrawlingYawOffset.load_from_xml(_node, TXT("wallCrawlingYawOffset"));
	processAIMessages = _node->get_bool_attribute_or_from_child_presence(TXT("processAIMessages"), processAIMessages);
	processAIMessages = ! _node->get_bool_attribute_or_from_child_presence(TXT("ignoreAIMessages"), !processAIMessages);

	if (IO::XML::Node const * child = _node->first_child_named(TXT("logic")))
	{
		logicName = child->get_name_attribute(TXT("name"));
		if ((logicData = Logics::create_data(logicName)))
		{
			result &= logicData->load_from_xml(child, _lc);
		}
	}

	if (IO::XML::Node const * child = _node->first_child_named(TXT("social")))
	{
		if ((socialData = new SocialData()))
		{
			result &= socialData->load_from_xml(child, _lc);
		}
	}

	return result;
}

bool Mind::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	if (logicData)
	{
		result &= logicData->prepare_for_game(_library, _pfgContext);
	}

	if (socialData)
	{
		result &= socialData->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

