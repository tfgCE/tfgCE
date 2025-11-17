#pragma once

#include "..\..\framework\text\stringFormatter.h"

//

struct Tags;

//

namespace TeaForGodEmperor
{
	struct TagsStringFormatterParamsProvider
	: public Framework::IStringFormatterParamsProvider
	{
		FAST_CAST_DECLARE(TagsStringFormatterParamsProvider);
		FAST_CAST_BASE(Framework::IStringFormatterParamsProvider);
		FAST_CAST_END();
	public:
		TagsStringFormatterParamsProvider(Tags const & _tags)
		: tags(_tags)
		{}
	public:
		implement_ bool requested_string_formatter_param(REF_ Framework::StringFormatterParams& _params, Name const& _param) const;
	private:
		Tags const & tags;
	};
};
