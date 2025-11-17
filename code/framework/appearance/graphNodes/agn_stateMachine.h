#pragma once

#include "..\animationGraphNode.h"
#include "..\..\..\core\other\factoryOfNamed.h"

//

namespace Framework
{
	namespace AnimationGraphNodesLib
	{
		interface_class IStateMachineDecision;
		class StateMachine;
		struct StateMachineState;

		interface_class IStateMachineDecision
		: public RefCountObject
		{
			FAST_CAST_DECLARE(IStateMachineDecision);
			FAST_CAST_END();

		public:
			IStateMachineDecision() {}
			virtual~IStateMachineDecision() {}

		public:
			virtual bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc) = 0;
			virtual bool prepare_runtime(AnimationGraphRuntime& _runtime) = 0;
			virtual bool execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const = 0; // returns true if processed and should stop further execution

		public:
			static bool load_decisions_from_xml(REF_ Array<RefCountObjectPtr<IStateMachineDecision>>& _decisions, IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			static bool prepare_decisions_runtime(REF_ Array<RefCountObjectPtr<IStateMachineDecision>>& _decisions, AnimationGraphRuntime& _runtime);
			static bool execute_decisions(Array<RefCountObjectPtr<IStateMachineDecision>> const & _decisions, StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime);
		};

		/*
		 * if a decision should lead to further decisions, it can derive from this, if the decision is fulfilled, it should call execute from here
		 */
		interface_class IStateMachineDecisionWithSubDecisions
		: public IStateMachineDecision
		{
			FAST_CAST_DECLARE(IStateMachineDecisionWithSubDecisions);
			FAST_CAST_BASE(IStateMachineDecision);
			FAST_CAST_END();

			typedef IStateMachineDecision base;
		public:
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext & _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime & _runtime);
			implement_ bool execute(StateMachine const * _stateMachine, AnimationGraphRuntime & _runtime) const;

		private:
			Array<RefCountObjectPtr<IStateMachineDecision>> decisions; // decisions to perform if true
		};

		struct StateMachineState
		{			
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(StateMachineState);
			Name id;
			AnimationGraphNodeSet nodeSet;
			AnimationGraphNodeInput inputNode;
			Array<RefCountObjectPtr<IStateMachineDecision>> exitDecision;
			Array<Name> exitDecisionFromStates; // reused, not copied... yet

			bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			bool prepare_runtime(StateMachine* forStateMachine, AnimationGraphRuntime& _runtime);
			void prepare_instance(AnimationGraphRuntime& _runtime) const;
			void on_activate(AnimationGraphRuntime& _runtime) const;
			void on_reactivate(AnimationGraphRuntime& _runtime) const;
			void on_deactivate(AnimationGraphRuntime& _runtime) const;
			void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

			float get_time_active(AnimationGraphRuntime& _runtime) const;
		};

		class StateMachine
		: public IAnimationGraphNode
		{
			FAST_CAST_DECLARE(StateMachine);
			FAST_CAST_BASE(IAnimationGraphNode);
			FAST_CAST_END();

			typedef IAnimationGraphNode base;

		public:
			static IAnimationGraphNode* create_node();

			StateMachine() {}
			virtual ~StateMachine() {}

		public:
			Name const & get_current_state(AnimationGraphRuntime& _runtime) const;
			float get_current_state_time_active(AnimationGraphRuntime& _runtime) const;
			bool change_state(Name const& _id, float _blendTime, bool _forceUseFrozenPose, AnimationGraphRuntime& _runtime) const; // returns true if switched, if the same returns false

		public: // IAnimationGraphNode
			implement_ bool load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc);
			implement_ bool prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet);
			implement_ void prepare_instance(AnimationGraphRuntime& _runtime) const;
			implement_ void on_activate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_reactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void on_deactivate(AnimationGraphRuntime& _runtime) const;
			implement_ void execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const;

		private:
			ANIMATION_GRAPH_RUNTIME_VARIABLES_USER(StateMachine);

			Array<StateMachineState> states;

			bool allowReactivatingStates = true; // if the states can't be reactivated, frozen pose will be used when entering the state that hasn't blend out fully
			bool rootMotionOnlyFromFirstState = false; // if true, won't mix root motion, will use full from main

			Array<RefCountObjectPtr<IStateMachineDecision>> startDecision;
			Array<RefCountObjectPtr<IStateMachineDecision>> globalDecision; // used AFTER start if no state activated and BEFORE states

			friend struct StateMachineState;
		};

		class StateMachineDecisions
		: private FactoryOfNamed<IStateMachineDecision, Name>
		{
			typedef FactoryOfNamed<IStateMachineDecision, Name> base;
		public:
			static void initialise_static();
			static void close_static();

			static void register_state_machine_decision(Name const & _name, Create _create) { base::add(_name, _create); }
			static IStateMachineDecision* create_decision(Name const & _name) { return base::create(_name); }
		};
	};
};
