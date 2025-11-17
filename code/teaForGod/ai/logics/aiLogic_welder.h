#pragma once

#include "aiLogic_npcBase.h"

#include "components\aiLogicComponent_spawnableShield.h"

#include "..\aiVarState.h"

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace Framework
{
	class DoorInRoom;
	class ItemType;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace CustomModules
	{
		class Pickup;
	};

	namespace AI
	{
		namespace Logics
		{
			class WelderData;

			class Welder
			: public NPCBase
			{
				FAST_CAST_DECLARE(Welder);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Welder(_mind, _logicData); }

			public:
				Welder(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Welder();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(behave);
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(weld_something);
				static LATENT_FUNCTION(manage_shield);

			private:
				Energy dischargeDamage = Energy(10.0f);
				Name dischargeSocket;

				bool isAttacking = false;

				bool shieldRequested = false;
				SpawnableShield shield;
			};
		
		};
	};
};