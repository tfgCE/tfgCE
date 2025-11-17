#pragma once

#include "..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class NoneData;

			/**
			 */
			class None
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(None);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new None(_mind, _logicData); }

			public:
				None(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~None();

			private:
				NoneData const* noneData = nullptr;

				static LATENT_FUNCTION(execute_logic);
			};
	
			class NoneData
			: public ::Framework::AI::LogicData
			{
				FAST_CAST_DECLARE(NoneData);
				FAST_CAST_BASE(::Framework::AI::LogicData);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicData base;
			public:
				static ::Framework::AI::LogicData* create_ai_logic_data() { return new NoneData(); }

			public: // LogicData
				override_ bool load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc);

			private:

				bool noRareWait = false;

				friend class None;
			};

		};
	};
};