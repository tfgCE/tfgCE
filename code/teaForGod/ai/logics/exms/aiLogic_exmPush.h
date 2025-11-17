#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMPush
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMPush);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMPush(_mind, _logicData); }

			public:
				EXMPush(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMPush();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			private:
				float confussionDuration = 0.0f;
				float coneAngle = 90.0f; // whole cone
				float maxOffDistance = 0.5f;
				float distance = 10.0f;
				bool changeMovement = false;
				Name withTag;
				Name allowAttachedWithTag;
				Vector3 velocity = Vector3(0.0f, 2.0f, 0.0f);
			};

		};
	};
};
