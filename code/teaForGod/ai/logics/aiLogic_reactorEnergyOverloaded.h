#pragma once

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"


namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			/**
			 */
			class ReactorEnergyOverloaded
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(ReactorEnergyOverloaded);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ReactorEnergyOverloaded(_mind, _logicData); }

			public:
				ReactorEnergyOverloaded(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~ReactorEnergyOverloaded();

			public: // AILogic
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

				Array<Transform> reactorPath;
				Name lightningStrikeID;

			};
		};
	};
};