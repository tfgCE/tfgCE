#include "agn_stateMachine.h"

#include "..\animationGraphNodes.h"
#include "..\..\..\core\mesh\pose.h"

//

#include "stateMachineDecisions/agnsmd_controller.h"
#include "stateMachineDecisions/agnsmd_currentState.h"
#include "stateMachineDecisions/agnsmd_enterState.h"
#include "stateMachineDecisions/agnsmd_movement.h"
#include "stateMachineDecisions/agnsmd_presence.h"
#include "stateMachineDecisions/agnsmd_syncPlaybackPt.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define MAX_ACTIVE_STATES 10
#define MAX_BONES_FOR_FROZEN_POSE 100

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

void StateMachineDecisions::initialise_static()
{
	base::initialise_static();

	// lib
	StateMachineDecisions::register_state_machine_decision(Name(TXT("controller")), StateMachineDecisionsLib::Controller::create_decision);
	StateMachineDecisions::register_state_machine_decision(Name(TXT("currentState")), StateMachineDecisionsLib::CurrentState::create_decision);
	StateMachineDecisions::register_state_machine_decision(Name(TXT("enterState")), StateMachineDecisionsLib::EnterState::create_decision);
	StateMachineDecisions::register_state_machine_decision(Name(TXT("movement")), StateMachineDecisionsLib::Movement::create_decision);
	StateMachineDecisions::register_state_machine_decision(Name(TXT("presence")), StateMachineDecisionsLib::Presence::create_decision);
	StateMachineDecisions::register_state_machine_decision(Name(TXT("syncPlaybackPt")), StateMachineDecisionsLib::SyncPlaybackPt::create_decision);
}

void StateMachineDecisions::close_static()
{
	base::close_static();
}

//--

struct ActiveStatesInfo
{
	struct StateInfo
	{
		StateMachineState const * state = nullptr;
		float active = 0.0f;
		float activeEffective = 0.0f; // used when switching order
		// all states are blended in by default, we calculate active weight from what is left from previous states
		float blendInTimeLeft = 0.0f;
	};
	int count = 0;
	StateInfo states[MAX_ACTIVE_STATES];

	void reset()
	{
		count = 0;
	}

	bool is_any_active() const { return count > 0; }

	void update_active_effective()
	{
		float activeLeft = 1.0f;
		auto* state = states;
		for (int stateIdx = 0; stateIdx < count; ++stateIdx, ++state)
		{
			state->activeEffective = activeLeft * state->active;
			activeLeft = max(0.0f, activeLeft - state->activeEffective);
			an_assert(activeLeft >= 0.0f);
		}
	}
	
	void update_active_from_active_effective()
	{
		float activeLeft = 1.0f;
		auto* state = states;
		for (int stateIdx = 0; stateIdx < count; ++stateIdx, ++state)
		{
			if (activeLeft <= 0.0f)
			{
				state->active = 0.0f;
			}
			else
			{
				state->active = clamp(state->activeEffective / activeLeft, 0.0f, 1.0f);
				activeLeft = max(0.0f, activeLeft - state->activeEffective);
			}
		}
	}

	void deactivate_if_needed(AnimationGraphRuntime& _runtime)
	{
		float activeLeft = 1.0f;
		int newCount = 0;
		// find new active count
		{
			auto* state = states;
			for (int stateIdx = 0; stateIdx < count; ++stateIdx, ++state)
			{
				if (activeLeft <= 0.0f)
				{
					break;
				}
				else
				{
					state->activeEffective = activeLeft * state->active;
					activeLeft = max(0.0f, activeLeft - state->activeEffective);
					++newCount;
				}
			}
		}
		// deactivate all that are no longer active
		if (newCount < count)
		{
			auto* state = &states[newCount];
			for (int stateIdx = newCount; stateIdx < count; ++stateIdx, ++state)
			{
				state->state->on_deactivate(_runtime);
			}
		}
		count = newCount;
	}
};

