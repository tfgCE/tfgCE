#pragma once

#include "..\..\game\energy.h"

#include "..\..\..\core\tags\tag.h"

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModuleBox;

	namespace AI
	{
		namespace Logics
		{
			class Discharger
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(Discharger);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new Discharger(_mind, _logicData); }

			public:
				Discharger(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~Discharger();

			public: // Logic
				override_ void learn_from(SimpleVariableStorage& _parameters);
				override_ void advance(float _deltaTime);

			private:
				static LATENT_FUNCTION(execute_logic);

				Range useInterval = Range(2.0f);
				Range intervalOnDischarge = Range(0.25f);
				float maxDist = 3.0f;

				Energy damage = Energy(1);

				bool discharge();
			};

		};
	};
};