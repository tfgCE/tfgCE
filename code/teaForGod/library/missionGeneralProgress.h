#pragma once

#include "..\..\framework\library\libraryStored.h"

//

namespace TeaForGodEmperor
{
	struct UnlockableCustomUpgradesContext;

	/**
	 *	These are unlocked and remain unlocked.
	 *	They have to be unlocked by the player in mission briefer or bridge shop.
	 *	Even if at no cost, the player has to press "unlock".
	 */
	class MissionGeneralProgress
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MissionGeneralProgress);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		MissionGeneralProgress(Framework::Library* _library, Framework::LibraryName const& _name);
		virtual ~MissionGeneralProgress();

	public:
		bool is_available_to_play() const; // is available now to be played
		bool is_available_to_unlock(OPTIONAL_ UnlockableCustomUpgradesContext const* _context = nullptr) const; // is available now to unlock, with context it is more strict, checks more requirements

	public:
		bool does_require_unlocking() const { return !requiresToUnlock.is_empty() || unlockCost > 0; }
		
		int get_unlock_cost() const { return unlockCost; }

		TagCondition const& get_required_to_unlock() const { return requiresToUnlock; }
		Tags const& get_playing_gives() const { return playingGives; }
		Tags const& get_success_gives() const { return successGives; }
		Tags const& get_failure_gives() const { return failureGives; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		override_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

	private:
		TagCondition requiresToUnlock; // persistence.missionGeneralProgressInfo
		Tags playingGives; // persistence.missionGeneralProgressInfo
		Tags successGives; // persistence.missionGeneralProgressInfo
		Tags failureGives; // persistence.missionGeneralProgressInfo
		int unlockCost = 0; // merit points
	};
};
