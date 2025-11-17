#pragma once

#include "aiLogic_npcBase.h"

#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\meshGen\meshGenParam.h"
#include "..\..\..\framework\presence\presencePath.h"

namespace Framework
{
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
			class PesterData;

			class Pester
			: public NPCBase
			{
				FAST_CAST_DECLARE(Pester);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Pester(_mind, _logicData); }

			public:
				Pester(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Pester();

			public: // Logic
				implement_ void learn_from(SimpleVariableStorage& _parameters);
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(act_normal);
				static LATENT_FUNCTION(attack_enemy);

			private:
				PesterData const* data = nullptr;

				Energy dischargeDamage;
			};

			class PesterData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(PesterData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new PesterData(); }

				PesterData();
				virtual ~PesterData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class Pester;
			};
	
		};
	};
};