#include "wmp_plane.h"

#include "iWMPOwner.h"
#include "wmp_context.h"

#include "..\io\xml.h"
#include "..\math\math.h"

using namespace WheresMyPoint;

bool WheresMyPoint::Plane::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	plane.load_from_xml(_node);

	result &= normal.load_from_xml(_node->first_child_named(TXT("normal")));
	result &= location.load_from_xml(_node->first_child_named(TXT("location")));
	int pointIdx = 0;
	loadedPoints = 0;
	for_every(pointNode, _node->children_named(TXT("point")))
	{
		if (pointIdx < 3)
		{
			if (points[pointIdx].load_from_xml(pointNode))
			{
				++loadedPoints;
				++pointIdx;
			}
			else
			{
				error_loading_xml(pointNode, TXT("error loading point for \"where's my point\" plane"));
				result = false;
			}
		}
		else
		{
			pointIdx = 5;
			break;
		}
	}
	
	if (pointIdx > 0 && pointIdx != 3)
	{
		error_loading_xml(_node, TXT("incorrect number of points!"));
		result = false;
	}

	return result;
}

bool WheresMyPoint::Plane::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	::Plane resultPlane = plane;

	{
		Value normalValue;
		normalValue.set(resultPlane.get_normal());
		if (normal.update(normalValue, _context))
		{
			resultPlane.set(normalValue.get_as<::Vector3>(), resultPlane.get_anchor());
		}
		else
		{
			result = false;
		}
	}

	{
		Value locationValue;
		locationValue.set(resultPlane.get_anchor());
		if (location.update(locationValue, _context))
		{
			resultPlane.set(resultPlane.get_normal(), locationValue.get_as<::Vector3>());
		}
		else
		{
			result = false;
		}
	}

	if (loadedPoints == 3)
	{
		Value pointValues[3];
		if (points[0].update(pointValues[0], _context) &&
			points[1].update(pointValues[1], _context) &&
			points[2].update(pointValues[2], _context))
		{
			resultPlane.set(pointValues[0].get_as<::Vector3>(),
							pointValues[1].get_as<::Vector3>(),
							pointValues[2].get_as<::Vector3>());
		}
		else
		{
			result = false;
		}
	}

	_value.set(resultPlane);

	return result;
}
