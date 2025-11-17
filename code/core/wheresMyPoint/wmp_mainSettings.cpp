#include "wmp_mainSettings.h"

#include "..\mainSettings.h"

//

using namespace WheresMyPoint;

//

bool WheresMyPoint::MainSettings::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	shaderOption = _node->get_name_attribute_or_from_child(TXT("shaderOption"), shaderOption);

	return result;
}

bool WheresMyPoint::MainSettings::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	bool anyOk = false;
	bool anyFailed = false;

	if (shaderOption.is_valid())
	{
		bool ok = ::MainSettings::global().get_shader_options().get_tag(shaderOption);
		anyOk = ok;
		anyFailed = !ok;
	}
	
	_value.set(anyOk && ! anyFailed);

	return result;
}
