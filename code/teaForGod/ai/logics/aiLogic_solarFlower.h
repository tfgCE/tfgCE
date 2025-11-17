#pragma once

#include "..\..\..\core\other\simpleVariableStorage.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Actor;
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
			/**
			 */
			class SolarFlower
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(SolarFlower);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new SolarFlower(_mind, _logicData); }

			public:
				SolarFlower(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~SolarFlower();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(update_power_source);

			private:
				bool isValid = false;

				float requestedActive = 0.0f;
				float powerActive = 0.0f;
				float angleFromVertical = 0.0f;
				Vector3 solarPanelTowards = Vector3(0.0f, 1000.0f, 0.0f);

				// variables
				Name solarFlowerActiveVarID;
				Name solarFlowerAngleFromVerticalVarID;
				Name solarPanelTowardsVarID;
				Name mapIconUnfoldedVarID;
				Name mapIconFoldedVarID;

				bool orderedActive = true;
				float sunAvailable = 1.0f;

				SimpleVariableInfo deviceTargetStateVar;
				SimpleVariableInfo deviceRequestedStateVar;
				SimpleVariableInfo deviceOrderedStateVar;

				SimpleVariableInfo solarFlowerActiveVar;
				SimpleVariableInfo solarFlowerAngleFromVerticalVar;
				SimpleVariableInfo solarPanelTowardsVar;
				
				SimpleVariableInfo mapIconVar;
				SimpleVariableInfo mapIconUnfoldedVar;
				SimpleVariableInfo mapIconFoldedVar;

			};

			class SolarFlowerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(SolarFlowerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new SolarFlowerData(); }

				SolarFlowerData();
				virtual ~SolarFlowerData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:

				friend class SolarFlower;
			};
		};
	};
};