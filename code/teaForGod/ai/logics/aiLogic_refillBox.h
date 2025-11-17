#pragma once

#include "components\aiLogicComponent_energyTransferPhysSens.h"

#include "aiLogic_energyKiosk.h"

#include "..\..\game\energy.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	class TexturePart;
	class ObjectType;
	interface_class IModulesOwner;
	struct DelayedObjectCreation;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			class RefillBoxData;

			/**
			 */
			class RefillBox
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(RefillBox);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new RefillBox(_mind, _logicData); }

			public:
				RefillBox(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~RefillBox();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				RefillBoxData const * refillBoxData = nullptr;

				Random::Generator rg;

				bool depleted = false;

				float timeToCheckPlayer = 0.0f;
				bool playerIsIn = false;

				SimpleVariableInfo openVar;
				SimpleVariableInfo coilOutVar;
				SimpleVariableInfo openActualVar;
				SimpleVariableInfo coilOutActualVar;

				//

				Energy energy = Energy(0);
				Energy maxEnergy = Energy(100);

				::Framework::SocketID baySocket;
				float bayRadius = 0.1f;

				struct EnergyHandling
				{
					float timeToNextTransfer = 0.0f;
					Energy transferSpeed = Energy(10);
					::Framework::RelativeToPresencePlacement objectInBay;
					bool transferring = false;

				} energyHandling;

				EnergyTransferPhysSens energyTransferPhysSens;
				Array<SafePtr<Framework::IModulesOwner>> energyTransferTemporaryObjects;

				::Framework::Display* display = nullptr;
				bool updateDisplay = false;
				int displayMoveLines = 0;
				float timeToUpdateDisplayLeft = 0.0f;

				float machineIsOn = 1.0f;

				EnergyInStorageDisplayData displayData;

				void update_object_in_coil(::Framework::IModulesOwner* imo);
				void transfer_energy(::Framework::IModulesOwner* imo, float _deltaTime, bool _allowTransfer);

				Optional<int> determine_priority(::Framework::IModulesOwner* _object);
			};

			class RefillBoxData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(RefillBoxData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new RefillBoxData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class RefillBox;
			};
		};
	};
};