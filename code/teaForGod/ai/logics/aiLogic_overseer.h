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

	namespace AI
	{
		namespace Logics
		{
			class Overseer
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Overseer);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Overseer(_mind, _logicData); }

			public:
				Overseer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Overseer();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
			};

		};
	};
};