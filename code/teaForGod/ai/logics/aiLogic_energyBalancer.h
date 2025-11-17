#pragma once

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Display;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class EnergyBalancerData;

			/**
			 */
			class EnergyBalancer
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EnergyBalancer);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EnergyBalancer(_mind, _logicData); }

			public:
				EnergyBalancer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EnergyBalancer();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				EnergyBalancerData const* energyBalancerData = nullptr;

				Random::Generator rg;

				struct Container
				{
					bool overheated = false;
					float overheatedDisplay = 0.0f;
					Energy prev = Energy::zero();
					Energy contains = Energy::zero();
					Energy capacity = Energy(100);
					Energy required = Energy(50);
					Energy maxSafe = Energy(90);

					float flowPt = 0.0f;
					float flowPtSpeed = 0.0f;

					SafePtr<Framework::IModulesOwner> button;
					SafePtr<Framework::IModulesOwner> slider;

					int sliderInteractiveDeviceID = NONE;

					void update_emissive(EnergyBalancer const * _eb);
				};
				friend struct Container;
				ArrayStatic<Container, 10> containers;

				float timeToOverheat = 7.0f;
				float timeToCooldown = 3.0f;

				::Framework::Display* display = nullptr;
				bool displaySetup = false;
				bool redrawNow = true;
				float redrawTime = 0.0f;

				float maxSafePt = 0.0f;
				float overheatedPt = 0.0f;

				bool done = false;

				// game script traps
				Name triggerGameScriptExecutionTrapOnDone;
				float repeatTriggerGameScriptExecutionTrapOnDone = 0.0f;

				void play_sound(Framework::IModulesOwner* imo, Name const& _sound);

				void force_balance();
			};

			class EnergyBalancerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(EnergyBalancerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new EnergyBalancerData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class EnergyBalancer;
			};
		};
	};
};