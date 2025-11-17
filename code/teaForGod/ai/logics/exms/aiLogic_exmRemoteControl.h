#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMRemoteControl
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMRemoteControl);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMRemoteControl(_mind, _logicData); }

			public:
				EXMRemoteControl(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMRemoteControl();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			private:
				float distanceInRoom = 10.0f;
				float distanceBeyondRoom = 5.0f;

				struct ObjectToControl
				{
					SafePtr<Framework::IModulesOwner> imo;
					float delay = 0.0f;
				};
				Array< ObjectToControl> objectsToControl;
			};

		};
	};
};
