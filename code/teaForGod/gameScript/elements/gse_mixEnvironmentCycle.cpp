#include "gse_mixEnvironmentCycle.h"

#include "..\..\pilgrimage\pilgrimageInstance.h"

#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\framework\world\room.h"

#include "..\..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(value);
DEFINE_STATIC_NAME(speed);

//

bool MixEnvironmentCycle::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);

	to = _node->get_float_attribute(TXT("to"), to);
	blendTime = _node->get_float_attribute(TXT("blendTime"), blendTime);
	smoothBlendTime = _node->get_float_attribute(TXT("smoothBlendTime"), smoothBlendTime);

	roomVar = _node->get_name_attribute_or_from_child(TXT("roomVar"), roomVar);
	setRoomMaterialParam = _node->get_name_attribute(TXT("setRoomMaterialParam"), setRoomMaterialParam);

	curve = _node->get_name_attribute(TXT("curve"), curve);

	return result;
}

bool MixEnvironmentCycle::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type MixEnvironmentCycle::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	float& value = _execution.access_variables().access<float>(NAME(value));
	float& speed = _execution.access_variables().access<float>(NAME(speed));

	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		if (auto* pi = PilgrimageInstance::get())
		{
			value = pi->get_mix_environment_cycle_var(name);
		}
		speed = abs(blendTime != 0.0f ? (to - value) / blendTime : 0.0f);
	}

	float deltaTime = TutorialSystem::check_active() ? ::System::Core::get_raw_delta_time() : ::System::Core::get_delta_time();
	if (smoothBlendTime != 0.0f)
	{
		value = blend_to_using_time(value, to, smoothBlendTime, deltaTime);
	}
	else
	{
		if (blendTime != 0.0f)
		{
			value = blend_to_using_speed(value, to, speed, deltaTime);
		}
		else
		{
			value = to;
		}
	}

	float outputValue = value;

	if (curve.is_valid())
	{
		float f = floor(outputValue);
		float v = outputValue - f;
		v = BlendCurve::apply(curve, v);
		outputValue = f + v;
	}

	if (auto* pi = PilgrimageInstance::get())
	{
		pi->set_mix_environment_cycle_var(name, outputValue);
	}

	if (roomVar.is_valid() && setRoomMaterialParam.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
		{
			if (auto* room = exPtr->get())
			{
				auto& roomAppearance = room->access_appearance_for_rendering();
				for_every_ptr(instance, roomAppearance.get_instances())
				{
					for_count(int, i, instance->get_material_instance_count())
					{
						if (auto* mat = instance->access_material_instance(i))
						{
							mat->set_uniform(setRoomMaterialParam, outputValue);
						}
					}
				}
			}
		}
	}

	return abs(value - to) < 0.0001f? Framework::GameScript::ScriptExecutionResult::Continue : Framework::GameScript::ScriptExecutionResult::Repeat;
}
