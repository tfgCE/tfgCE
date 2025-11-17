#include "wmp_sphere.h"

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

bool WheresMyPoint::Sphere::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	sphere.load_from_xml(_node);

	result &= location.load_from_xml(_node->first_child_named(TXT("location")));
	result &= radius.load_from_xml(_node->first_child_named(TXT("radius")));

	return result;
}

bool WheresMyPoint::Sphere::update(REF_ Value & _value, Context & _context) const
{
	bool result = true;

	_value.set(sphere);

	Value locationValue;
	result &= location.update(locationValue, _context);
	::Vector3 locationVector3 = sphere.get_centre();
	if (locationValue.get_type() == type_id<::Vector3>())
	{
		locationVector3 = locationValue.get_as<::Vector3>();
	}

	float radiusFloat = sphere.get_radius();
	if (!radius.is_empty())
	{
		Value radiusValue;
		result &= radius.update(radiusValue, _context);
		if (radiusValue.get_type() == type_id<float>())
		{
			radiusFloat = radiusValue.get_as<float>();
		}
	}

	{
		::Sphere resultSphere = ::Sphere::zero;
		resultSphere.setup(locationVector3, radiusFloat);

		_value.set(resultSphere);
	}

	return result;
}
