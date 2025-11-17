#pragma once

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"


namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			/**
			 */
			class SolarPanel
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(SolarPanel);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new SolarPanel(_mind, _logicData); }

			public:
				SolarPanel(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~SolarPanel();

			private:
				static LATENT_FUNCTION(execute_logic);
			};
		};
	};
};