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
			class ThroneTree
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(ThroneTree);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new ThroneTree(_mind, _logicData); }

			public:
				ThroneTree(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~ThroneTree();

			public: // Logic
				override_ void learn_from(SimpleVariableStorage& _parameters);
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

				static const int MAX_WAVE_COUNT = 8;

				struct Wave
				{
					float offset = 0.0f;
					float speed = 1.0f;
					float lengthFront = 0.1f;
					float lengthBack = 1.0f;
					float power = 0.0f;
					float powerTarget = 1.0f;
					float maxOffset = 10.0f;
					float minOffset = 1.0f;
					Colour colour = Colour(0.3f, 0.9f, 1.0f, 1.0f);
				};

				Array<Wave> waves;

				Random::Generator rg;

				float currentSeparation = 4.0f;

				Range separation = Range(4.0f);
				Range speed = Range(1.0f);
				float backwardChance = 0.0f;
				Range lengthBack = Range(1.0f);
				Range lengthFront = Range(0.1f);
				Range maxOffset = Range(10.0f);
				Range minOffset = Range(10.0f);
				
				float blendIn = 0.3f;
				float blendOut = 2.0f;

			};
		};
	};
};