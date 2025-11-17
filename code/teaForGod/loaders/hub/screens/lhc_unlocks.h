#pragma once

#include "..\loaderHubScreen.h"
#include "..\..\..\game\runSetup.h"
#include "..\..\..\utils\unlockableCustomUpgrades.h"
#include "..\..\..\utils\unlockableEXMs.h"

namespace TeaForGodEmperor
{
	class Pilgrimage;
};

namespace Loader
{
	struct HubDraggable;

	namespace HubScreens
	{
		class EXMInfo;
		class CustomUpgradeInfo;
		class PilgrimageInfo;

		class Unlocks
		: public HubScreen
		{
			FAST_CAST_DECLARE(Unlocks);
			FAST_CAST_BASE(HubScreen);
			FAST_CAST_END();

			typedef HubScreen base;
		public:
			Unlocks(Hub* _hub, Name const& _id, Optional<Rotator3> const& _atDir, Vector2 const& _size, float _radius, Vector2 const& _ppa);

		private:
			enum ShowUnlocks
			{
				SU_EXMs,
				SU_Weapons,
				SU_StartingChapters
			};

			static ShowUnlocks lastUnlocksShown;
			static Optional<int> lastUnlocksEXMIdx;
			RefCountObjectPtr<HubDraggable> selectedInfoElement;

			bool simpleRules = true;
			bool runInProgressDisallowsPilgrimageUnlocks = false;
			bool unlockablePilgrimages = true;

			Framework::UsedLibraryStored<TeaForGodEmperor::LineModel> activeEXMBorderLineModel;
			Framework::UsedLibraryStored<TeaForGodEmperor::LineModel> passiveEXMBorderLineModel;
			Framework::UsedLibraryStored<TeaForGodEmperor::LineModel> permanentEXMBorderLineModel;
			Framework::UsedLibraryStored<TeaForGodEmperor::LineModel> pilgrimageBorderLineModel;

			TeaForGodEmperor::Energy xpToSpend = TeaForGodEmperor::Energy::zero();
			int meritPointsToSpend = 0;
			TeaForGodEmperor::UnlockableEXMs unlockableEXMs;
			TeaForGodEmperor::UnlockableCustomUpgrades unlockableCustomUpgrades;

			Array<TeaForGodEmperor::Pilgrimage*> availablePilgrimagesToUnlock;
			Array<TeaForGodEmperor::Pilgrimage*> unlockedPilgrimages;

			void update_unlockables();

			void update_info();
			void unlock_selected();
			TeaForGodEmperor::Energy calculate_unlock_xp_cost();
			int calculate_unlock_merit_points_cost();

			void show_unlocks(Optional<ShowUnlocks> _showUnlocks = NP);

			void show_unlocks_help();

			friend class EXMInfo;
			friend class PilgrimageInfo;
		};
	};
};