struct FrozenPose
{
	bool isValid = false;
	Transform bones[MAX_BONES_FOR_FROZEN_POSE];

	int max_data_size() const { return sizeof(Transform) * MAX_BONES_FOR_FROZEN_POSE; }

	void reset()
	{
		isValid = false;
	}
};

//--

REGISTER_FOR_FAST_CAST(StateMachine);

IAnimationGraphNode* StateMachine::create_node()
{
	return new StateMachine();
}

bool StateMachine::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// reset/clear
	//
	allowReactivatingStates = true;
	rootMotionOnlyFromFirstState = false;
	startDecision.clear();
	globalDecision.clear();
	states.clear();

	// load
	//
	XML_LOAD_BOOL_ATTR(_node, allowReactivatingStates);
	XML_LOAD_BOOL_ATTR(_node, rootMotionOnlyFromFirstState);

	for_every(node, _node->children_named(TXT("startDecision")))
	{
		result &= IStateMachineDecision::load_decisions_from_xml(REF_ startDecision, node, _lc);
	}

	for_every(node, _node->children_named(TXT("globalDecision")))
	{
		result &= IStateMachineDecision::load_decisions_from_xml(REF_ globalDecision, node, _lc);
	}

	for_every(node, _node->children_named(TXT("state")))
	{
		states.push_back();
		auto& s = states.get_last();
		if (s.load_from_xml(node, _lc))
		{
			// ok
		}
		else
		{
			error_loading_xml(node, TXT("could not load state"));
			result = false;
			states.pop_back();
		}
	}

	return result;
}

bool StateMachine::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(ActiveStatesInfo, activeStates, ActiveStatesInfo());
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(FrozenPose, frozenPose, FrozenPose());
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(FrozenPose, frozenPoseAtAnyTime, FrozenPose());

	result &= IStateMachineDecision::prepare_decisions_runtime(REF_ startDecision, _runtime);
	result &= IStateMachineDecision::prepare_decisions_runtime(REF_ globalDecision, _runtime);
	
	for_every(state, states)
	{
		result &= state->prepare_runtime(this, _runtime);
	}

	return result;
}

void StateMachine::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPose);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPoseAtAnyTime);

	activeStates.reset();
	frozenPose.reset();
	frozenPoseAtAnyTime.reset();

	for_every(state, states)
	{
		state->prepare_instance(_runtime);
	}
}

void StateMachine::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPose);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPoseAtAnyTime);

	activeStates.reset();
	frozenPose.reset();
	frozenPoseAtAnyTime.reset();

	IStateMachineDecision::execute_decisions(startDecision, this, _runtime);
	if (!activeStates.is_any_active())
	{
		IStateMachineDecision::execute_decisions(globalDecision, this, _runtime);
	}
}

void StateMachine::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_reactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);

	IStateMachineDecision::execute_decisions(startDecision, this, _runtime);
}

void StateMachine::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(FrozenPose, frozenPose);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(FrozenPose, frozenPoseAtAnyTime);

	auto* state = activeStates.states;
	for(int stateIdx = 0; stateIdx < activeStates.count; ++ stateIdx, ++state)
	{
		state->state->on_deactivate(_runtime);
	}
	activeStates.count = 0;
}

