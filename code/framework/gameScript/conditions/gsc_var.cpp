#include "gsc_var.h"

#include "..\registeredGameScriptConditions.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\modulesOwner\modulesOwner.h"

//

using namespace Framework;
using namespace GameScript;
using namespace Conditions;

//

bool Var::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	var = _node->get_name_attribute(TXT("var"), var);

	error_loading_xml_on_assert(var.is_valid(), result, _node, TXT("no \"var\" provided"));

	return result;
}

bool Var::check(ScriptExecution const & _execution) const
{
	bool result = false;

	auto* vi = _execution.get_variables().find_any_existing(var);

	if (vi)
	{
		if (vi->type_id() == type_id<bool>())
		{
			result = vi->get<bool>();
		}
		else if (vi->type_id() == type_id<float>())
		{
			result = vi->get<float>() != 0.0f;
		}
		else if (vi->type_id() == type_id<SafePtr<Framework::IModulesOwner>>())
		{
			result = vi->get<SafePtr<Framework::IModulesOwner>>().get() != nullptr;
		}
		else
		{
			error(TXT("no support for type \"%S\""), RegisteredType::get_name_of(vi->type_id()));
		}
	}
	else
	{
		error(TXT("no var \"%S\""), var.to_char());
	}

	return result;
}
