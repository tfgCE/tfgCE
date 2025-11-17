#include "musicTrack.h"

#include "..\..\framework\library\libraryPrepareContext.h"
#include "..\..\framework\sound\soundSample.h"

#ifdef AN_CLANG
#include "..\..\framework\library\usedLibraryStored.inl"
#endif

//

using namespace TeaForGodEmperor;

//
 
#define LOAD_ON_DEMAND_IF_REQUIRED(sample) if (auto* s = sample.get()) { s->load_on_demand_if_required(); }
#define UNLOAD_FOR_LOAD_ON_DEMAND(sample) if (auto* s = sample.get()) { s->unload_for_load_on_demand(); }

//

bool MusicTrack::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute_or_from_child(TXT("id"), id);

	playAtLeast = _node->get_float_attribute_or_from_child(TXT("playAtLeast"), playAtLeast);

	result &= sample.load_from_xml(_node, TXT("sample"), _lc);
	result &= sampleHighIntensity.load_from_xml(_node, TXT("sampleHighIntensity"), _lc);
	result &= sampleHighIntensityOverlay.load_from_xml(_node, TXT("sampleHighIntensityOverlay"), _lc);
	result &= sampleVictoryStinger.load_from_xml(_node, TXT("sampleVictoryStinger"), _lc);

	return result;
}

bool MusicTrack::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	result &= sample.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= sampleHighIntensity.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= sampleHighIntensityOverlay.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= sampleVictoryStinger.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void MusicTrack::load_on_demand_if_required() const
{
	LOAD_ON_DEMAND_IF_REQUIRED(sample);
	LOAD_ON_DEMAND_IF_REQUIRED(sampleHighIntensity);
	LOAD_ON_DEMAND_IF_REQUIRED(sampleHighIntensityOverlay);
	LOAD_ON_DEMAND_IF_REQUIRED(sampleVictoryStinger);
}

void MusicTrack::unload_for_load_on_demand() const
{
	UNLOAD_FOR_LOAD_ON_DEMAND(sample);
	UNLOAD_FOR_LOAD_ON_DEMAND(sampleHighIntensity);
	UNLOAD_FOR_LOAD_ON_DEMAND(sampleHighIntensityOverlay);
	UNLOAD_FOR_LOAD_ON_DEMAND(sampleVictoryStinger);
}