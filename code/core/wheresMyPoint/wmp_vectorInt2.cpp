#include "wmp_vectorInt2.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool WheresMyPoint::VectorInt2::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	value.load_from_xml(_node);

	result &= toolSet.load_from_xml(_node);

	return result;
}

bool WheresMyPoint::VectorInt2::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	::VectorInt2 resultValue;
	result &= process_value(resultValue, value, toolSet, _context, TXT("\"vectorInt2\" expected vectorInt2 value"));
	_value.set(resultValue);

	return result;
}

//

bool ToVectorInt2::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	if (_value.get_type() == type_id<::Vector2>())
	{
		_value.set(_value.get_as<::Vector2>().to_vector_int2());
	}
	else
	{
		error_processing_wmp(TXT("can't convert \"%S\" to VectorInt2"), RegisteredType::get_name_of(_value.get_type()));
		result = false;
	}

	return result;
}