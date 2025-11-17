#pragma once

#include "aiLogic_npcBase.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class TutorialTarget
			: public NPCBase
			{
				FAST_CAST_DECLARE(TutorialTarget);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new TutorialTarget(_mind, _logicData); }

			public:
				TutorialTarget(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~TutorialTarget();

			private:
				static LATENT_FUNCTION(execute_logic);
			};

		};
	};
};