#pragma once

#include "..\..\utils\interactiveButtonHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class GameScriptTrapTriggerData;

			/**
			 */
			class GameScriptTrapTrigger
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(GameScriptTrapTrigger);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new GameScriptTrapTrigger(_mind, _logicData, nullptr); }

			public:
				GameScriptTrapTrigger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction));
				virtual ~GameScriptTrapTrigger();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				GameScriptTrapTriggerData const* data;

				bool initialised = false;
				float initialisationWait = 0.0f;

				InteractiveButtonHandler buttonHandler;
			};
	
			class GameScriptTrapTriggerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(GameScriptTrapTriggerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new GameScriptTrapTriggerData(); }

				GameScriptTrapTriggerData();

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name triggerGameScriptTrap;
				Name buttonIdVar;

				friend class GameScriptTrapTrigger;
			};
		};
	};
};