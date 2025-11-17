#pragma once

#include "aiLogic_npcBase.h"

#include "..\..\..\framework\ai\aiLogicData.h"

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
			class RammerData;

			class Rammer
			: public NPCBase
			{
				FAST_CAST_DECLARE(Rammer);
				FAST_CAST_BASE(NPCBase);
				FAST_CAST_END();

				typedef NPCBase base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Rammer(_mind, _logicData); }

			public:
				Rammer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Rammer();

			public: // Framework::AI::LogicWithLatentTask
				override_ void learn_from(SimpleVariableStorage & _parameters);

			public: // Logic
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

			private:
				RammerData const * rammerData = nullptr;

				Array<SafePtr<Framework::IModulesOwner>> watchedDevices;
			};

			class RammerData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(RammerData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new RammerData(); }

				RammerData();
				virtual ~RammerData();

			public: // LogicData
				virtual bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);
				virtual bool prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext);

			private:
				friend class Rammer;
			};
		
		};
	};
};