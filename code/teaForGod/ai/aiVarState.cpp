#include "aiVarState.h"

using namespace TeaForGodEmperor;
using namespace AI;

void VarState::update(float _deltaTime, std::function<float(float const & _in, float const & _target, float _blendTime, float const _deltaTime)> _blend, std::function<float(float _in)> _transform)
{
	state = _blend ? _blend(state, target, blendTime, _deltaTime) : blend_to_using_speed_based_on_time(state, target, blendTime, _deltaTime);

	if (variable.is_valid())
	{
		variable.access<float>() = _transform ? _transform(state) : state;
	}
}

