#pragma once

#include "components\aiLogicComponent_upgradeCanister.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class DoorInRoom;
	class Sample;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class ExtraUpgradeOption;
	class LineModel;
	class ModulePilgrim;

	namespace AI
	{
		namespace Logics
		{
			class UpgradeMachineData;

			/**
			 */
			class UpgradeMachine
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(UpgradeMachine);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new UpgradeMachine(_mind, _logicData); }

			public:
				UpgradeMachine(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~UpgradeMachine();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				UpgradeMachineData const * upgradeMachineData = nullptr;
				
				bool markedAsKnownForOpenWorldDirection = false;

				Concurrency::Counter rewardGiven; // if not depleted, extra check?
				Concurrency::SpinLock rewardGivenLock = Concurrency::SpinLock(TXT("TeaForGodEmperor.AI.Logics.UpgradeMachine.rewardGivenLock")); // if not depleted, extra check?

				struct Machine
				{
					enum MachineState
					{
						MachineOff,
						MachineStart,
						MachineShowContent,
						MachineShowUsage,
						MachineShutDown,
					};

					int machineIdx = NONE;
					UpgradeMachine* upgradeMachine = nullptr;

					SafePtr<::Framework::IModulesOwner> displayOwner;
					::Framework::Display* display = nullptr;
					bool displaySetup = false;

					UpgradeCanister canister;

					InteractiveSwitchHandler rerollHandler;

					bool shouldBeOn = false;
					bool contentGiven = false;
					float timeToCheckOn = 0.0f;
					MachineState state = MachineOff;
					bool redrawNow = false;
					float timeToRedraw = 0.0f;
					int inStateDrawnFrames = 0;
					float inStateTime = 0.0f;
					float lastRedrawInStateTime = 0.0f;

					bool rerollEmissive = false;

					struct VariousDrawingVariables
					{
						float shutdownAtPt = 0.0f;
						LineModel* contentLineModel = nullptr;
						LineModel* borderLineModel = nullptr;
						float contentDrawLines = 0.0f;
						float usageDrawAtPt = 0.0f;
						float usageDrawLines = 0.0f;
					} variousDrawingVariables;

					void update(REF_ bool& _depleted, float _deltaTime);
					void update_reroll_emissive(bool _on, bool _force = false);

					void reset_display();

					struct GiveRewardContext
					{
						ModulePilgrim* showMapTo = nullptr;
						Array<VectorInt2> revealedSpecialDevices;
					};
					bool give_reward(REF_ GiveRewardContext & _context); // returns true if given

					~Machine();
				};

				struct ExecutionData
				: public RefCountObject
				{
					ArrayStatic<Machine, 3> machines;
					bool machinesReady = false;
					bool firstUpgradeMachine = false;
					bool doAnalysingSequence = false;
					Framework::UsedLibraryStored<Framework::Sample> doAnalysingSequence_lineStart;
					Framework::UsedLibraryStored<Framework::Sample> doAnalysingSequence_lineMid;
					Framework::UsedLibraryStored<Framework::Sample> doAnalysingSequence_lineEnd;
					bool activateNavigation = false;
					Framework::UsedLibraryStored<Framework::Sample> activateNavigation_line;
					Optional<float> showUsageIn;
					Optional<int> chosenOption;
					Optional<int> chosenReroll[3];
					int freeRerolls = 0;

					bool showUpgradeSelection = false;
					float forceShowUpgradeSelection = 0.0f;
					bool upgradeSelectionVisible = false;

					ExecutionData()
					{
						SET_EXTRA_DEBUG_INFO(machines, TXT("UpgradeMachine::ExecutionData.machines"));
					}
				};

				int freeRerollsLimit = 1;

				RefCountObjectPtr<ExecutionData> executionData;

				bool manageExitDoor = false;
				bool manageEntranceDoor = false;
				SafePtr<Framework::DoorInRoom> entranceDoor;
				SafePtr<Framework::DoorInRoom> exitDoor;
				SafePtr<Framework::Room> initialRoomBehindEntranceDoor; // if we have one door, this is used to check if the door has changed

				void set_chosen_option(int _idx);
				void add_chosen_reroll(int _idx);

				void issue_choose_unlocks();

				friend class UpgradeMachineData;
			};

			class UpgradeMachineData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(UpgradeMachineData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new UpgradeMachineData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class UpgradeMachine;
				friend struct UpgradeMachine::Machine;
	
				struct UpgradeMachineSets
				{
					struct Set
					{
						Array<Name> exms;
					};
					TagCondition forGameDefinitionTagged;
					Array<Set> sets;
				};
				Array<UpgradeMachineSets> upgradeMachineSets;

				int revealSpecialDevicesCellRadius = 6;
				int longRangeRevealSpecialDevicesCellRadius = 10;
				int revealSpecialDevicesAmount = 2;
				Name revealSpecialDevicesWithTag;

				int revealExitsCellRadius = 2;

				int autoMapOnlyRevealCellRadius = 6;
				int autoMapOnlyRevealExitsCellRadius = 4;
				int autoMapOnlyRevealDevicesCellRadius = 3;

				Framework::UsedLibraryStored<LineModel> portLineModel;
				Framework::UsedLibraryStored<LineModel> canisterLineModel;
				Framework::UsedLibraryStored<LineModel> activeEXMLineModel;
				Framework::UsedLibraryStored<LineModel> passiveEXMLineModel;

				// if we run out of exms, we add extras
				float chanceOf1Extra = 0.5f;
				float chanceOf2Extras = 0.05f;
				float chanceOf3Extras = 0.001f;

				TagCondition upgradesTagged;
				TagCondition extraUpgradesTagged;
				
				friend class UpgradeMachine;
			};
		};
	};
};