#include "wmp_get.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool Get::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	name = _node->get_name_attribute(TXT("field"), name);
	name = _node->get_name_attribute(TXT("sub"), name);
	name = _node->get_name_attribute(TXT("subfield"), name);
	name = _node->get_name_attribute(TXT("subField"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no sub field defined"));
		result = false;
	}

	return result;
}

bool Get::load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix)
{
	bool result = base::load_from_xml(_node);

	name = Name(_node->get_name().get_sub(_prefix.get_length()).trim());

	return result;
}

bool Get::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value subField;

	void* subFieldRawData = nullptr;
	TypeId subFieldId = type_id_none();
	{
		if (access_sub_field(name, _value.access_raw(), _value.get_type(), subFieldRawData, subFieldId))
		{
			subField.set_raw(subFieldId, subFieldRawData);
		}
		else
		{
			error_processing_wmp(TXT("unable to find sub field \"%S\""), name.to_char());
			result = false;
		}
	}

	_value = subField;

	return result;
}
