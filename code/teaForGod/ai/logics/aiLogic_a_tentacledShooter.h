#pragma once

#include "aiLogic_npcBase.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class A_TentacledShooter
			: public NPCBase
			{
				FAST_CAST_DECLARE(A_TentacledShooter);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new A_TentacledShooter(_mind, _logicData); }

			public:
				A_TentacledShooter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~A_TentacledShooter();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(manage_tentacles);
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(execute_logic);

				int shootCount = 1;
				float shootInterval = 0.1f;
				float shootSetInterval = 2.0f;

			};

		};
	};
};