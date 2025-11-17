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
			class Inspecter
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Inspecter);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Inspecter(_mind, _logicData); }

			public:
				Inspecter(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Inspecter();

				bool is_armed() const;
				Framework::IModulesOwner* get_armed_by() const;

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(act_armed);
				static LATENT_FUNCTION(act_normal);

				CustomModules::Pickup* asPickup = nullptr;

				bool seeking = false;
				bool seekingArmed = false;

				bool appearSeeking = false;
				void update_appear_seeking();
			};

		};
	};
};