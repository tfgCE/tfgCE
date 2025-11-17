#include "missionResult.h"

#include "playerSetup.h"

#include "..\game\game.h"
#include "..\library\library.h"
#include "..\modules\custom\mc_pickup.h"
#include "..\modules\gameplay\equipment\me_gun.h"

#include "..\..\framework\library\usedLibraryStored.inl"
#include "..\..\framework\object\object.h"
#include "..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

void MissionResult::set_mission(MissionDefinition* _md, MissionDifficultyModifier* _mdm)
{
	an_assert(!missionDefinition.is_set() || missionDefinition.get() == _md);
	missionDefinition = _md;
	difficultyModifier = _mdm;
}

MissionResult::MissionResult()
{
}

MissionResult::~MissionResult()
{
}

bool MissionResult::save_to_xml(IO::XML::Node* _node) const
{
	bool result = true;

	if (missionDefinition.is_name_valid())
	{
		_node->set_attribute(TXT("missionDefinition"), missionDefinition.get_name().to_string());
	}

	_node->set_bool_attribute(TXT("started"), started);
	_node->set_bool_attribute(TXT("cameBack"), cameBack);
	_node->set_bool_attribute(TXT("success"), success);
	_node->set_bool_attribute(TXT("failed"), failed);
	_node->set_bool_attribute(TXT("died"), died);

	_node->set_int_attribute(TXT("visitedCells"), visitedCells);
	_node->set_int_attribute(TXT("visitedInterfaceBoxes"), visitedInterfaceBoxes);
	_node->set_int_attribute(TXT("broughtItems"), broughtItems);
	_node->set_int_attribute(TXT("hackedBoxes"), hackedBoxes);
	_node->set_int_attribute(TXT("stoppedInfestations"), stoppedInfestations);
	_node->set_int_attribute(TXT("refills"), refills);

	appliedBonus.save_to_xml(_node->add_node(TXT("appliedBonus")));

	stats.save_to_xml(_node->add_node(TXT("stats")));

	return result;
}

bool MissionResult::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	missionDefinition.load_from_xml(_node, TXT("missionDefinition")); // full name expected!

	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, started);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, cameBack);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, success);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, failed);
	XML_LOAD_BOOL_ATTR_CHILD_PRESENCE(_node, died);

	XML_LOAD_INT_ATTR_CHILD(_node, visitedCells);
	XML_LOAD_INT_ATTR_CHILD(_node, visitedInterfaceBoxes);
	XML_LOAD_INT_ATTR_CHILD(_node, broughtItems);
	XML_LOAD_INT_ATTR_CHILD(_node, hackedBoxes);
	XML_LOAD_INT_ATTR_CHILD(_node, stoppedInfestations);
	XML_LOAD_INT_ATTR_CHILD(_node, refills);

	appliedBonus.reset();
	for_every(n, _node->children_named(TXT("appliedBonus"))) { appliedBonus.load_from_xml(n); }

	stats.reset();
	stats.load_from_xml_child_node(_node, TXT("stats"));

	return result;
}

bool MissionResult::resolve_library_links()
{
	bool result = true;

	result &= missionDefinition.find_may_fail(Framework::Library::get_current());

	return result;
}