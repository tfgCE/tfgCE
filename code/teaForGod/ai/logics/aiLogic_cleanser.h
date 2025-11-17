#pragma once

#include "aiLogic_npcBase.h"
#include "components\aiLogicComponent_upgradeCanister.h"

#include "..\..\..\framework\sound\soundSource.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Cleanser
			: public NPCBase
			{
				FAST_CAST_DECLARE(Cleanser);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Cleanser(_mind, _logicData); }

			public:
				Cleanser(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Cleanser();

			public: // Logic
				override_ void advance(float _deltaTime);

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(do_attack);
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(death_sequence);
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(needler_idling);

			private:
				Random::Generator rg;

				bool firstAdvance = true;

				float spitDuration = 0.3f;

				UpgradeCanister canister;
				Optional<float> canisterGivenTime;
				SafePtr<::Framework::IModulesOwner> runAwayFrom;

				bool needlerEmissiveTarget = false;
				float needlerActiveTarget = 0.0f;
				bool needlerAttackEmissiveTarget = false;
				float needlerAttackActiveTarget = 0.0f;

				bool needlerEmissive = false;
				float needlerActive = 0.0f;
				float needlerDir = 1.0f;
				float needlerTime = 1.0f; // at 0.0f all are synced

				Framework::SoundSourcePtr needlerSound;

				SimpleVariableInfo useSpitPose;
				SimpleVariableInfo useSpitPoseArms;
				SimpleVariableInfo useSpitPoseHit;
				SimpleVariableInfo useStrikeRPose;
				SimpleVariableInfo useStrikeRPoseArm;
				SimpleVariableInfo useStrikeRPoseHit;
				SimpleVariableInfo arm_rf_animated;
				SimpleVariableInfo arm_lf_animated;
			};

		};
	};
};