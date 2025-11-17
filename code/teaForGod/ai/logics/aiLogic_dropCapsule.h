#pragma once

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"


namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			/**
			 */
			class DropCapsule
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(DropCapsule);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new DropCapsule(_mind, _logicData); }

			public:
				DropCapsule(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~DropCapsule();

			private:
				static LATENT_FUNCTION(execute_logic);
			};
		};
	};
};