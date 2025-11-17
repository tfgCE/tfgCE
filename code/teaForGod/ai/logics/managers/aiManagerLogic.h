#pragma once

#include "..\..\..\..\framework\ai\aiLogicData.h"
#include "..\..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace Framework
{
	class Region;
};

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			namespace Managers
			{
				/**
				 */
				class AIManager
				: public ::Framework::AI::LogicWithLatentTask
				{
					FAST_CAST_DECLARE(AIManager);
					FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
					FAST_CAST_END();

					typedef ::Framework::AI::LogicWithLatentTask base;
				public:
					AIManager(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData, LATENT_FUNCTION_VARIABLE(_executeFunction));
					virtual ~AIManager();

				public:
					void register_ai_manager_in(Framework::Room* r);
					void register_ai_manager_in(Framework::Region* r);
					void register_ai_manager_in_top_region(Framework::Region* r);
					void unregister_ai_manager();

				private:
					SafePtr<Framework::Region> registeredInRegion;
				};
			};
		};
	};
};