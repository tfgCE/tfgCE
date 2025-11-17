#pragma once

#include "aiLogic_npcBase.h"

#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	class DoorInRoom;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Charger
			: public NPCBase
			{
				FAST_CAST_DECLARE(Charger);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Charger(_mind, _logicData); }

			public:
				Charger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Charger();

			public: // Logic
				override_ void advance(float _deltaTime);

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(hatch);
				static LATENT_FUNCTION(perception_blind);
				static LATENT_FUNCTION(attack_or_wander);
				static LATENT_FUNCTION(to_target);
				static LATENT_FUNCTION(discharges);
				static LATENT_FUNCTION(death_sequence);

				InteractiveSwitchHandler switchHandler;

				Framework::RelativeToPresencePlacement possibleTarget;
				bool newKeepAttackingFor = false;
				float keepAttackingFor = 0.0f;
				int keepAttackingForPriority = 0;
				float blockNewKeepAttackingFor = 0.0f;
				bool hitEnemy = false;

				int deathExplosionCount = 3;

				void drop_target();
				void keep_attacking_for(float _keepAttackingFor, Framework::RelativeToPresencePlacement const & _r2pp, float _blockNewKeepAttackingFor = 1.0f, int _keepAttackingForPriority = 0);

				void play_sound(Name const & _sound);
			};

		};
	};
};