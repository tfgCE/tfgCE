#pragma once

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

	namespace CustomModules
	{
		class Pickup;
	};

	namespace AI
	{
		namespace Logics
		{
			class Item
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Item);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Item(_mind, _logicData); }

			public:
				Item(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Item();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

				CustomModules::Pickup* asPickup = nullptr;
			};

		};
	};
};