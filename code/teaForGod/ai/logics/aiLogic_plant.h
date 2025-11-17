#pragma once

#include "..\aiVarState.h"

#include "..\..\modules\gameplay\moduleEquipment.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"

namespace Framework
{
	class DoorInRoom;
};

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class PlantData;

			/**
			 */
			class Plant
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Plant);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Plant(_mind, _logicData); }

			public:
				Plant(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Plant();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(being_held);
				static LATENT_FUNCTION(remain_in_pocket);
				static LATENT_FUNCTION(remain_attached);
				static LATENT_FUNCTION(try_to_attach);
				static LATENT_FUNCTION(open_and_close_hooks);

				PlantData const * data = nullptr;

				ModuleEquipment* asEquipment = nullptr;

				bool attachedToRoom = false;
				SafePtr<Collision::ICollidableObject> attachedToCollidableObject;
				SafePtr<Framework::DoorInRoom> attachedToDoorInRoom;
				float attachedToDoorInRoomOpenFactor = 0.0f; // to know if we should detach

				Framework::SocketID attachPointSocket;

				Framework::SocketID flyDirSocket;
				SimpleVariableInfo flyTowardsRelativeToPresenceVar;

				Framework::SocketID spitEnergySocket;
				Range spitEnergyInterval;
				EnergyRange spitEnergyAmount;
				EnergyCoef spitEnergyAmountMul;
				Range spitEnergyDischargeInterval;

				VarState hooksInGround;
				VarState hooksOpen;
				VarState flyTowardsActive;
				VarState draggingLegActive;
			};

			class PlantData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PlantData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PlantData(); }

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);

			private:
				Framework::MeshGenParam<Name> hooksInGroundVar;
				Framework::MeshGenParam<Name> hooksOpenVar;
				Framework::MeshGenParam<Name> attachPointSocket;
				Framework::MeshGenParam<Name> flyDirSocket;
				Framework::MeshGenParam<Name> flyTowardsActiveVar;
				Framework::MeshGenParam<Name> flyTowardsRelativeToPresenceVar;
				Framework::MeshGenParam<Name> draggingLegActiveVar;
				Framework::MeshGenParam<Name> spitEnergySocket;
				Framework::MeshGenParam<Range> spitEnergyInterval = Range(1.0f);
				Framework::MeshGenParam<EnergyRange> spitEnergyAmount = EnergyRange(Energy(10));
				Framework::MeshGenParam<EnergyCoef> spitEnergyAmountMul = EnergyCoef(1); // multiplied by this is what user receives
				Framework::MeshGenParam<Range> spitEnergyDischargeInterval = Range(1.0f, 10.0f);

				Collision::Flags attachToCollisionFlags = Collision::Flags::none(); // where should it attach
				Collision::Flags dontAttachToCollisionFlags = Collision::Flags::none(); // where should it attach
				Optional<Collision::Flags> beingHeldCollidesWithFlags; // when attached
				Optional<Collision::Flags> beingHeldCollisionFlags; // when attached
				Optional<Collision::Flags> inPocketCollidesWithFlags; // when attached
				Optional<Collision::Flags> inPocketCollisionFlags; // when attached

				Framework::AI::RegisteredLatentTasksInfo duringMain;
				Framework::AI::RegisteredLatentTasksInfo whileAttached;

				friend class Plant;
			};
		};
	};
};