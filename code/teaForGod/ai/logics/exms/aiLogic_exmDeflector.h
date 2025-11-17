#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMDeflector
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMDeflector);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMDeflector(_mind, _logicData); }

			public:
				EXMDeflector(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMDeflector();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			private:
				float acceleration = 3.0f;
				float safeRadius = 1.0f;
				float maxDistance = 500.0f;

				bool isPlayingDeflect = false;
				float playDeflectFor = 0.0f;

				SafePtr<Framework::IModulesOwner> deflectParticles;
			};

		};
	};
};
