#pragma once

#include "..\..\framework\text\stringFormatter.h"

//

class SimpleVariableStorage;

//

namespace TeaForGodEmperor
{
	struct VariablesStringFormatterParamsProvider
	: public Framework::IStringFormatterParamsProvider
	{
		FAST_CAST_DECLARE(VariablesStringFormatterParamsProvider);
		FAST_CAST_BASE(Framework::IStringFormatterParamsProvider);
		FAST_CAST_END();
	public:
		VariablesStringFormatterParamsProvider(SimpleVariableStorage const & _variables)
		: variables(_variables)
		{}
	public:
		implement_ bool requested_string_formatter_param(REF_ Framework::StringFormatterParams& _params, Name const& _param) const;
	private:
		SimpleVariableStorage const & variables;
	};
};
