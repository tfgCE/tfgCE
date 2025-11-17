#include "lightSource.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

bool LightSource::allowFlickeringLights = true;

bool LightSource::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	location.load_from_xml_child_node(_node, TXT("location"));
	stick.load_from_xml_child_node(_node, TXT("stick"));
	coneDir.load_from_xml_child_node(_node, TXT("coneDir"));
	colour.load_from_xml_child_or_attr(_node, TXT("colour"));
	distance = _node->get_float_attribute_or_from_child(TXT("distance"), distance);
	power = _node->get_float_attribute_or_from_child(TXT("power"), power);
	coneInnerAngle = _node->get_float_attribute_or_from_child(TXT("coneInnerAngle"), coneInnerAngle);
	coneOuterAngle = _node->get_float_attribute_or_from_child(TXT("coneOuterAngle"), coneOuterAngle);
	priority = _node->get_int_attribute_or_from_child(TXT("priority"), priority);
	flickeringLight = _node->get_bool_attribute_or_from_child_presence(TXT("flickeringLight"), flickeringLight);

	error_loading_xml_on_assert(stick.is_zero() || coneDir.is_zero(), result, _node, TXT("stick or cone, not both"));

	return result;
}
