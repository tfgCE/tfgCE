#include "wmp_intersect.h"

#include "..\containers\arrayStack.h"
#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

Intersect::~Intersect()
{
	for_every(intersect, intersecting)
	{
		delete_and_clear(*intersect);
	}
}

bool Intersect::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	for_every(node, _node->all_children())
	{
		if (ITool* tool = ITool::create_from_xml(node))
		{
			intersecting.push_back(tool);
		}
	}

	return result;
}

bool Intersect::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	Value resultValue;

	ARRAY_PREALLOC_SIZE(Value, values, intersecting.get_size());
	values.set_size(intersecting.get_size());

	for (int i = 0; i < intersecting.get_size(); ++i)
	{
		result &= intersecting[i]->update(values[i], _context);
	}

	if (result)
	{
		if (intersecting.get_size() == 3 &&
			values[0].get_type() == ::type_id<Plane>() &&
			values[1].get_type() == ::type_id<Plane>() &&
			values[2].get_type() == ::type_id<Plane>())
		{
			resultValue.set(Plane::intersect(values[0].get_as<Plane>(), values[1].get_as<Plane>(), values[2].get_as<Plane>()));
		}
		else
		{
			error_processing_wmp(TXT("can't handle this intersect"));
			result = false;
		}
	}

	_value = resultValue;

	return result;
}