void StateMachine::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);
	
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPose);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPoseAtAnyTime);

	AnimationGraphExecuteContext localContext = _context;
	an_assert(! localContext.deltaTimeUnused.is_set());

	// check if maybe something left as unused delta time, if so, do the whole process again, we may want to blend into something else
	while (localContext.deltaTime > 0.0f)
	{
		// do global decisions first
		IStateMachineDecision::execute_decisions(globalDecision, this, _runtime);

		if (activeStates.count > 0)
		{
			IStateMachineDecision::execute_decisions(activeStates.states->state->exitDecision, this, _runtime);
		}
		else
		{
			IStateMachineDecision::execute_decisions(startDecision, this, _runtime);
		}

		// advance active
		{
			float activeLeft = 1.0f;
			auto* state = activeStates.states;
			for (int stateIdx = 0; stateIdx < activeStates.count; ++stateIdx, ++state)
			{
				if (state->blendInTimeLeft > 0.0f)
				{
					float pt = clamp(localContext.deltaTime / state->blendInTimeLeft, 0.0f, 1.0f);
					state->active = lerp(pt, state->active, 1.0f);
					state->blendInTimeLeft = max(0.0f, state->blendInTimeLeft - localContext.deltaTime);
				}
				else
				{
					state->active = 1.0f;
					state->blendInTimeLeft = 0.0f;
				}
				state->activeEffective = activeLeft * state->active;
				activeLeft = max(0.0f, activeLeft - state->activeEffective);
			}
		}

		activeStates.deactivate_if_needed(_runtime);

		// execute all left
		if (activeStates.count)
		{
			AnimationGraphOutput workingOutput;

			float activeEffectiveUsed = 0.0f;
			auto* state = activeStates.states;
			for (int stateIdx = 0; stateIdx < activeStates.count; ++stateIdx, ++state)
			{
				if (stateIdx == 0)
				{
					workingOutput.use_pose(_output.access_pose());
				}
				else if (stateIdx == 1)
				{
					workingOutput.use_own_pose(_output.access_pose()->get_skeleton());
				}

				activeEffectiveUsed += state->activeEffective;
				float blendInWeight = state->activeEffective > 0.0f? state->activeEffective / activeEffectiveUsed : 0.0f;

				workingOutput.access_root_motion() = Transform::identity;
				state->state->execute(_runtime, localContext, workingOutput);

				// for first one the pose is the same and we can leave it as it is
				if (_output.access_pose() != workingOutput.access_pose())
				{
					_output.blend_in(blendInWeight, workingOutput, !rootMotionOnlyFromFirstState);
				}
			}

			// mix in frozen pose if required, store only if invalid
			if (activeEffectiveUsed < 1.0f && frozenPose.isValid)
			{
				float blendInFrozenPoseWeight = 1.0f - activeEffectiveUsed;
				auto & destBones = _output.access_pose()->access_bones();
				auto* destBone = destBones.begin();
				auto* blendInBone = frozenPose.bones;
				for_count(int, boneIdx, min(MAX_BONES_FOR_FROZEN_POSE, destBones.get_size()))
				{
					*destBone = lerp(blendInFrozenPoseWeight, *destBone, *blendInBone);
					++destBone;
					++blendInBone;
				}
			}
			
			{
				auto const& srcBones = _output.access_pose()->access_bones();
				memory_copy(frozenPoseAtAnyTime.bones, srcBones.get_data(), min(frozenPoseAtAnyTime.max_data_size(), srcBones.get_data_size()));
				frozenPoseAtAnyTime.isValid = true;
			}
		}

		localContext.deltaTime = localContext.deltaTimeUnused.get(0.0f);
		localContext.deltaTimeUnused.clear();
	}
}

Name const& StateMachine::get_current_state(AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);

	if (activeStates.count > 0)
	{
		if (auto* state = activeStates.states[0].state)
		{
			return state->id;
		}
	}

	return Name::invalid();
}

float StateMachine::get_current_state_time_active(AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);

	if (activeStates.count > 0)
	{
		if (auto* state = activeStates.states[0].state)
		{
			return state->get_time_active(_runtime);
		}
	}

	return 0.0f;
}

