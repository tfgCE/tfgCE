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
		class InteractiveButtonHandler;
		class EnergyStorage;
	};

	namespace AI
	{
		namespace Logics
		{
			class Battery
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Battery);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Battery(_mind, _logicData); }

			public:
				Battery(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Battery();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(act_normal);

				CustomModules::Pickup* asPickup = nullptr;
				CustomModules::InteractiveButtonHandler* asInteractiveButtonHandler = nullptr;
				CustomModules::EnergyStorage* asEnergyStorage = nullptr;

				SafePtr<Framework::IModulesOwner> transferringEnergyTo;
			};

		};
	};
};