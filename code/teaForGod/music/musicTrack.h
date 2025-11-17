#pragma once

#include "..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class Sample;
};

namespace TeaForGodEmperor
{
	/**
	 *	Music track samples may have customParameters:
	 *		float	musicTrackFadeTime	if no other fade time defined (via music definition) this one is used
	 *		int		musicTrackBarCount	how many bars (4 beats) are in the sample, required only if beatSyncing (has to be defined for at least one)
	 */
	struct MusicTrack
	{
	public:
		enum Type
		{
			None,
			Ambient,
			AmbientIncidental,
			Combat,
			Incidental
		};

		Name id;
		MusicTrack::Type type = MusicTrack::Ambient; // check MusicDefinition::load_from_xml
		float playAtLeast = 0.0f;
		Framework::UsedLibraryStored<Framework::Sample> sample;
		Framework::UsedLibraryStored<Framework::Sample> sampleHighIntensity; // for combat
		Framework::UsedLibraryStored<Framework::Sample> sampleHighIntensityOverlay; // for combat
		Framework::UsedLibraryStored<Framework::Sample> sampleVictoryStinger; // for combat, when done

	public:
		bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

		void load_on_demand_if_required() const;
		void unload_for_load_on_demand() const;
	};

};