bool StateMachine::change_state(Name const& _id, float _blendTime, bool _forceUseFrozenPose, AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachine);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(ActiveStatesInfo, activeStates);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPose);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(FrozenPose, frozenPoseAtAnyTime);

	if (activeStates.count > 0 &&
		activeStates.states[0].state &&
		activeStates.states[0].state->id == _id)
	{
		// already there
		return false;
	}

	if (activeStates.count == 0)
	{
		// no blend for the first state to become active
		_blendTime = 0.0f;
	}

	// calculate effective active weight
	activeStates.update_active_effective();

	// check if the state to start is already on the list of active states
	{
		auto* state = activeStates.states;
		for (int stateIdx = 0; stateIdx < activeStates.count; ++stateIdx, ++state)
		{
			if (state->state->id == _id)
			{
				if (allowReactivatingStates)
				{
					// move to front
					ActiveStatesInfo::StateInfo toActivate = *state;
					while (stateIdx > 0)
					{
						auto* dest = state;
						--state;
						*dest = *state;
						--stateIdx;
					}
					toActivate.blendInTimeLeft = _blendTime;
					*activeStates.states = toActivate;
					break;
				}
				else
				{
					// force all to become deactivated
					auto* state = activeStates.states;
					for (int stateIdx = 0; stateIdx < activeStates.count; ++stateIdx, ++state)
					{
						state->state->on_deactivate(_runtime);
					}
					// frozen pose will be used
					activeStates.count = 0;
					frozenPose = frozenPoseAtAnyTime;
				}
				break;
			}
		}
	}

	// if nothing, add something
	if (activeStates.count == 0)
	{
		++activeStates.count;
		activeStates.states->state = nullptr;
	}

	an_assert(activeStates.count > 0);
	
	bool newState = false;

	// now check if we have to put something in front
	if (activeStates.states->state &&
		activeStates.states->state->id != _id)
	{
		if (activeStates.count >= MAX_ACTIVE_STATES)
		{
			// will be removed because there is no place, force deactivate 
			activeStates.states[activeStates.count - 1].state->on_deactivate(_runtime);
		}

		// make place for a new one
		activeStates.count = min(activeStates.count + 1, MAX_ACTIVE_STATES);
		auto* state = &activeStates.states[activeStates.count - 1];
		for (int idx = activeStates.count - 1; idx > 0; --idx)
		{
			auto* dest = state;
			--state;
			*dest = *state;
		}
		activeStates.states->state = nullptr;
		activeStates.states->activeEffective = 0.0f;
	}

	// check if new and find the right state
	if (!activeStates.states->state)
	{
		for_every(state, states)
		{
			if (state->id == _id)
			{
				activeStates.states->state = state;
			}
		}
		newState = true;
		if (!activeStates.states->state)
		{
			error(TXT("no state found!"));
			activeStates.count = 0;
			newState = false;
		}
	}

	// fill blend time
	activeStates.states->blendInTimeLeft = _blendTime;

	// update active from active effective
	activeStates.update_active_from_active_effective();

	// check if something should be deactivated
	activeStates.deactivate_if_needed(_runtime);

	// call "on_activate"
	if (newState)
	{
		activeStates.states->state->on_activate(_runtime);
	}
	else
	{
		activeStates.states->state->on_reactivate(_runtime);
	}

	// switched
	return true;
}

//--

REGISTER_FOR_FAST_CAST(IStateMachineDecision);

bool IStateMachineDecision::load_decisions_from_xml(REF_ Array<RefCountObjectPtr<IStateMachineDecision>>& _decisions, IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	for_every(node, _node->all_children())
	{
		Name type = Name(node->get_name());
		if (auto* decision = StateMachineDecisions::create_decision(type))
		{
			_decisions.push_back();
			_decisions.get_last() = decision;
			if (decision->load_from_xml(node, _lc))
			{
				// ok
			}
			else
			{
				_decisions.pop_back();
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("couldn't recognise state machine decision type \"%S\""), type.to_char());
			result = false;
		}
	}

	return result;
}

