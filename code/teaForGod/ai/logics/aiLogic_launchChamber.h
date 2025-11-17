#pragma once

#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\game\energy.h"

#include "..\..\utils\interactiveButtonHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class Display;
	class ItemType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class LaunchChamberData;

			/**
			 *	All parts communicate via ai messages - if is on or off
			 */
			class LaunchChamber
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(LaunchChamber);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new LaunchChamber(_mind, _logicData); }

			public:
				LaunchChamber(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~LaunchChamber();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				LaunchChamberData const* launchChamberData = nullptr;

				Random::Generator rg;

				Name element;

				struct MainControl
				{
					float initialState = 0.0f;
					float minStateRequired = 0.3f;
					float speedPerMinute = 0.2f;
					float drainPerMinute = 0.0f; // till empty

					float state = 0.0f;
					bool working = false;
					bool wasEverWorking = false;

					struct Generator
					{
						Framework::IModulesOwner* generator = nullptr; // we don't access it, just for id
						bool isOn = false;
					};
					ArrayStatic<Generator, 5> generators;
					int workingGeneratorsCount = 0;

					Energy spawnAmmoEnergyQuantum = Energy::zero();
					Energy spawnHealthEnergyQuantum = Energy::zero();

					Framework::UsedLibraryStored<Framework::ItemType> ammoEnergyQuantumType;
					Framework::UsedLibraryStored<Framework::ItemType> healthEnergyQuantumType;
				} mainControl;
				
				struct Generator
				{
					Range worksFor = Range(3.0f, 20.0f);

					float workingTimeLeft = 0.0f;

					InteractiveButtonHandler buttonHandler;

					bool updateEmissiveRequired = true;
				} generator;

				struct Display
				{
					SafePtr<Framework::IModulesOwner> mainControl;
					::Framework::Display* display = nullptr;

					float displayedState = -1.0f;
					Optional<bool> displayedFlash;

					float currentState = 0.0f;
					bool currentWorking = false;
					Optional<bool> currentFlash;
					float flashTimeLeft = 0.0f;
					float minStateRequired = 0.0f;
				} display;

				bool readyAndRunning = false;

				void set_generator(Framework::IModulesOwner* _genImo, bool _working);
			};

			class LaunchChamberData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(LaunchChamberData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new LaunchChamberData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class LaunchChamber;

				Name element;

				Name triggerGameScriptTrapOnWorkingFirstTime;
				Name triggerGameScriptTrapOnMinStateReached;
				Name triggerGameScriptTrapOnGeneratorWorking;
				Name triggerGameScriptTrapOnGeneratorNotWorking;

				friend class LaunchChamber;
			};
		};
	};
};