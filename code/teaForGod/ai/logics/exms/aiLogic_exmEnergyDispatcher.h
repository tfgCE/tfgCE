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
			class EXMEnergyDispatcher
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMEnergyDispatcher);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMEnergyDispatcher(_mind, _logicData); }

			public:
				EXMEnergyDispatcher(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMEnergyDispatcher();

			public: // Logic
				implement_ void advance(float _deltaTime);

			public:
				static Energy redistribute_energy_from(IEnergyTransfer* _source, EnergyType::Type _energyType, Energy const& _amount); // will check if has a pilgrim owner, if that owner has this exm and redistributes energy from "source" to other modules, returns amount of energy transferred

			private:
				bool initialised = false;
				Energy maxDifference = Energy::zero();
				Energy transferSpeed = Energy::zero();
				float transferCooldown = 0.0f;
				float transferBit = 0.0f;

				// runtime
				float cooldownLeft = 0.0f;
				Energy prevHealth = Energy::zero();
				Energy prevHandL = Energy::zero();
				Energy prevHandR = Energy::zero();
			};

		};
	};
};
