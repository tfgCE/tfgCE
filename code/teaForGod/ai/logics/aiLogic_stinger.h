#pragma once

#include "aiLogic_npcBase.h"

#include "..\aiVarState.h"

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\appearance\socketID.h"

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
			class StingerData;

			class Stinger
			: public NPCBase
			{
				FAST_CAST_DECLARE(Stinger);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Stinger(_mind, _logicData); }

			public:
				Stinger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Stinger();

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(behave);
				static LATENT_FUNCTION(manage_mouth);
				static LATENT_FUNCTION(attack_enemy);
				static LATENT_FUNCTION(be_around_enemy);
				static LATENT_FUNCTION(fly_freely);
				static LATENT_FUNCTION(go_investigate);

			private:
				StingerData const * data = nullptr;

				CustomModules::Pickup* asPickup = nullptr;

				Framework::SocketID spitEnergySocket;
				Range spitEnergyInterval;
				EnergyRange spitEnergyAmount;
				EnergyCoef spitEnergyAmountMul;
				Range spitEnergyDischargeInterval;

				Energy dischargeDamage;

				bool beingHeld = false;
				bool isAttacking = false;
				float beingThrownTimeLeft = 0.0f;

				VarState mouthOpen;
			};

			class StingerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(StingerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new StingerData(); }

				StingerData();
				virtual ~StingerData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class Stinger;
	
				Framework::MeshGenParam<Name> mouthOpenVar;
				
				Framework::MeshGenParam<Name> spitEnergySocket;
				Framework::MeshGenParam<Range> spitEnergyInterval = Range(0.5f);
				Framework::MeshGenParam<EnergyRange> spitEnergyAmount = EnergyRange(Energy(10));
				Framework::MeshGenParam<EnergyCoef> spitEnergyAmountMul = EnergyCoef(1); // multiplied by this is what user receives
				Framework::MeshGenParam<Range> spitEnergyDischargeInterval = Range(2.0, 10.0f);

			};
		
		};
	};
};