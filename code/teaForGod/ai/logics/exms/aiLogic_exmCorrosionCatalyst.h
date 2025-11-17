#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMCorrosionCatalyst
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMCorrosionCatalyst);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMCorrosionCatalyst(_mind, _logicData); }

			public:
				EXMCorrosionCatalyst(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMCorrosionCatalyst();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			private:
				float maxDistanceInVisibleRoom = 50.0f;
				float maxDistanceBeyondVisibleRoom = 5.0f;
				Energy catalyseDamage = Energy::zero();
				EnergyCoef catalyseDamageCoef = EnergyCoef::zero();
			};

		};
	};
};
