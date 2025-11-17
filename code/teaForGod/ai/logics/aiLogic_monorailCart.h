#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\framework\ai\aiIMessageHandler.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
	class Actor;
	class DoorInRoom;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModuleBox;
	
	namespace ModuleNavElements
	{
		class Cart;
	};

	namespace AI
	{
		namespace Logics
		{
			class MonorailCart
			: public ::Framework::AI::Logic
			{
				FAST_CAST_DECLARE(MonorailCart);
				FAST_CAST_BASE(::Framework::AI::Logic);
				FAST_CAST_END();

				typedef ::Framework::AI::Logic base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new MonorailCart(_mind, _logicData); }

			public:
				MonorailCart(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~MonorailCart();

			public: // Logic
				implement_ void advance(float _deltaTime);

			};

		};
	};
};
