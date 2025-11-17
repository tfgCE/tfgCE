#pragma once

#include "..\..\game\energy.h"
#include "..\..\pilgrim\pilgrimOverlayInfo.h"
#include "..\..\utils\unlockableEXMs.h"
#include "..\..\utils\unlockableCustomUpgrades.h"

#include "..\..\utils\interactiveButtonHandler.h"
#include "..\..\utils\interactiveDialHandler.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"

namespace Framework
{
	class Door;
};

namespace TeaForGodEmperor
{
	class MissionDefinition;
	class PilgrimageInstanceOpenWorld;
	struct MissionResult;
	struct PilgrimOverlayInfo;

	namespace AI
	{
		namespace Logics
		{
			class BridgeShopData;

			/**
			 */
			class BridgeShop
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(BridgeShop);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new BridgeShop(_mind, _logicData); }

			public:
				BridgeShop(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~BridgeShop();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				void initial_setup();
				void update_unlockables();

			private:
				BridgeShopData const * bridgeShopData = nullptr;
			
				struct Unlockable
				{
					EXMType const * exm = nullptr;
					CustomUpgradeType const* customUpgradeType = nullptr;

					bool operator!=(Unlockable const& other) const { return exm != other.exm || customUpgradeType != other.customUpgradeType; }
				};

				struct ExecutionData
				: public RefCountObject
				{
					bool initialSetupDone = false;

					SafePtr<Framework::IModulesOwner> pilgrim;

					bool showingShop = false;
					bool updateShopRequired = false;

					InteractiveDialHandler changeDialHandler;
					InteractiveSwitchHandler chooseSwitchHandler;
					bool clearToUnlock = true;

					bool simpleRules = false;

					Energy xpToSpend = Energy::zero();
					int meritPointsToSpend = 0;

					UnlockableEXMs unlockableEXMs;
					UnlockableCustomUpgrades unlockableCustomUpgrades;

					Array<Unlockable> unlockables;
					Unlockable selectedUnlockable;

					PilgrimOverlayInfoRenderableData selectedUnlockableRenderableData;

					Framework::UsedLibraryStored<LineModel> oiPermanentEXM;
					Framework::UsedLibraryStored<LineModel> oiPermanentEXMp1;
					Framework::UsedLibraryStored<LineModel> oiPassiveEXM;
					Framework::UsedLibraryStored<LineModel> oiActiveEXM;
					Framework::UsedLibraryStored<LineModel> oiIntegralEXM;

					ExecutionData()
					{}
				};

				RefCountObjectPtr<ExecutionData> executionData;
			
				Colour hitIndicatorColour = Colour::white;

				void play_sound(Name const& _sound);
				void stop_sound(Name const& _sound);

				PilgrimOverlayInfo* access_overlay_info();

				void hide_any_overlay_info();
				void show_overlay_info();

				void update_dial_info();
				void update_select_info();
				
				Energy calculate_unlock_xp_cost();
				int calculate_unlock_merit_points_cost();
				bool unlock_selected();

				friend class BridgeShopData;
			};

			class BridgeShopData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(BridgeShopData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new BridgeShopData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name overlayPOI; // information in overlay is placed using POI in world
				float overlayScale = 1.0f;

				friend class BridgeShop;
			};
		};
	};
};