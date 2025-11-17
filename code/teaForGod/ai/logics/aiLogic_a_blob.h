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
			class A_Blob
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(A_Blob);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new A_Blob(_mind, _logicData); }

			public:
				A_Blob(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~A_Blob();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

				double blobTime = 0.0f;
			};
		};
	};
};