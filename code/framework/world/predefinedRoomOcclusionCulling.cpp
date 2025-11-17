#include "predefinedRoomOcclusionCulling.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

void PredefinedRoomOcclusionCulling::clear()
{
	elements.clear();
}

bool PredefinedRoomOcclusionCulling::is_occluded(Vector3 const& _cameraWS, Vector3 const& _centreWS) const
{
	for_every(e, elements)
	{
		bool cameraOK = true;
		for_every(p, e->forCameraLocationWS)
		{
			if (p->get_in_front(_cameraWS) < 0.0f)
			{
				cameraOK = false;
				break;
			}
		}
		if (cameraOK)
		{
			bool occluded = true;
			for_every(p, e->quickCullLocationWS)
			{
				if (p->get_in_front(_centreWS) < 0.0f)
				{
					occluded = false;
					break;
				}
			}
			if (occluded)
			{
				return true;
			}
		}
	}
	return false;
}

bool PredefinedRoomOcclusionCulling::load_from_xml_child_node(IO::XML::Node const* _node, tchar const * _childName)
{
	bool result = true;

	clear();

	for_every(node, _node->children_named(_childName))
	{
		for_every(n, node->children_named(TXT("occlude")))
		{
			Element e;
			for_every(fn, n->children_named(TXT("forCameraInFrontOfPlane")))
			{
				Plane p = Plane::zero;
				if (p.load_from_xml(fn))
				{
					e.forCameraLocationWS.push_back(p);
				}
				else
				{
					result = false;
					error_loading_xml(fn, TXT("problem loading occlusion culling"));
				}
			}
			for_every(fn, n->children_named(TXT("quickCullInFrontOfPlane")))
			{
				Plane p = Plane::zero;
				if (p.load_from_xml(fn))
				{
					e.quickCullLocationWS.push_back(p);
				}
				else
				{
					result = false;
					error_loading_xml(fn, TXT("problem loading occlusion culling"));
				}
			}
			elements.push_back(e);
		}
	}

	return result;
}
