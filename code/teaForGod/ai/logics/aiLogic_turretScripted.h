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
			/**
			 *	Provides aiming interface.
			 */
			class TurretScripted
			: public NPCBase
			{
				FAST_CAST_DECLARE(TurretScripted);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new TurretScripted(_mind, _logicData); }

			public:
				TurretScripted(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~TurretScripted();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

				// can point at a target or at a location
				Framework::RelativeToPresencePlacement target;

				Vector3 targetMS = Vector3(0.0f, 10.0f, 0.0f);
				bool aimAtPresenceCentre = false;
				Optional<Vector3> offsetTS; // target space

				Name turretTargetMSVarID;
				SimpleVariableInfo turretTargetMSVar;
				Framework::SocketID turretTargetingSocket;

			};

		};
	};
};