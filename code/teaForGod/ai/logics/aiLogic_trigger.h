#pragma once

#include "aiLogic_npcBase.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Trigger
			: public NPCBase
			{
				FAST_CAST_DECLARE(Trigger);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Trigger(_mind, _logicData); }

			public:
				Trigger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Trigger();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(execute_logic);

				int shootCount = 1;
				float shootInterval = 0.1f;
				float shootSetInterval = 1.8f;

			};

		};
	};
};