#pragma once

#include "..\agn_stateMachine.h"

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		namespace StateMachineDecisionsLib
		{
			class Presence
			: public IStateMachineDecisionWithSubDecisions
			{
				FAST_CAST_DECLARE(Presence);
				FAST_CAST_BASE(IStateMachineDecisionWithSubDecisions);
				FAST_CAST_END();

				typedef IStateMachineDecisionWithSubDecisions base;
			public:
				static IStateMachineDecision* create_decision();

				Presence() {}
				virtual ~Presence() {}

				implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
				implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime);
				implement_ bool execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const;

			private:
				Optional<Range> currentSpeedXY;
				Optional<Range> currentSpeedZ;
			};

		};
	};
};

