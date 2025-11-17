#pragma once

#include "aiLogic_npcBase.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Creeper
			: public NPCBase
			{
				FAST_CAST_DECLARE(Creeper);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Creeper(_mind, _logicData); }

			public:
				Creeper(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Creeper();

			private:
				static LATENT_FUNCTION(execute_logic);
			};

		};
	};
};