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
			class EXMEnergyManipulator
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMEnergyManipulator);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMEnergyManipulator(_mind, _logicData); }

			public:
				EXMEnergyManipulator(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMEnergyManipulator();

			public: // Logic
				implement_ void advance(float _deltaTime);

			private:
				bool initialised = false;
				Energy transferSpeed = Energy::zero();
				float transferBit = 0.0f;

				bool transferring = false;
				bool requested = false;
			};

		};
	};
};
