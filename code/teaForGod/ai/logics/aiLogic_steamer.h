#pragma once

#include "..\..\teaEnums.h"
#include "..\..\game\energy.h"

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Steamer
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Steamer);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Steamer(_mind, _logicData); }

			public:
				Steamer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Steamer();

			public: // Logic
				override_ void learn_from(SimpleVariableStorage& _parameters);
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

				Array<SafePtr<Framework::IModulesOwner>> steamObjects;

				Range steamInterval = Range(2.0f, 60.0f);
				Range steamLength = Range(1.0f, 10.0f);

				void start_steam();
				void stop_steam();
			};

		};
	};
};