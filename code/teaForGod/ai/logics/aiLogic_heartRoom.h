#pragma once

#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\game\energy.h"

#include "..\..\utils\interactiveButtonHandler.h"
#include "..\..\utils\overlayStatus.h"

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
			class HeartRoomData;

			/**
			 *	All parts communicate via ai messages - if is on or off
			 */
			class HeartRoom
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(HeartRoom);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new HeartRoom(_mind, _logicData); }

			public:
				HeartRoom(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~HeartRoom();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				HeartRoomData const* heartRoomData = nullptr;

				Random::Generator rg;

				Name element;

				struct MainControl
				{
					float speedPerMinutePerGenerator = 0.05f;
					float speedPerMinuteBonusAllGenerators = 0.2f;

					float state = 0.0f;

					struct Generator
					{
						Framework::IModulesOwner* generator = nullptr; // we don't access it, just for id
						bool isOn = false;
					};
					ArrayStatic<Generator, 5> generators;
					int workingGeneratorsCount = 0;
					int workingGeneratorsCountPrev = 0;

					OverlayStatus overlayStatus;
				} mainControl;
				
				struct Generator
				{
					InteractiveButtonHandler buttonHandler;

					bool working = false;

					bool updateEmissiveRequired = true;

					Energy spawnAmmoEnergyQuantumOnOn = Energy::zero();
					Energy spawnHealthEnergyQuantumOnOn = Energy::zero();
					Framework::UsedLibraryStored<Framework::ItemType> ammoEnergyQuantumType;
					Framework::UsedLibraryStored<Framework::ItemType> healthEnergyQuantumType;
				} generator;

				struct Display
				{
					SafePtr<Framework::IModulesOwner> mainControl;
					::Framework::Display* display = nullptr;

					float displayedState = -1.0f;

					float currentState = 0.0f;
				} display;

				bool readyAndRunning = false;

				void set_generator(Framework::IModulesOwner* _genImo, bool _working);
			};

			class HeartRoomData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(HeartRoomData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new HeartRoomData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class HeartRoom;

				Name element;

				struct TriggerGameScript
				{
					float on = 2.0f;
					Name trap;
				};
				Array<TriggerGameScript> triggerGameScript;
				Name triggerGameScriptTrapOnReachedGoal;
				Name triggerGameScriptTrapOnGeneratorWorking;

				friend class HeartRoom;
			};
		};
	};
};