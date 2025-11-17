#pragma once

#include "..\..\..\core\tags\tagCondition.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class PuzzlePanelData;

			/**
			 */
			class PuzzlePanel
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(PuzzlePanel);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PuzzlePanel(_mind, _logicData); }

			public:
				PuzzlePanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PuzzlePanel();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				PuzzlePanelData const* puzzlePanelData = nullptr;

				Random::Generator rg;

				struct Button
				{
					enum State
					{
						Off,
						Active,
						Error,
						OK,
						MAX
					};
					SafePtr<Framework::IModulesOwner> imo;
					State state = State::Off;
					State altState = State::Off; // for crash
					bool mistake = false;
					
					bool pressed = false;
					bool prevPressed = false;

					void update_emissive(bool _alt = false);
				};
				Array<Button> buttons;
				TagCondition buttonsTagged;

				enum State
				{
					Reset,			// will auto go to next state
					Active,			// the whole sequence with pushing buttons (only here buttons are interactive)
					Mistake,		// if a user made a mistake
					AboutToDone,	// input sequence done, animation to be done
					Done,			// all done
					Crash,			// crash instead of done?
					Off
				};
				State state = State::Reset;

				int stateSubIdx = NONE; // NONE - entered
				float stateSubTime = 0.0f;

				int tryIdx = NONE;

				float currentInterval = 0.0f;
				float interval = 0.0f;
				float intervalIncrease = 0.0f;

				RangeInt addActive = RangeInt(1);

				int currentAddError = 1;
				int startingAddError = 1;
				int addErrorIncreaseTryInterval = 4;

				// onDone behaviour
				bool crashOnDone = false;

				// game script traps
				Name triggerGameScriptExecutionTrapOnGoodButton;
				Name triggerGameScriptExecutionTrapOnBadButton;
				Name triggerGameScriptExecutionTrapOnMistake;
				Name triggerGameScriptExecutionTrapOnAboutToDone;
				Name triggerGameScriptExecutionTrapOnDone;

				void switch_state(State _state);
				void next_state_sub_idx();

				void play_sound(Framework::IModulesOwner* imo, Name const& _sound);
			};

			class PuzzlePanelData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PuzzlePanelData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PuzzlePanelData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class PuzzlePanel;
			};
		};
	};
};