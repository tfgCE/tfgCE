#include "variablesStringFormatterParamsProvider.h"

#include "..\game\energy.h"
#include "..\..\core\other\simpleVariableStorage.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(VariablesStringFormatterParamsProvider);

bool VariablesStringFormatterParamsProvider::requested_string_formatter_param(REF_ Framework::StringFormatterParams& _params, Name const& _param) const
{
	void const* value;
	TypeId type;
	if (variables.get_existing_of_any_type_id(_param, type, value))
	{
		if (type == type_id<int>())
		{
			_params.add(_param, *((int*)value));
			return true;
		}
		else if (type == type_id<float>())
		{
			_params.add(_param, *((float*)value));
			return true;
		}
		else if (type == type_id<Energy>())
		{
			_params.add(_param, ((Energy*)value)->as_float());
			return true;
		}
		else
		{
			todo_implement(TXT("add missing type"));
		}
	}
	return false;
}
