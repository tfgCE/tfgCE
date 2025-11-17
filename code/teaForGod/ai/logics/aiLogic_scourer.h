#pragma once

#include "aiLogic_npcBase.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Scourer
			: public NPCBase
			{
				FAST_CAST_DECLARE(Scourer);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Scourer(_mind, _logicData); }

			public:
				Scourer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Scourer();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(wander_or_scour);
				static LATENT_FUNCTION(scour);
				static LATENT_FUNCTION(follow_to_scour);
				static LATENT_FUNCTION(deflect);

				int shootCount = 1;

				float maxDeflectDistance = 2.0f;
				float deflectionAcceleration = 1.0f;
				float deflectionSlowDownAcceleration = 1.0f;
				float deflectedProjectileSpeed = 2.0f;

				int lookAtBlocked = 1;

				bool deflectActiveGeneral = true;
				bool shootingNow = true;

				Array<Name> muzzleSockets;
				Array<Name> shootFromMuzzles;
				int shootFromMuzzlesLeft = 0;
			};

		};
	};
};