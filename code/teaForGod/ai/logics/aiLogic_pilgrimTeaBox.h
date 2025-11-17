#pragma once

#include "aiLogic_gameScriptTrapTrigger.h"

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
			class PilgrimTeaBox
			: public GameScriptTrapTrigger
			{
				FAST_CAST_DECLARE(PilgrimTeaBox);
				FAST_CAST_BASE(GameScriptTrapTrigger);
				FAST_CAST_END();

				typedef GameScriptTrapTrigger base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PilgrimTeaBox(_mind, _logicData); }

			public:
				PilgrimTeaBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PilgrimTeaBox();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				Random::Generator rg;

				bool initialUpdateRequired = true;

				Colour notArmedColour = Colour::black;
				Colour armingColour = Colour::yellow;
				Colour armedColour = Colour::red;
				float notArmedPower = 0.0f; 
				float armingLightPower = 0.8f;
				float armedLightPower = 1.5f;

				struct Light
				{
					float timeToUpdate = 0.0f;
					bool armed = false;
					bool arming = false;
				};
				Array<Light> lights;

				float timeActive = 0.0f; // once activated, this is non zero
				float timeLeft = 0.0f;
			};
	
			class PilgrimTeaBoxData
			: public GameScriptTrapTriggerData
			{
				FAST_CAST_DECLARE(PilgrimTeaBox);
				FAST_CAST_BASE(GameScriptTrapTriggerData);
				FAST_CAST_END();

				typedef GameScriptTrapTriggerData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PilgrimTeaBoxData(); }

				PilgrimTeaBoxData();

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Colour notArmedColour = Colour::black;
				Colour armingColour = Colour::yellow;
				Colour armedColour = Colour::red;
				float notArmedPower = 0.0f;
				float armingLightPower = 0.8f;
				float armedLightPower = 1.5f;
				
				friend class PilgrimTeaBox;
			};
		};
	};
};