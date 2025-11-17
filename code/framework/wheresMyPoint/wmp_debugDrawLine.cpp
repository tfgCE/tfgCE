#include "wmp_debugDrawLine.h"

#include "..\meshGen\meshGenMeshNodeContext.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

//

bool DebugDrawLine::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	if (auto const * node = _node->first_child_named(TXT("from")))
	{
		result &= from.load_from_xml(node);
	}

	if (auto const * node = _node->first_child_named(TXT("to")))
	{
		result &= to.load_from_xml(node);
	}

	if (auto const * attr = _node->get_attribute(TXT("colour")))
	{
		result &= colour.load_from_string(attr->get_as_string());
	}
	if (auto const * node = _node->first_child_named(TXT("colour")))
	{
		result &= colour.load_from_xml(node);
	}

	return result;
}

bool DebugDrawLine::update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const
{
	bool result = true;

#ifdef AN_DEVELOPMENT
	Vector3 actualFrom = Vector3::zero;
	Vector3 actualTo = Vector3::zero;
	if (_value.get_type() == type_id<Vector3>())
	{
		actualTo = actualFrom = _value.get_as<Vector3>();
	}

	if (!from.is_empty())
	{
		WheresMyPoint::Value value = _value;
		if (from.update(value, _context))
		{
			if (value.get_type() == type_id<Vector3>())
			{
				actualFrom = value.get_as<Vector3>();
			}
			else
			{
				error(TXT("expecing Vector3!"));
			}
		}
	}

	if (!to.is_empty())
	{
		WheresMyPoint::Value value = _value;
		if (to.update(value, _context))
		{
			if (value.get_type() == type_id<Vector3>())
			{
				actualTo = value.get_as<Vector3>();
			}
			else
			{
				error(TXT("expecing Vector3!"));
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
				draw(drf, colour, actualFrom, actualTo);
			}
		}
		wmpOwner = wmpOwner->get_wmp_owner();
	}
#endif

	return result;
}

void DebugDrawLine::draw(DebugRendererFrame* _drf, Colour const & _colour, Vector3 const & _a, Vector3 const & _b) const
{
	_drf->add_line(true, _colour, _a, _b);
}
