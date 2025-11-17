#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMReroll
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMReroll);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMReroll(_mind, _logicData); }

			public:
				EXMReroll(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMReroll();

			public: // Logic
				implement_ void advance(float _deltaTime);
			};

		};
	};
};
