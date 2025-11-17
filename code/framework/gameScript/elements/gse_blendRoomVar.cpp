#include "gse_blendRoomVar.h"

#include "..\..\world\room.h"

#include "..\..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(blendTimeLeft);

//

bool BlendRoomVar::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	roomVar = _node->get_name_attribute(TXT("roomVar"), roomVar);
	floatVarName = _node->get_name_attribute(TXT("floatVarName"), floatVarName);
	floatTarget = _node->get_float_attribute(TXT("floatTarget"), floatTarget);
	
	blendTime = _node->get_float_attribute(TXT("blendTime"), blendTime);

	return result;
}

ScriptExecutionResult::Type BlendRoomVar::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
	{
		if (auto* room = exPtr->get())
		{
			float blendTimeLeft = 0.0f;
			float deltaTime = 0.0f;
			float floatValue = floatTarget;
			if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
			{
				blendTimeLeft = blendTime;
				if (floatVarName.is_valid())
				{
					floatValue = room->get_value<float>(floatVarName, floatTarget);
				}
			}
			else
			{
				blendTimeLeft = _execution.get_variables().get_value<float>(NAME(blendTimeLeft), 0.0f);
				deltaTime = ::System::Core::get_delta_time();
				if (floatVarName.is_valid())
				{
					// just from this room, we already set it, right?
					floatValue = room->get_variables().get_value<float>(floatVarName, floatTarget);
				}
			}
			
			if (blendTimeLeft <= deltaTime)
			{
				floatValue = floatTarget;
			}
			else
			{
				floatValue = blend_to_using_speed_based_on_time(floatValue, floatTarget, blendTimeLeft, deltaTime);
			}

			blendTimeLeft -= deltaTime;

			if (floatVarName.is_valid())
			{
				room->access_variables().access<float>(floatVarName) = floatValue;
			}

			_execution.access_variables().access<float>(NAME(blendTimeLeft)) = blendTimeLeft;

			return blendTimeLeft >= 0.0f ? ScriptExecutionResult::Repeat : ScriptExecutionResult::Continue;
		}
	}

	return ScriptExecutionResult::Continue;
}
