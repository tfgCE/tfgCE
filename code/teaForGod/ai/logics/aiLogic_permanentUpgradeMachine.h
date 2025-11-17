#pragma once

#include "components\aiLogicComponent_upgradeCanister.h"

#include "..\..\game\energy.h"

#include "..\..\utils\interactiveDialHandler.h"

#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class Sample;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class EXMType;
	class LineModel;

	namespace AI
	{
		namespace Logics
		{
			class PermanentUpgradeMachineData;

			/**
			 */
			class PermanentUpgradeMachine
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(PermanentUpgradeMachine);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PermanentUpgradeMachine(_mind, _logicData); }

			public:
				PermanentUpgradeMachine(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PermanentUpgradeMachine();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				PermanentUpgradeMachineData const * permanentUpgradeMachineData = nullptr;

				bool markedAsKnownForOpenWorldDirection = false;

				SafePtr<::Framework::IModulesOwner> currentPilgrim; // for which we do stuff
				SafePtr<::Framework::IModulesOwner> presentPilgrim; // who is now present

				SafePtr<::Framework::IModulesOwner> displayOwner;
				::Framework::Display* display = nullptr;
				bool displaySetup = false;
				
				bool redrawNow = false;
				float timeToRedraw = 0.0f;

				enum State
				{
					Off,
					Start,
					EnergyRequired,
					ShowContent,
					ShutDown,
				};
				State state = Off;
				float inStateTime = 0.0f;
				int inStateDrawnFrames = 0;

				struct VariousDrawingVariables
				{
					LineModel* contentLineModel = nullptr;
					LineModel* borderLineModel = nullptr;
					float contentDrawLines = 0.0f;
					bool contentDrawn = false;
					float shutdownAtPt = 0.0f;
					int energyRequiredInterval = 0;
				} variousDrawingVariables;

				int upgradeIdx = NONE;
				Array<EXMType*> availableUpgrades;
				bool updateAvailableUpgrades = false;

				InteractiveDialHandler chooseUpgradeHandler;
				UpgradeCanister canister;
				SafePtr<Framework::IModulesOwner> energyKiosk;
				bool forceEnergyKioskOff = false;
				Energy currentEnergyKioskPrice = Energy::zero();
				int priceLevel = 0; // does not modify, up steps

				Framework::UsedLibraryStored<Framework::Sample> speakDeviceRequiresEnergyLine;
				Framework::UsedLibraryStored<Framework::Sample> speakDeviceReadyLine;

				bool is_energy_kiosk_full();
				void use_energy_kiosk();

				void setup_energy_kiosk(Framework::IModulesOwner* _pilgrimIMO);
			};

			class PermanentUpgradeMachineData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PermanentUpgradeMachineData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PermanentUpgradeMachineData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Colour selectColour = Colour::white;

				TagCondition upgradesTagged;
				
				Framework::UsedLibraryStored<LineModel> permanentEXMLineModel;
				Framework::UsedLibraryStored<LineModel> energyRequiredLineModel;
				
				friend class PermanentUpgradeMachine;
			};
		};
	};
};