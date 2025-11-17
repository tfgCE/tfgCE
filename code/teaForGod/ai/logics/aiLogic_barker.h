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

	namespace CustomModules
	{
		class Pickup;
		class SwitchSidesHandler;
	};

	namespace AI
	{
		namespace Logics
		{
			class BarkerData;

			class Barker
			: public NPCBase
			{
				FAST_CAST_DECLARE(Barker);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Barker(_mind, _logicData); }

			public:
				Barker(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Barker();

			private:
				static LATENT_FUNCTION(execute_logic);
				static LATENT_FUNCTION(attack_enemy);

			private:
				BarkerData const * data = nullptr;

				CustomModules::Pickup* asPickup = nullptr;

				Energy dischargeDamage;
			};

			class BarkerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(BarkerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new BarkerData(); }

				BarkerData();
				virtual ~BarkerData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class Barker;
			};
	
		};
	};
};