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
	namespace AI
	{
		namespace Logics
		{
			class EnergyCoilData;

			/**
			 */
			class EnergyCoil
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(EnergyCoil);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new EnergyCoil(_mind, _logicData); }

			public:
				EnergyCoil(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~EnergyCoil();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				EnergyCoilData const * energyCoilData = nullptr;

				::Framework::Display* display = nullptr;

				Energy energy = Energy::zero();
				Energy maxStorage = Energy::zero();

				bool markedAsKnownForOpenWorldDirection = false;

				bool redrawNow = false;
				float timeToRedraw = 0.0f;

				::Framework::SocketID baySocket;
				float bayRadius = 0.1f;
				bool bayOccupiedEmissive = false;

				float displayBottomAtPt = 0.0f;
				float displayBottomDir = 1.0f;

				struct EnergyHandling
				{
					EnergyType::Type energyType = EnergyType::None;
					float timeToNextTransfer = 0.0f;
					Energy transferSpeed = Energy(10);
					::Framework::RelativeToPresencePlacement objectInBay;
				};
				EnergyHandling bay;

				enum EnergyTransfer
				{
					None,
					Withdraw,
				};
				EnergyTransfer energyTransfer = EnergyTransfer::None;
				EnergyTransferPhysSens energyTransferPhysSens;
				Array<SafePtr<Framework::IModulesOwner>> energyTransferTemporaryObjects;

				Optional<int> determine_priority(::Framework::IModulesOwner* _object);
					
				void update_bay(::Framework::IModulesOwner* imo, bool _force = false);
				void update_object_in_bay(::Framework::IModulesOwner* imo);
				void handle_transfer_energy(float _deltaTime, ::Framework::IModulesOwner* imo);
				void handle_energy_transfer_temporary_objects(EnergyTransfer _prevEnergyTransfer, EnergyTransfer _energyTransfer, ::Framework::IModulesOwner* imo, bool _force);

				Name get_energy_transfer_temporary_object_name(EnergyTransfer _energyTransfer);
			};

			class EnergyCoilData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(EnergyCoilData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new EnergyCoilData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				override_ bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class EnergyCoil;

				Colour displayAmmoColour = Colour::white;
				Colour displayHealthColour = Colour::white;
			};
		};
	};
};