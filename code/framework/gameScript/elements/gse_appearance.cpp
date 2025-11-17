#include "gse_appearance.h"

#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleAppearanceImpl.inl"

#include "..\..\..\core\io\xml.h"
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

// variable
DEFINE_STATIC_NAME(self);
DEFINE_STATIC_NAME(currentTime);

//

bool Appearance::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	mayFail = _node->get_bool_attribute_or_from_child_presence(TXT("mayFail"), mayFail);

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	toAppearance = _node->get_name_attribute(TXT("toAppearance"), toAppearance);

	setVisible.load_from_xml(_node, TXT("setVisible"));
	setMain.load_from_xml(_node, TXT("setMain"));

	blendTime = _node->get_float_attribute(TXT("blendTime"), blendTime);
	blendCurve = _node->get_name_attribute(TXT("blendCurve"), blendCurve);

	setShaderParams0.load_from_xml_child_node(_node, TXT("setShaderParams"));
	setShaderParams0.load_from_xml_child_node(_node, TXT("setShaderParams0"));
	setShaderParams1.load_from_xml_child_node(_node, TXT("setShaderParams"));
	setShaderParams1.load_from_xml_child_node(_node, TXT("setShaderParams1"));

	return result;
}

bool Appearance::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type Appearance::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
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

	if (objectVar.is_valid())
	{
		if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
		{
			if (auto* imo = exPtr->get())
			{
				MODULE_OWNER_LOCK_FOR_IMO(imo, TXT("gse appearance"));
				if (toAppearance.is_valid())
				{
					if (auto* a = imo->get_appearance_named(toAppearance))
					{
						act_on(a, blendPt);
					}
					else if (!mayFail)
					{
						warn_dev_investigate(TXT("missing appearance for gse appearance"));
					}
				}
				else
				{
					for_every_ptr(a, imo->get_appearances())
					{
						act_on(a, blendPt);
					}
				}
			}
			else if (!mayFail)
			{
				warn_dev_investigate(TXT("missing object for gse appearance"));
			}
		}
		else if (!mayFail)
		{
			warn_dev_investigate(TXT("missing var for gse appearance"));
		}
	}

	return blendPt >= 1.0f? ScriptExecutionResult::Continue : ScriptExecutionResult::Repeat;
}

void Appearance::act_on(Framework::ModuleAppearance* _a, float _blendPt) const
{
	if (setVisible.is_set())
	{
		_a->be_visible(setVisible.get());
	}
	if (setMain.is_set() && setMain.get())
	{
		_a->be_main_appearance();
	}
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
				_a->set_shader_param(p->get_name(), found? lerp(_blendPt, found->get<float>(), p->get<float>()) : p->get<float>());
			}
			else if (p->type_id() == type_id<Vector2>())
			{
				_a->set_shader_param(p->get_name(), found ? lerp(_blendPt, found->get<Vector2>(), p->get<Vector2>()) : p->get<Vector2>());
			}
			else if (p->type_id() == type_id<Vector3>())
			{
				_a->set_shader_param(p->get_name(), found ? lerp(_blendPt, found->get<Vector3>(), p->get<Vector3>()) : p->get<Vector3>());
			}
			else if (p->type_id() == type_id<Vector4>())
			{
				_a->set_shader_param(p->get_name(), found ? lerp(_blendPt, found->get<Vector4>(), p->get<Vector4>()) : p->get<Vector4>());
			}
			else if (p->type_id() == type_id<Colour>())
			{
				_a->set_shader_param(p->get_name(), (found ? lerp(_blendPt, found->get<Colour>(), p->get<Colour>()) : p->get<Colour>()).to_vector4());
			}
			else
			{
				error(TXT("type \"%S\" not supported by setShaderParams for appearance GSE"), RegisteredType::get_name_of(p->type_id()));
			}
		}
	}
}
