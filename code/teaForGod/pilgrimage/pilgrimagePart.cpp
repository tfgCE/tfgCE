#include "pilgrimagePart.h"

#include "pilgrimageEnvironment.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;

//

bool PilgrimagePart::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute_or_from_child(TXT("name"), name);
	nextPart = _node->get_name_attribute_or_from_child(TXT("nextPart"), nextPart);

	for_every(node, _node->children_named(TXT("station")))
	{
		result &= stationRoomType.load_from_xml(node, TXT("roomType"), _lc);
		result &= stationDoorType.load_from_xml(node, TXT("doorType"), _lc);
		result &= stationEntranceRoomType.load_from_xml(node, TXT("entranceRoomType"), _lc);
		result &= stationExitRoomType.load_from_xml(node, TXT("exitRoomType"), _lc);
	}

	result &= containerRegionType.load_from_xml(_node, TXT("containerRegionType"), _lc);
	result &= innerRegionType.load_from_xml(_node, TXT("innerRegionType"), _lc);

	for_every(environmentsNode, _node->children_named(TXT("environments")))
	{
		for_every(node, environmentsNode->children_named(TXT("environment")))
		{
			RefCountObjectPtr<PilgrimageEnvironmentMix> ple;
			ple = new PilgrimageEnvironmentMix();
			if (ple->load_from_xml(node, _lc))
			{
				environments.push_back(ple);
			}
			else
			{
				error_loading_xml(node, TXT("could not load pilgrimage part environment"));
				result = false;
			}
		}
	}

	error_loading_xml_on_assert(stationRoomType.is_name_valid(), result, _node, TXT("no station room type"));
	error_loading_xml_on_assert(stationDoorType.is_name_valid(), result, _node, TXT("no station door type"));
	error_loading_xml_on_assert(stationEntranceRoomType.is_name_valid(), result, _node, TXT("no station entrance room type"));
	error_loading_xml_on_assert(stationExitRoomType.is_name_valid(), result, _node, TXT("no station exit room type"));

	return result;
}

bool PilgrimagePart::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= stationRoomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= stationDoorType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= stationEntranceRoomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= stationExitRoomType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	result &= containerRegionType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= innerRegionType.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	for_every_ref(env, environments)
	{
		result &= env->prepare_for_game(_library, _pfgContext);
	}

	return result;
}
