#pragma once

#include "aiLogic_npcBase.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Enforcer
			: public NPCBase
			{
				FAST_CAST_DECLARE(Enforcer);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Enforcer(_mind, _logicData); }

			public:
				Enforcer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Enforcer();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(execute_logic);

				int shootCount = 1;
				float shootInterval = 0.3f;

			};

		};
	};
};