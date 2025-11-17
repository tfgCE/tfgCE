#pragma once

#include "..\..\core\math\blendUtils.h"
#include "..\..\core\other\simpleVariableStorage.h"

#include "..\..\framework\meshGen\meshGenParam.h"
#include "..\..\framework\modulesOwner\modulesOwner.h"

namespace TeaForGodEmperor
{
	namespace AI
	{
		struct VarState
		{
			SimpleVariableInfo variable;
			float state = 0.0f;
			float target = 0.0f;
			float blendTime = 0.0f;

			void blend_to(float _target, float _blendTime) { target = _target; blendTime = _blendTime; }
			void update(float _deltaTime,
				std::function<float(float const & _in, float const & _target, float _blendTime, float const _deltaTime)> _blend = blend_to_using_time<float>,
				std::function<float(float _in)> _transform = nullptr);

			template <typename Class>
			void set_variable(Framework::IModulesOwner* imo, Framework::MeshGenParam<Name> const & _varName)
			{
				variable.set_name(_varName.get(imo->access_variables()));
				variable.look_up<Class>(imo->access_variables());
			}
		};

	};
};