#pragma once

#include "aiLogic_npcBase.h"
#include "components\aiLogicComponent_movementGaitTimeBased.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Spitter
			: public NPCBase
			{
				FAST_CAST_DECLARE(Spitter);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Spitter(_mind, _logicData); }

			public:
				Spitter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Spitter();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(shoot);
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(wander_scanning);
				static LATENT_FUNCTION(execute_logic);

				int shootCount = 1;
				float shootInterval = 0.2f;
				float shootSetInterval = 100.0f;

				SafePtr<::Framework::IModulesOwner> runAwayFrom;

				Array<ShotInfo> bellyShotInfos;

				MovementGaitTimeBased movementGaitTimeBased;
			};

		};
	};
};