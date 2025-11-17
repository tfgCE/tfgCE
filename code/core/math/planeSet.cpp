#include "math.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

PlaneSet PlaneSet::s_empty;

bool PlaneSet::load_from_xml_child_nodes(IO::XML::Node const * _node, tchar const * _childName)
{
	bool result = true;
	for_every(node, _node->children_named(_childName))
	{
		Plane plane = Plane::identity;
		if (plane.load_from_xml(node))
		{
			add(plane);
		}
		else
		{
			error_loading_xml(node, TXT("could not load plane for plane set"));
			result = false;
		}
	}
	return result;
}

void PlaneSet::add_smartly_more_relevant(Plane const& _plane)
{
	bool addNow = true;
	for(int i = 0; i < planes.get_size(); ++ i)
	{
		auto& p = planes[i];
		if ((p.get_normal() - _plane.get_normal()).is_almost_zero())
		{
			if (p.get_in_front(_plane.get_anchor()) >= -EPSILON)
			{
				planes.remove_at(i);
				--i;
			}
			else
			{
				addNow = false;
			}
		}
	}
	if (addNow)
	{
		if (planes.get_size() >= PLANE_SET_PLANE_LIMIT)
		{
			planes.pop_front();
		}
		planes.push_back(_plane);
	}
}
