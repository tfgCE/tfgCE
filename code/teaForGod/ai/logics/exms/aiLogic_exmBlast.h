#pragma once

#include "..\..\..\teaEnums.h"

#include "..\..\..\..\framework\ai\aiLogicData.h"

#include "aiLogic_exmBase.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMBlastData;

			class EXMBlast
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMBlast);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMBlast(_mind, _logicData); }

			public:
				EXMBlast(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMBlast();

			public: // Logic
				implement_ void advance(float _deltaTime);

			private:
				EXMBlastData const* exmBlastData;
			};
			
			class EXMBlastData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(EXMBlastData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new EXMBlastData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				/*
				Range range = Range(2.0f); // min - full damage, max - no damage
				Energy damage = Energy(2);
				DamageType::Type damageType = DamageType::Plasma;
				Name damageExtraInfo;
				EnergyCoef armourPenetration = EnergyCoef::zero();
				float confussionTime = 0.0f;
				*/

				friend class EXMBlast;
			};
		};
	};
};
