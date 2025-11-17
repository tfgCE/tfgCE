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
			class CoreShaftDischarges
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(CoreShaftDischarges);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new CoreShaftDischarges(_mind, _logicData); }

			public:
				CoreShaftDischarges(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~CoreShaftDischarges();

			public: // AILogic
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

				struct Pair
				{
					struct Placement
					{
						Vector3 loc;
						float distFromPrev = 0.0f;
					};
					ArrayStatic<Placement,8> placements;
					float totalLength = 0.0f;
					float radius = 1.0f;
					RangeInt amount = RangeInt(1);
					float timeLeft = 0.0f;
					Range interval = Range(0.05f);
					Name firstLightningStrikeID; // in a batch - this is to add lights inside
					Name lightningStrikeID;
				};
				Array<Pair> pairs;

			};
		};
	};
};