#pragma once

#include "..\..\..\teaEnums.h"

#include "..\..\..\..\framework\library\usedLibraryStored.h"

#include "aiLogic_exmBase.h"

namespace Framework
{
	class ItemType;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EXMShield
			: public EXMBase
			{
				FAST_CAST_DECLARE(EXMShield);
				FAST_CAST_BASE(EXMBase);
				FAST_CAST_END();

				typedef EXMBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EXMShield(_mind, _logicData); }

			public:
				EXMShield(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EXMShield();

				void recall_shield(bool _removeFromWorldImmediately = false, int _count = MAX_SHIELDS);

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				implement_ void advance(float _deltaTime);

			private:
				enum ShieldType
				{
					Hand,
					Stand
				};
				Framework::UsedLibraryStored<Framework::ItemType> spawnShield;
				ShieldType spawnShieldType = ShieldType::Hand;
				Name atHandSocket;

				static int const MAX_SHIELDS = 12;
				ArrayStatic<SafePtr<Framework::IModulesOwner>, MAX_SHIELDS> spawnedShields; // 0 oldest, the greater index the more recent
				Energy initialEnergyState; // for spawned shield
			};

		};
	};
};
