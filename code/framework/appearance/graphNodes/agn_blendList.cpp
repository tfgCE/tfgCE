#include "agn_blendList.h"

#include "..\animationGraphNodes.h"
#include "..\animationSet.h"

#include "..\..\module\moduleAppearance.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace AnimationGraphNodesLib;

//

REGISTER_FOR_FAST_CAST(BlendList);

IAnimationGraphNode* BlendList::create_node()
{
	return new BlendList();
}

bool BlendList::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	XML_LOAD_NAME_ATTR(_node, blendVarID);
	XML_LOAD_BOOL_ATTR(_node, circular);
	XML_LOAD_FLOAT_ATTR(_node, blendTime);
	XML_LOAD_BOOL_ATTR(_node, blendLineary);

	for_every(node, _node->children_named(TXT("input")))
	{
		inputs.push_back();
		auto& i = inputs.get_last();
		i.load_from_xml(node, TXT("nodeId"));
	}

	return result;
}

bool BlendList::prepare_runtime(AnimationGraphRuntime& _runtime, AnimationGraphNodeSet& _inSet)
{
	bool result = base::prepare_runtime(_runtime, _inSet);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_DECLARING(BlendList);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, currBlend, 0.0f);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE_NO_DEFAULT(SimpleVariableInfo, blendVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(int, activeInputHi, NONE);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(int, activeInputLo, NONE);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, atInputHi, 0.0f);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_DECLARE(float, atInputLo, 0.0f);

	for_every(input, inputs)
	{
		result &= input->prepare_runtime(_inSet);
	}

	return result;
}

void BlendList::prepare_instance(AnimationGraphRuntime& _runtime) const
{
	base::prepare_instance(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(BlendList);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, currBlend);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, blendVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(int, activeInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(int, activeInputLo);

	blendVar.set_name(blendVarID);
	blendVar.look_up<float>(_runtime.imoOwner->access_variables());
}

void BlendList::on_activate(AnimationGraphRuntime& _runtime) const
{
	base::on_activate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(BlendList);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, currBlend);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(SimpleVariableInfo, blendVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputLo);

	activeInputHi = NONE;
	activeInputLo = NONE;
	update_curr_blend(_runtime, true, 0.0f);
}

void BlendList::on_reactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_reactivate(_runtime);

	//ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(BlendList);
}

void BlendList::on_deactivate(AnimationGraphRuntime& _runtime) const
{
	base::on_deactivate(_runtime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(BlendList);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, currBlend);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(SimpleVariableInfo, blendVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputLo);

	update_inputs(_runtime, REF_ activeInputHi, REF_ activeInputLo, NONE, NONE);
}

void BlendList::execute(AnimationGraphRuntime& _runtime, AnimationGraphExecuteContext& _context, OUT_ AnimationGraphOutput& _output) const
{
	base::execute(_runtime, _context, _output);

	update_curr_blend(_runtime, false, _context.deltaTime);

	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(BlendList);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, currBlend);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(SimpleVariableInfo, blendVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputLo);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_NOT_USED(float, atInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atInputLo);

	if (auto* node = inputs[activeInputHi].get_node())
	{
		node->execute(_runtime, _context, _output);
	}
	if (atInputLo >= 0.0f && activeInputLo != NONE)
	{
		if (auto* node = inputs[activeInputLo].get_node())
		{
			AnimationGraphOutput workingOutput;
			workingOutput.use_own_pose(_output.access_pose()->get_skeleton());
			node->execute(_runtime, _context, workingOutput);

			_output.blend_in(atInputLo, workingOutput);
		}
	}
}

void BlendList::update_curr_blend(AnimationGraphRuntime& _runtime, bool _justActivated, float _deltaTime) const
{
	ANIMATION_GRAPH_RUNTIME_VARIABLES_START_USING(BlendList);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, currBlend);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(SimpleVariableInfo, blendVar);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(int, activeInputLo);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atInputHi);
	ANIMATION_GRAPH_RUNTIME_VARIABLE_USE(float, atInputLo);

	float maxInput = (float)(inputs.get_size() + (circular ? 0 : -1));
	float targetBlend = blendVar.is_valid() ? blendVar.get<float>() : 0.0f;
	if (_justActivated)
	{
		currBlend = targetBlend;
	}
	else
	{
		if (circular)
		{
			float whole = maxInput;
			float half = whole * 0.5f;
			while (targetBlend > currBlend + half) targetBlend -= whole;
			while (targetBlend < currBlend - half) targetBlend += whole;
		}
		if (blendLineary)
		{
			currBlend = blend_to_using_speed_based_on_time(currBlend, targetBlend, blendTime, _deltaTime);
		}
		else
		{
			currBlend = blend_to_using_time(currBlend, targetBlend, blendTime, _deltaTime);
		}
	}

	if (circular)
	{
		currBlend = mod(currBlend, maxInput);
	}
	else
	{
		currBlend = clamp(currBlend, 0.0f, maxInput);
	}

	int inputHi = TypeConversions::Normal::f_i_closest(currBlend);
	int inputLo = currBlend < (float)inputHi ? inputHi - 1 : inputHi + 1;
	float atLo = abs(currBlend - (float)inputHi);
	if (atLo < 0.0001f)
	{
		atLo = 0.0f;
	}
	float atHi = 1.0f - atLo;

	inputHi = mod(inputHi, inputs.get_size());
	inputLo = mod(inputLo, inputs.get_size());

	if (atHi == 1.0f)
	{
		inputLo = NONE;
	}

	update_inputs(_runtime, REF_ activeInputHi, REF_ activeInputLo, inputHi, inputLo);

	atInputHi = atHi;
	atInputLo = atLo;
}

void BlendList::update_inputs(AnimationGraphRuntime& _runtime, int& _activeInputHi, int& _activeInputLo, int _inputHi, int _inputLo) const
{
	if (_activeInputHi != NONE &&
		_activeInputHi != _inputHi &&
		_activeInputHi != _inputLo)
	{
		if (auto* node = inputs[_activeInputHi].get_node())
		{
			node->on_deactivate(_runtime);
		}
		_activeInputHi = NONE;
	}

	if (_activeInputLo != NONE &&
		_activeInputLo != _inputHi &&
		_activeInputLo != _inputLo)
	{
		if (auto* node = inputs[_activeInputLo].get_node())
		{
			node->on_deactivate(_runtime);
		}
		_activeInputLo = NONE;
	}

	if (_inputHi != NONE &&
		_inputHi != _activeInputHi &&
		_inputHi != _activeInputLo)
	{
		if (auto* node = inputs[_inputHi].get_node())
		{
			node->on_activate(_runtime);
		}
	}

	if (_inputLo != NONE &&
		_inputLo != _activeInputHi &&
		_inputLo != _activeInputLo)
	{
		if (auto* node = inputs[_inputLo].get_node())
		{
			node->on_activate(_runtime);
		}
	}

	_activeInputHi = _inputHi;
	_activeInputLo = _inputLo;
}
