#include "meshGenUVDef.h"

#include "meshGenGenerationContext.h"

#include "..\..\core\other\simpleVariableStorage.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

void UVDef::clear()
{
	u.clear();
	uParam.clear();
}

float UVDef::get_u(GenerationContext const & _context) const
{
	if (uParam.is_set())
	{
		if (float const * value = _context.get_parameter<float>(uParam.get()))
		{
			return *value;
		}
		error(TXT("uParam \"%S\" not found!"), uParam.get().to_char());
	}
	if (u.is_set())
	{
		return u.get();
	}
	error(TXT("\"u\" not defined!"));
	return 0.0f;
}

bool UVDef::load_from_xml(IO::XML::Node const * _node)
{
	return load_from_xml(_node, TXT("u"), TXT("uParam"));
}

bool UVDef::load_from_xml(IO::XML::Node const * _node, tchar const * _uAttr, tchar const * _uParamAttr)
{
	bool result = true;

	if (_node)
	{
		u.load_from_xml(_node->get_attribute(_uAttr));
		uParam.load_from_xml(_node->get_attribute(_uParamAttr));
	}

	return result;
}

bool UVDef::load_from_xml_with_clearing(IO::XML::Node const * _node)
{
	bool result = true;

	IO::XML::Attribute const * uAttr = _node->get_attribute(TXT("u"));
	IO::XML::Attribute const * uParamAttr = _node->get_attribute(TXT("uParam"));
	if (uAttr || uParamAttr)
	{
		clear();
		u.load_from_xml(uAttr);
		uParam.load_from_xml(uParamAttr);
	}

	return result;
}
