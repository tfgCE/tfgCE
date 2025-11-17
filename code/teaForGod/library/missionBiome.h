#pragma once

#include "..\..\framework\library\libraryStored.h"

//

namespace TeaForGodEmperor
{
	/**
	 *	Biomes unlock and lock automatically
	 *	They usually unlock after certain progress and lock when something has happened.
	 *	Might be used also for different starts/hubs/bases
	 */
	class MissionBiome
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionBiome);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		MissionBiome(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionBiome();

	public:
		bool is_available_to_play() const; // is available now to be played

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		TagCondition requires; // persistence.missionGeneralProgressInfo
	};
};
