#include "agn_prepareVariables.h"

#include "..\animationGraphNodes.h"
#include "..\animationSet.h"

#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleController.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

DEFINE_STATIC_NAME_STR(animation_averageRootMotionSpeed, TXT("averageRootMotionSpeed"));
DEFINE_STATIC_NAME_STR(controller_requestedMovementDirectionYaw, TXT("requestedMovementDirectionYaw"));
DEFINE_STATIC_NAME_STR(presence_currentSpeedXY, TXT("currentSpeedXY"));
DEFINE_STATIC_NAME_STR(presence_currentSpeedZ, TXT("currentSpeedZ"));
DEFINE_STATIC_NAME_STR(presence_velocityRotationYaw, TXT("velocityRotationYaw"));

//

REGISTER_FOR_FAST_CAST(PrepareVariables);

IAnimationGraphNode* PrepareVariables::create_node()
{
	return new PrepareVariables();
}

bool PrepareVariables::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	// clear
	//
	variables.clear();

	// read
	//
	for_every(varNode, _node->children_named(TXT("variable")))
	{
		variables.push_back();
		auto& v = variables.get_last();
		{
			v.varID = varNode->get_name_attribute(TXT("id"));
			error_loading_xml_on_assert(v.varID.is_valid(), result, varNode, TXT("no \"id\" provided"));
			for_every(opNode, varNode->all_children())
			{
				if (!v.operations.has_place_left())
				{
					error(TXT("not enough place for operations, reached limit"));
					result = false;
					continue;
				}
				v.operations.push_back();
				auto &op = v.operations.get_last();
				op.type = Operation::Unknown;
				if (opNode->get_name() == TXT("animation"))
				{
					op.type = Operation::Animation;
					op.data.animation.animation = opNode->get_name_attribute(TXT("animation"));
					op.data.animation.what = opNode->get_name_attribute(TXT("what"));
					op.data.animation.resultReg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.animation.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.animation.resultReg);
				}
				else if (opNode->get_name() == TXT("controller"))
				{
					op.type = Operation::Controller;
					op.data.controller.what = opNode->get_name_attribute(TXT("what"));
					op.data.controller.resultReg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.controller.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.controller.resultReg);
				}
				else if (opNode->get_name() == TXT("add") ||
						 opNode->get_name() == TXT("div") ||
						 opNode->get_name() == TXT("mul") ||
						 opNode->get_name() == TXT("sub"))
				{
					if (opNode->get_name() == TXT("add")) op.type = Operation::Add;
					if (opNode->get_name() == TXT("div")) op.type = Operation::Div;
					if (opNode->get_name() == TXT("mul")) op.type = Operation::Mul;
					if (opNode->get_name() == TXT("sub")) op.type = Operation::Sub;
					op.data.mathOp.reg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.mathOp.byReg = opNode->get_int_attribute(TXT("byReg"), NONE);
					op.data.mathOp.byReg = opNode->get_int_attribute(TXT("otherReg"), op.data.mathOp.byReg);
					op.data.mathOp.byValue = opNode->get_float_attribute(TXT("byValue"), 0.0f);
					op.data.mathOp.byValue = opNode->get_float_attribute(TXT("value"), op.data.mathOp.byValue);
					op.data.mathOp.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.mathOp.reg);
				}
				else if (opNode->get_name() == TXT("limit"))
				{
					op.type = Operation::Limit;
					op.data.limit.range = Range::empty;
					op.data.limit.range.load_from_attribute_or_from_child(opNode, TXT("range"));
					op.data.limit.reg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.limit.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.limit.reg);
				}
				else if (opNode->get_name() == TXT("presence"))
				{
					op.type = Operation::Presence;
					op.data.presence.what = opNode->get_name_attribute(TXT("what"));
					op.data.presence.resultReg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.presence.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.presence.resultReg);
				}
				else if (opNode->get_name() == TXT("result"))
				{
					op.type = Operation::Result;
					op.data.result.reg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.result.onlyHigher = opNode->get_bool_attribute(TXT("onlyHigher"), false);
				}
				else if (opNode->get_name() == TXT("value"))
				{
					op.type = Operation::Value;
					op.data.value.resultReg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.value.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.value.resultReg);
					op.data.value.value = opNode->get_float_attribute(TXT("value"), 0.0f);
				}
				else if (opNode->get_name() == TXT("variable"))
				{
					op.type = Operation::Variable;
					op.data.variable.varID = opNode->get_name_attribute(TXT("varID"));
					op.data.variable.resultReg = opNode->get_int_attribute(TXT("reg"), 0);
					op.data.variable.resultReg = opNode->get_int_attribute(TXT("resultReg"), op.data.variable.resultReg);
				}
			}
		}
	}

	return result;
}

bool PrepareVariables::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(PrepareVariables);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(bool, justActivated, false);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE_NO_DEFAULT(Registers, registers);

	for_every(variableInfo, variables)
	{
		ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(SimpleVariableInfo, variable, SimpleVariableInfo());
		for_every(op, variableInfo->operations)
		{
			switch (op->type)
			{
			case Operation::Animation:
				{
					ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(AnimationLookUp, animation, AnimationLookUp());
				} break;
			case Operation::Variable:
				{
					ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(SimpleVariableInfo, variableVar, SimpleVariableInfo());
				} break;
			default: break;
			}
		}
	}

	return result;
}

