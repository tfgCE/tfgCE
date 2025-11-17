#pragma once

#include "..\aiVarState.h"

#include "..\..\modules\gameplay\moduleEquipment.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"


namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			/**
			 */
			class PortableShield
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(PortableShield);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new PortableShield(_mind, _logicData); }

			public:
				PortableShield(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~PortableShield();

			private:
				static LATENT_FUNCTION(execute_logic);
			};

		};
	};
};