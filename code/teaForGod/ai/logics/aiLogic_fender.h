#pragma once

#include "aiLogic_npcBase.h"

#include "components\aiLogicComponent_spawnableShield.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Fender
			: public NPCBase
			{
				FAST_CAST_DECLARE(Fender);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Fender(_mind, _logicData); }

			public:
				Fender(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Fender();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(manage_shield);

				int shootCount = 1;
				float shootInterval = 0.5f;
				float shootSetInterval = 0.5f;

				bool shieldRequested = false;
				SpawnableShield shield;
			};

		};
	};
};