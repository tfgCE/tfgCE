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
			class A_Tentacle
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(A_Tentacle);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new A_Tentacle(_mind, _logicData); }

			public:
				A_Tentacle(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~A_Tentacle();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
			};
		};
	};
};