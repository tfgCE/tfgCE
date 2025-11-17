#include "tagsStringFormatterParamsProvider.h"

#include "..\game\energy.h"
#include "..\..\core\tags\tag.h"

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(TagsStringFormatterParamsProvider);

bool TagsStringFormatterParamsProvider::requested_string_formatter_param(REF_ Framework::StringFormatterParams& _params, Name const& _param) const
{
	if (tags.has_tag(_param))
	{
		_params.add(_param, tags.get_tag_as_int(_param));
		_params.add(_param, tags.get_tag(_param));
		return true;
	}
	return false;
}
