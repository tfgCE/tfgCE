#pragma once

#include "components\aiLogicComponent_energyTransferPhysSens.h"

#include "..\..\teaEnums.h"
#include "..\..\game\energy.h"
#include "..\..\utils\interactiveSwitchHandler.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"

namespace Framework
{
	class Display;
	class DisplayText;
	class TexturePart;
	interface_class IModulesOwner;
};

namespace TeaForGodEmperor
{
	class ModulePilgrim;

	namespace AI
	{
		namespace Logics
		{
			class EnergyKioskData;

			struct EnergyInStorageDisplayData
			{
				Array<int> used;
				Array<int> notUsed;

				void update(int _maxAmount, float _fullPt, int _minUsed, int _maxMoveLines = 0);
			};

			/**
			 */
			class EnergyKiosk
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EnergyKiosk);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EnergyKiosk(_mind, _logicData); }

			public:
				EnergyKiosk(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EnergyKiosk();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				EnergyKioskData const * energyKioskData = nullptr;
				::Framework::Display* display = nullptr;
				bool updateDisplay = false;
				int displayMoveLines = 0;
				float timeToUpdateDisplayLeft = 0.0f;

				bool markedAsKnownForOpenWorldDirection = false;

				float machineIsOnTarget = 1.0f;
				float machineIsOn = 1.0f;

				EnergyInStorageDisplayData displayData;

				::Framework::SocketID baySocket;
				float bayRadius = 0.1f;
				int bayOccupiedEmissive = 0;

				Energy energy = Energy(0);
				Energy maxEnergy = Energy(0); // if 0, unlimited

				int const DEPOSIT_IN_KIOSK = 1;
				int const DEPOSIT_IN_OBJECT = -1;

				struct EnergyHandling
				{
					EnergyType::Type energyType = EnergyType::None;
					InteractiveSwitchHandler switchHandler;
					bool requiresNeutral = false;
					bool goodToTransfer = false;
					float timeToNextTransfer = 0.0f;
					Energy transferSpeed = Energy(10);
					::Framework::RelativeToPresencePlacement objectInBay;
					bool objectInBayWasSet = false;
					bool indicateOn = false;
					bool indicateWasOn = false;
					bool requiresNeutralIndicateOn = false;
					bool requiresNeutralIndicateWasOn = false;
				};
				EnergyHandling left;
				EnergyHandling right;
				EnergyHandling* activeTransfer = nullptr;

				Optional<Energy> showingInBayEnergy;
				Optional<Energy> showingKioskEnergy;
				ModulePilgrim* showingForPilgrim = nullptr;

				// deposit in kiosk, withdraw from kiosk
				enum EnergyTransfer
				{
					None,
					LeftDeposit,
					LeftWithdraw,
					RightDeposit,
					RightWithdraw,
				};
				EnergyTransfer energyTransfer = EnergyTransfer::None;
				EnergyTransferPhysSens energyTransferPhysSens;
				Array<SafePtr<Framework::IModulesOwner>> energyTransferTemporaryObjects;

				Optional<int> determine_priority(EnergyHandling * _energyHandling, ::Framework::IModulesOwner* _object); // if not set, ignore

				void update_bay(::Framework::IModulesOwner* imo, bool _force = false);
				void update_indicators(EnergyHandling * _energyHandling, ::Framework::IModulesOwner* imo, bool _force = false);
				void update_object_in_bay(EnergyHandling * _energyHandling, ::Framework::IModulesOwner* imo);
				void handle_switch_and_transfer_energy(EnergyHandling * _energyHandling, float _deltaTime, ::Framework::IModulesOwner* imo);
				void handle_energy_transfer_temporary_objects(EnergyTransfer _prevEnergyTransfer, EnergyTransfer _energyTransfer, ::Framework::IModulesOwner* imo);
				static Name get_energy_transfer_temporary_object_name(EnergyTransfer _energyTransfer);
				Energy get_available_energy_for(EnergyHandling& _energyHandling);
			};

			class EnergyKioskData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(EnergyKioskData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new EnergyKioskData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				struct Element
				{
					Framework::UsedLibraryStored<Framework::TexturePart> texturePart;
					VectorInt2 at = VectorInt2::zero;
					Colour ink = Colour::white;
					Colour inkInactive = Colour::none;
					bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
					bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);
				};
				EnergyType::Type leftEnergyType = EnergyType::None;
				EnergyType::Type rightEnergyType = EnergyType::None;
				Element right;
				Element left;
				Colour displayInactiveColour = Colour::grey;
				Colour displayActiveColour = Colour::yellow;
				Colour displayWrongItemColour = Colour::red;

				friend class EnergyKiosk;
			};
		};
	};
};