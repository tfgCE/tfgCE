#pragma once

#include "aiLogic_npcBase.h"

#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Turret
			: public NPCBase
			{
				FAST_CAST_DECLARE(Turret);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Turret(_mind, _logicData); }

			public:
				Turret(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Turret();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(remain_idle);
				static LATENT_FUNCTION(attack_enemy);

				Vector3 turretTargetMS = Vector3(0.0f, 10.0f, 0.0f);

				Name turretTargetMSVarID;
				SimpleVariableInfo turretTargetMSVar;
				Framework::SocketID turretTargetingSocket;

				Name idleBehvaiour;
				Range scanInterval = Range(0.5f, 1.0f);
				Range scanStep = Range(30.0f, 80.0f);
				Name scanPOI;
			};

		};
	};
};