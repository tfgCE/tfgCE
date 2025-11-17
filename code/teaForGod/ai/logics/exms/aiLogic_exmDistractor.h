#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMDistractor
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMDistractor);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMDistractor(_mind, _logicData); }

			public:
				EXMDistractor(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMDistractor();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			private:
				float distractionTime = 2.0f;
			};

		};
	};
};
