#pragma once

#include "..\..\framework\library\libraryStored.h"

//

namespace TeaForGodEmperor
{
	/**
	 *	Selection tiers help to determine how many missions of given type there should be
	 *	We first take the top tier to get the max value. Then per mission we check what tier it is and how many there could be.
	 *	We get the max value of possible limits (although it would be better to avoid tiers of the same value)
	 *	Then missions are removed if the limit is exceeded
	 */
	class MissionSelectionTier
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionSelectionTier);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		MissionSelectionTier(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionSelectionTier();

	public:
		int get_tier() const { return tier; }
		int get_limit_for_tier_diff(int _tierDiff) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		int tier = 0;

		struct ForTierDiff
		{
			RangeInt diff = RangeInt::empty; // we use absolute difference, so this value should be, if empty - all
			int limit = 1;
		};
		Array<ForTierDiff> forTierDiffs;
	};
};
