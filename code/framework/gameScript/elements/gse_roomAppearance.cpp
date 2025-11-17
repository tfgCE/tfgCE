#include "gse_roomAppearance.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\core\system\core.h"

#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(self);
DEFINE_STATIC_NAME(currentTime);

//

bool RoomAppearance::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	roomVar = _node->get_name_attribute(TXT("roomVar"), roomVar);

	blendTime = _node->get_float_attribute(TXT("blendTime"), blendTime);
	blendCurve = _node->get_name_attribute(TXT("blendCurve"), blendCurve);

	setShaderParams0.load_from_xml_child_node(_node, TXT("setShaderParams"));
	setShaderParams0.load_from_xml_child_node(_node, TXT("setShaderParams0"));
	setShaderParams1.load_from_xml_child_node(_node, TXT("setShaderParams"));
	setShaderParams1.load_from_xml_child_node(_node, TXT("setShaderParams1"));

	return result;
}

bool RoomAppearance::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type RoomAppearance::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	float& currentTimeVar = _execution.access_variables().access<float>(NAME(currentTime));

	if (is_flag_set(_flags, ScriptExecution::Entered))
	{
		currentTimeVar = 0.0f;
	}
	else
	{
		float deltaTime = ::System::Core::get_delta_time();
		currentTimeVar += deltaTime;
	}

	float blendPt = blendTime > 0.0f ? clamp(currentTimeVar / blendTime, 0.0f, 1.0f) : 1.0f;
	blendPt = BlendCurve::apply(blendCurve, blendPt);

	if (roomVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(roomVar))
		{
			if (auto* room = exPtr->get())
			{
				act_on(room, blendPt);
			}
		}
	}

	return blendPt >= 1.0f? ScriptExecutionResult::Continue : ScriptExecutionResult::Repeat;
}

template<typename Class>
void set_shader_param(Framework::Room* _room, Name const& _paramName, Class const& _value)
{
	auto& mset = _room->access_appearance_for_rendering();

	for_every_ptr(mi, mset.get_instances())
	{
		for_count(int, i, mi->get_material_instance_count())
		{
			if (auto* mati = mi->access_material_instance(i))
			{
				mati->set_uniform(_paramName, _value);
			}
		}
	}
}

void RoomAppearance::act_on(Framework::Room* _room, float _blendPt) const
{
	{
		for_every(p, setShaderParams1.get_all())
		{
			SimpleVariableInfo const * found = nullptr;
			if (_blendPt < 1.0f)
			{
				for_every(ps, setShaderParams0.get_all())
				{
					if (p->type_id() == ps->type_id() &&
						p->get_name() == ps->get_name())
					{
						found = ps;
						break;
					}
				}
			}
			if (p->type_id() == type_id<float>())
			{
				set_shader_param(_room, p->get_name(), found? lerp(_blendPt, found->get<float>(), p->get<float>()) : p->get<float>());
			}
			else if (p->type_id() == type_id<Vector2>())
			{
				set_shader_param(_room, p->get_name(), found ? lerp(_blendPt, found->get<Vector2>(), p->get<Vector2>()) : p->get<Vector2>());
			}
			else if (p->type_id() == type_id<Vector3>())
			{
				set_shader_param(_room, p->get_name(), found ? lerp(_blendPt, found->get<Vector3>(), p->get<Vector3>()) : p->get<Vector3>());
			}
			else if (p->type_id() == type_id<Vector4>())
			{
				set_shader_param(_room, p->get_name(), found ? lerp(_blendPt, found->get<Vector4>(), p->get<Vector4>()) : p->get<Vector4>());
			}
			else if (p->type_id() == type_id<Colour>())
			{
				set_shader_param(_room, p->get_name(), (found ? lerp(_blendPt, found->get<Colour>(), p->get<Colour>()) : p->get<Colour>()).to_vector4());
			}
			else
			{
				error(TXT("type \"%S\" not supported by setShaderParams for appearance GSE"), RegisteredType::get_name_of(p->type_id()));
			}
		}
	}
}
