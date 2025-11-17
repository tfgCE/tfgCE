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
			class BigTrainEngineCylinderData;

			/**
			 */
			class BigTrainEngineCylinder
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(BigTrainEngineCylinder);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new BigTrainEngineCylinder(_mind, _logicData); }

			public:
				BigTrainEngineCylinder(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~BigTrainEngineCylinder();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				BigTrainEngineCylinderData const* bigTrainEngineCylinderData = nullptr;

				Random::Generator rg;

				struct Light
				{
					float power = 0.0f;
					Colour colour = Colour::black;

					float timeToChange = 0.0f;
				};
				Array<Light> lights;

				enum State
				{
					Maintenance,
					GoingBad,
					Bad,
					Good
				};
				State state = State::Maintenance;
				float timeToChange = 0.0f;
				int stateSubIdx = 0;

				void switch_state(State _state);
			};

			class BigTrainEngineCylinderData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(BigTrainEngineCylinderData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new BigTrainEngineCylinderData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class BigTrainEngineCylinder;

				Colour colourGood = Colour::green;
				Colour colourBad = Colour::red;
			};
		};
	};
};