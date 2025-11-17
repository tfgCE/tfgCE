#include "wmp_access.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace WheresMyPoint;

//

bool Access::load_from_xml(IO::XML::Node const * _node)
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

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool Access::load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix)
{
	bool result = base::load_from_xml(_node);

	name = Name(_node->get_name().get_sub(_prefix.get_length()).trim());

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool Access::update(REF_ Value & _value, Context & _context) const
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

	result &= toolSet.update(subField, _context);

	// store from where it was read (_value) but keep _value as it was (type id, whole data, just with modified sub field)
	if (subFieldRawData)
	{
		if (subFieldId != subField.get_type())
		{
			error_processing_wmp(TXT("mismatch of sub field type with a result \"%S\" v \"%S\""), RegisteredType::get_name_of(subField.get_type()), RegisteredType::get_name_of(subFieldId));
		}
		subField.read_raw(subFieldId, subFieldRawData);
	}

	return result;
}
