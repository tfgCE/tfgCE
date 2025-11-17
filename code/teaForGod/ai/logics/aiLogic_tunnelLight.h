#pragma once

#include "..\..\..\framework\ai\aiLogicWithLatentTask.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		namespace Logics
		{
			/**
			 */
			class TunnelLight
			: public ::Framework::AI::LogicWithLatentTask
			{
				FAST_CAST_DECLARE(TunnelLight);
				FAST_CAST_BASE(::Framework::AI::LogicWithLatentTask);
				FAST_CAST_END();

				typedef ::Framework::AI::LogicWithLatentTask base;
			public:
				static ::Framework::AI::Logic* create_ai_logic(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData) { return new TunnelLight(_mind, _logicData); }

			public:
				TunnelLight(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData);
				virtual ~TunnelLight();

			public: // Logic
				implement_ void advance(float _deltaTime);

				implement_ void learn_from(SimpleVariableStorage & _parameters);

			private:
				static LATENT_FUNCTION(execute_logic);
				
			private:
				Range switchCheckInterval = Range(0.0f);
				Vector3 switchOnAt = Vector3(0.0f, 0.0f, 10.0f);
				Vector3 switchOffAt = Vector3(0.0f, 0.0f, -10.0f);
				Name switchEmissive;
				Name switchLight;
				Name lightSocket;
				Vector3 lightLocOS = Vector3::zero;

				bool lightOn = false;
			};
		};
	};
};