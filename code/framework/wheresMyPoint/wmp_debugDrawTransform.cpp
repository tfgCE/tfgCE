#include "wmp_debugDrawTransform.h"

#include "..\meshGen\meshGenMeshNodeContext.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

//

bool DebugDrawTransform::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (auto const * node = _node->first_child_named(TXT("transform")))
	{
		result &= transform.load_from_xml(node);
	}

	if (auto const * attr = _node->get_attribute(TXT("colour")))
	{
		result &= colour.load_from_string(attr->get_as_string());
	}
	if (auto const * node = _node->first_child_named(TXT("colour")))
	{
		result &= colour.load_from_xml(node);
	}

	size = _node->get_float_attribute_or_from_child(TXT("size"), size);

	return result;
}

bool DebugDrawTransform::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

#ifdef AN_DEVELOPMENT
	Transform actualTransform = Transform::identity;
	if (_value.get_type() == type_id<Transform>())
	{
		actualTransform = _value.get_as<Transform>();
	}

	if (!transform.is_empty())
	{
		WheresMyPoint::Value value = _value;
		if (transform.update(value, _context))
		{
			if (value.get_type() == type_id<Transform>())
			{
				actualTransform = value.get_as<Transform>();
			}
			else
			{
				error(TXT("expecing transform!"));
			}
		}
	}

	WheresMyPoint::IOwner* wmpOwner = _context.get_owner();
	while (wmpOwner)
	{
		if (auto * context = fast_cast<MeshGeneration::GenerationContext>(wmpOwner))
		{
			if (auto * drf = context->debugRendererFrame)
			{
				Colour r = Colour::red;
				Colour g = Colour::green;
				Colour b = Colour::blue;
				r = Colour::lerp(colour.a, r, colour);
				g = Colour::lerp(colour.a, g, colour);
				b = Colour::lerp(colour.a, b, colour);
				r.a = 1.0f;
				g.a = 1.0f;
				b.a = 1.0f;
				drf->add_matrix(true, actualTransform.to_matrix(), size, r, g, b);
			}
		}
		wmpOwner = wmpOwner->get_wmp_owner();
	}
#endif

	return result;
}
