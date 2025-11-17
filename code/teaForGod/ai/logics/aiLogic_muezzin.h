#pragma once

#include "..\..\game\damage.h"

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
	class DoorInRoom;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Muezzin
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Muezzin);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Muezzin(_mind, _logicData); }

			public:
				Muezzin(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Muezzin();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(horn);

				bool needsToCall = false;
				bool hasBeenDamaged = false;
				SafePtr<Framework::IModulesOwner> damageInstigator;
			};

		};
	};
};