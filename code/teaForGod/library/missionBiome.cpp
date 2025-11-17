#include "missionBiome.h"

#include "..\game\persistence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(MissionBiome);
LIBRARY_STORED_DEFINE_TYPE(MissionBiome, missionBiome);

MissionBiome::MissionBiome(Framework::Library* _library, Framework::LibraryName const& _name)
: base(_library, _name)
{
}

MissionBiome::~MissionBiome()
{
}

bool MissionBiome::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset
	//
	requires.clear();


	// load
	//
	requires.load_from_xml_attribute_or_child_node(_node, TXT("requires"));


	return result;
}

bool MissionBiome::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

bool MissionBiome::is_available_to_play() const
{
	auto& p = Persistence::get_current();
	return requires.check(p.get_mission_general_progress_info());
}
