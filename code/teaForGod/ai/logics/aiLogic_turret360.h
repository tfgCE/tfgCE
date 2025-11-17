#pragma once

#include "aiLogic_npcBase.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Turret360Data;

			class Turret360
			: public NPCBase
			{
				FAST_CAST_DECLARE(Turret360);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Turret360(_mind, _logicData); }

			public:
				Turret360(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Turret360();

			public: // Logic
				override_ void advance(float _deltaTime);

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage& _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);

				Turret360Data const* turret360Data = nullptr;

				SimpleVariableInfo extendedVar;

				float extended = 0.0f;
				float extendTarget = 0.0f;

				Array<Framework::SocketID> muzzleSockets;

				int shootCount = 0; // set with shootingBurst
			};

			class Turret360Data
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(Turret360Data);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new Turret360Data(); }

				Turret360Data();
				virtual ~Turret360Data();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				Name muzzleSocketPrefix;
				Name projectileTOID;
				Name muzzleFlashTOID;
				Name shootSoundID;

				Range idleInterval = Range(5.0f, 10.0f);
				Range alertedInterval = Range(2.0f, 4.0f);
				
				float preFireWait = 1.0f;
				float postFireWait = 0.0f;

				int shootingBurst = 1;
				float shootingInterval = 0.1f;

				Name extendedVarID;

				friend class Turret360;
			};


		};
	};
};