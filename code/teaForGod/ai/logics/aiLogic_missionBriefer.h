#pragma once

#include "..\..\utils\interactiveButtonHandler.h"
#include "..\..\utils\interactiveDialHandler.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"

#include "..\..\game\missionStateObjectives.h"

#include "..\..\library\missionDefinition.h"

#include "..\..\pilgrim\pilgrimOverlayInfo.h"

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
			class MissionBrieferData;

			/**
			 */
			class MissionBriefer
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(MissionBriefer);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new MissionBriefer(_mind, _logicData); }

			public:
				MissionBriefer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~MissionBriefer();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				MissionBrieferData const * missionBrieferData = nullptr;
			
				struct ExecutionData
				: public RefCountObject
				{
					PilgrimageInstanceOpenWorld* ourPilgrimage = nullptr;
					SafePtr<Framework::Door> exitToCapsuleDoor;
					SafePtr<Framework::IModulesOwner> pilgrim;

					InteractiveDialHandler changeDialHandler;
					InteractiveDialHandler change2DialHandler;
					InteractiveSwitchHandler chooseSwitchHandler;
					InteractiveButtonHandler briefingDebriefingButtonHandler;
					bool briefingDebriefingButtonHandlerEmissiveActive = false;

					int sharedPageRef = 0;
					int sharedPageDialAt = 0;

					Array<AvailableMission> availableMissions;
					int briefingPageDialRef = 0;
					int availableMissionDialRef = 0;
					MissionDefinition* chosenMissionDefinition = nullptr; // it's more of an information that we have chosen mission
					AvailableMission* selectedMission = nullptr; // selected from the list
					AvailableMission* showingMissionInfo = nullptr; // actively showing from the list
					int showingBriefingPageCount = 0;
					bool pendingChosenMission = false;

					PilgrimOverlayInfoRenderableData missionItemsRenderableData;

					bool inDebriefing = false;
					int debriefingScreenIdx = 0;
					int debriefingScreenDialRef = 0;
					int debriefingPageDialRef = 0;
					MissionResult* showingDebriefingInfo = nullptr;
					int showingDebriefingPageCount = 0;
					RefCountObjectPtr<MissionResult> lastMissionResult;

					//
					bool updateUnlockStatus = false;
					bool updateStartInfoRequired = false;

					ExecutionData()
					{}
				};

				RefCountObjectPtr<ExecutionData> executionData;
			
				Colour hitIndicatorColour = Colour::white;
				Colour hitIndicatorUnlockColour = Colour::white;
				Colour missionTitleColour = Colour::yellow;
				Colour missionSelectedColour = Colour::orange;
				Colour missionSuccessColour = Colour::green;
				Colour missionFailedColour = Colour::red;
				Colour missionResultColour = Colour::cyan;

				void play_sound(Name const& _sound);
				void stop_sound(Name const& _sound);

				PilgrimOverlayInfo* access_overlay_info();

				void show_mission_info(AvailableMission* _show);
				void show_debriefing_info(MissionResult* _show);
				void hide_any_overlay_info();
				void show_overlay_info();
				void update_shared_page_ref();
				
				void update_page_dial_info();
				void update_task_dial_info_for_not_chosen_mission();
				void update_start_info_for_not_chosen_mission();

				void on_chosen_mission();

				void effect_on_selection(Colour const& _colour, int _count);
					
				friend class MissionBrieferData;
			};

			class MissionBrieferData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(MissionBrieferData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new MissionBrieferData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name overlayPOI; // information in overlay is placed using POI in world
				float overlayScale = 1.0f;
				Name exitToCapsuleDoorTag;

				friend class MissionBriefer;
			};
		};
	};
};