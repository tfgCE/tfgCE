#pragma once

#include "..\..\game\energy.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

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
			class PilgrimTankData;

			/**
			 */
			class PilgrimTank
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(PilgrimTank);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PilgrimTank(_mind, _logicData); }

			public:
				PilgrimTank(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PilgrimTank();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				PilgrimTankData const * pilgrimTankData = nullptr;

				::Framework::Display* display = nullptr;
				bool displaySetup = false;

				Random::Generator rg;

				struct ExecutionData
				: public RefCountObject
				{
					enum State
					{
						None,
						StartUp,
						Idle,
						Run,
						RunSync // run through ai messages
					};
					bool redrawNow = true;
					float timeSinceLastDraw = 0.0f;
					float timeToRedraw = 0.0f;
					State currentState = State::None;
					int inStateDrawnFrames = 0;

					float timeToRandomise = 0.0f;
					bool randomiseNow = false;
					float timeToChangeBlinking = 0.0f;
					bool changeBlinkingNow = false;
					bool blinkNow = false;
					bool blinkOn = false;
					bool blinkPaused = false;
					int blinkCount = 0;
					Array<VectorInt2> prevBlinking;
					Array<VectorInt2> blinking;
					Array<Colour> colours;

					int idleSize = 0;
					int idleSpacing = 0;
					VectorInt2 idleCount;
					Vector2 idleOffset;

					bool set_state(State _state);
				};

				RefCountObjectPtr<ExecutionData> executionData;

			};

			class PilgrimTankData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PilgrimTankData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PilgrimTankData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Range randomiseInterval = Range(5.0f, 120.0f);
				Range blinkingInterval = Range(3.0f, 20.0f);
				RangeInt blinkingCount = RangeInt(1, 20);
				int blinkingFrames = 5;

				friend class PilgrimTank;
			};
		};
	};
};