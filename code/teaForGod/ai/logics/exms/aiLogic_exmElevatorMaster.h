#pragma once

#include "..\..\..\teaEnums.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	interface_class IEnergyTransfer;

	namespace AI
	{
		namespace Logics
		{
			class EXMElevatorMaster
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMElevatorMaster);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMElevatorMaster(_mind, _logicData); }

			public:
				EXMElevatorMaster(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMElevatorMaster();

			public: // Logic
				implement_ void advance(float _deltaTime);
				implement_ void end();

			private:
				bool initialised = false;
			};

		};
	};
};
