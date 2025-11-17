#pragma once

#include "..\agn_stateMachine.h"

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		namespace StateMachineDecisionsLib
		{
			class EnterState
			: public IStateMachineDecision
			{
				FAST_CAST_DECLARE(EnterState);
				FAST_CAST_BASE(IStateMachineDecision);
				FAST_CAST_END();

				typedef IStateMachineDecision base;
			public:
				static IStateMachineDecision* create_decision();

				EnterState() {}
				virtual ~EnterState() {}

				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime);
				implement_ bool execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const;

			private:
				Name id;
				float blendTime = 0.0f;
				bool forceUseFrozenPose = false;
			};

		};
	};
};