void PrepareVariables::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(PrepareVariables);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(bool, justActivated);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(Registers, registers);

	for_every(variableInfo, variables)
	{
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, variable);
		variable.set_name(variableInfo->varID);
		variable.look_up<float>(_runtime.imoOwner->access_variables());
		for_every(op, variableInfo->operations)
		{
			switch (op->type)
			{
			case Operation::Animation:
			{
				ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animation);
				animation.look_up(_runtime.owner, op->data.animation.animation);
			} break;
			case Operation::Variable:
			{
				ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, variableVar);
				variableVar.set_name(op->data.variable.varID);
				variableVar.look_up<float>(_runtime.imoOwner->access_variables());
			} break;
			default: break;
			}
		}
	}
}

void PrepareVariables::on_activate(AnimationGraphRuntime& _runtime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(PrepareVariables);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(bool, justActivated);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(Registers, registers);

	base::on_activate(_runtime);

	justActivated = true;
}

void PrepareVariables::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(PrepareVariables);

	base::on_deactivate(_runtime);
}

#define REG(_operation, _reg) float& _reg = registers.reg[op->data._operation._reg];

void PrepareVariables::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(PrepareVariables);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(bool, justActivated);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(Registers, registers);

	for_every(variableInfo, variables)
	{
		ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, variable);
		float & varValue = variable.access<float>();
		for_every(op, variableInfo->operations)
		{
			switch (op->type)
			{
			case Operation::Animation:
			{
				ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(AnimationLookUp, animation);
				REG(animation, resultReg);
				if (op->data.animation.what == NAME(animation_averageRootMotionSpeed) &&
					animation.get())
				{
					resultReg = animation.get()->get_average_root_motion_speed();
				}
				else
				{
					error(TXT("failed PrepareVariables::Operation::Animation"));
				}
			} break;
			case Operation::Controller:
			{
				REG(controller, resultReg);
				auto* controller = _runtime.imoOwner->get_controller();
				if (op->data.controller.what == NAME(controller_requestedMovementDirectionYaw))
				{
					float movDirYaw = 0.0f;
					auto movDir = controller->get_relative_requested_movement_direction();
					if (movDir.is_set())
					{
						movDirYaw = Rotator3::get_yaw(movDir.get());
					}
					resultReg = movDirYaw;
				}
				else
				{
					error(TXT("failed PrepareVariables::Operation::Controller"));
				}
			} break;
			case Operation::Limit:
			{
				REG(limit, reg);
				REG(limit, resultReg);
				resultReg = op->data.limit.range.clamp(reg);
			} break;
			// math op
				case Operation::Add:
				{
					REG(mathOp, reg);
					REG(mathOp, resultReg);
					if (op->data.mathOp.byReg == NONE)
					{
						resultReg = reg + op->data.mathOp.byValue;
					}
					else
					{
						REG(mathOp, byReg);
						resultReg = reg + byReg;
					}
				} break;
				case Operation::Div:
				{
					REG(mathOp, reg);
					REG(mathOp, resultReg);
					if (op->data.mathOp.byReg == NONE)
					{
						resultReg = reg / op->data.mathOp.byValue;
					}
					else
					{
						REG(mathOp, byReg);
						resultReg = reg / byReg;
					}
				} break;
				case Operation::Mul:
				{
					REG(mathOp, reg);
					REG(mathOp, resultReg);
					if (op->data.mathOp.byReg == NONE)
					{
						resultReg = reg * op->data.mathOp.byValue;
					}
					else
					{
						REG(mathOp, byReg);
						resultReg = reg * byReg;
					}
				} break;
				case Operation::Sub:
				{
					REG(mathOp, reg);
					REG(mathOp, resultReg);
					if (op->data.mathOp.byReg == NONE)
					{
						resultReg = reg - op->data.mathOp.byValue;
					}
					else
					{
						REG(mathOp, byReg);
						resultReg = reg - byReg;
					}
				} break;
			case Operation::Presence:
			{
				REG(presence, resultReg);
				auto* presence = _runtime.imoOwner->get_presence();
				if (op->data.presence.what == NAME(presence_currentSpeedXY))
				{
					float speedXY = presence->get_velocity_linear().drop_using(presence->get_placement().get_axis(Axis::Up)).length();
					resultReg = speedXY;
				}
				else if (op->data.presence.what == NAME(presence_currentSpeedZ))
				{
					Vector3 up = presence->get_placement().get_axis(Axis::Up).normal();
					float speedZ = Vector3::dot(up, presence->get_velocity_linear().along(up));
					resultReg = speedZ;
				}
				else if (op->data.presence.what == NAME(presence_velocityRotationYaw))
				{
					float velRotYaw = presence->get_velocity_rotation().yaw;
					resultReg = velRotYaw;
				}
				else
				{
					error(TXT("failed PrepareVariables::Operation::Presence"));
				}
			} break;
			case Operation::Result:
			{
				REG(result, reg);
				if (justActivated)
				{
					varValue = reg;
				}
				else
				{
					if (op->data.result.onlyHigher)
					{
						reg = max(varValue, reg);
					}
					varValue = blend_to_using_time(varValue, reg, variableInfo->blendTime, _context.deltaTime);
				}
			} break;
			case Operation::Value:
			{
				REG(value, resultReg);
				resultReg = op->data.value.value;
			} break;
			case Operation::Variable:
			{
				ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, variableVar);
				REG(variable, resultReg);
				resultReg = variableVar.get<float>();
			} break;
			default:
				error(TXT("failed PrepareVariables::Operation::Unknown or not implemented"));
				break;
			}
		}
	}

	// post preparation
	base::execute(_runtime, _context, _output);

	justActivated = false;
}