bool IStateMachineDecision::prepare_decisions_runtime(REF_ Array<RefCountObjectPtr<IStateMachineDecision>>& _decisions, AnimationGraphRuntime& _runtime)
{
	bool result = true;

	for_every_ref(decision, _decisions)
	{
		result &= decision->prepare_runtime(_runtime);
	}

	return result;
}

bool IStateMachineDecision::execute_decisions(Array<RefCountObjectPtr<IStateMachineDecision>> const& _decisions, StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime)
{
	for_every_ref(decision, _decisions)
	{
		if (decision->execute(_stateMachine, _runtime))
		{
			return true;
		}
	}

	return false;
}

//--

REGISTER_FOR_FAST_CAST(IStateMachineDecisionWithSubDecisions);

bool IStateMachineDecisionWithSubDecisions::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	result &= IStateMachineDecision::load_decisions_from_xml(REF_ decisions, _node, _lc);

	return result;
}

bool IStateMachineDecisionWithSubDecisions::prepare_runtime(AnimationGraphRuntime& _runtime)
{
	bool result = true;

	result &= IStateMachineDecision::prepare_decisions_runtime(REF_ decisions, _runtime);

	return result;
}

bool IStateMachineDecisionWithSubDecisions::execute(StateMachine const * _stateMachine, AnimationGraphRuntime& _runtime) const
{
	for_every_ref(decision, decisions)
	{
		if (decision->execute(_stateMachine, _runtime))
		{
			return true;
		}
	}

	return false;
}

//--

bool StateMachineState::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	XML_LOAD_NAME_ATTR(_node, id);

	for_every(node, _node->children_named(TXT("content")))
	{
		result &= nodeSet.load_from_xml(node, _lc);
		result &= inputNode.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("exitDecisionUseFromState")))
	{
		if (auto* attr = node->get_attribute(TXT("id")))
		{
			exitDecisionFromStates.push_back_unique(attr->get_as_name());
		}
	}
	for_every(node, _node->children_named(TXT("exitDecision")))
	{
		result &= IStateMachineDecision::load_decisions_from_xml(REF_ exitDecision, node, _lc);
	}

	return result;
}

bool StateMachineState::prepare_runtime(StateMachine* forStateMachine, AnimationGraphRuntime& _runtime)
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(StateMachineState);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, timeActive, 0.0f);

	bool result = true;

	result &= nodeSet.prepare_runtime(_runtime);
	result &= inputNode.prepare_runtime(nodeSet);
	// while we're still reusing "fromStates", do not call on all yet
	result &= IStateMachineDecision::prepare_decisions_runtime(REF_ exitDecision, _runtime);

	for_every(stateId, exitDecisionFromStates)
	{
		bool found = false;
		for_every(state, forStateMachine->states)
		{
			if (state->id == *stateId)
			{
				todo_note(TXT("maybe we should copy them instead of reusing?"));
				// add afterwards as we want main exits to be more important
				exitDecision.add_from(state->exitDecision);
				found = true;
			}
		}
		if (!found)
		{
			error(TXT("state \"%S\" not found"), stateId->to_char());
		}
	}

	return result;
}

void StateMachineState::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	nodeSet.prepare_instance(_runtime);
}

void StateMachineState::on_activate(AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachineState);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, timeActive);

	timeActive = 0.0f;

	if (auto* node = inputNode.get_node())
	{
		node->on_activate(_runtime);
	}
}

void StateMachineState::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachineState);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, timeActive);

	timeActive = 0.0f;

	if (auto* node = inputNode.get_node())
	{
		node->on_reactivate(_runtime);
	}
}

void StateMachineState::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	if (auto* node = inputNode.get_node())
	{
		node->on_deactivate(_runtime);
	}
}

void StateMachineState::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachineState);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, timeActive);

	timeActive += _context.deltaTime;

	if (auto* node = inputNode.get_node())
	{
		node->execute(_runtime, _context, OUT_ _output);
	}
}

float StateMachineState::get_time_active(AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(StateMachineState);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, timeActive);

	return timeActive;
}

