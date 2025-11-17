#pragma once

#include "aiLogic_npcBase.h"

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
			class Waiter
				: public NPCBase
			{
				FAST_CAST_DECLARE(Waiter);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Waiter(_mind, _logicData); }

			public:
				Waiter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Waiter();

			private:
				static LATENT_FUNCTION(execute_logic);
			};

		};
	};
};