#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMAttractor
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMAttractor);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMAttractor(_mind, _logicData); }

			public:
				EXMAttractor(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMAttractor();

			public: // Logic
				implement_ void advance(float _deltaTime);
			};

		};
	};
};
