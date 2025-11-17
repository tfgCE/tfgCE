#pragma once

#include "..\..\core\math\math.h"

namespace Framework
{
	struct PredefinedRoomOcclusionCulling
	{
	public:
		bool is_empty() const { return elements.is_empty(); }

		bool is_occluded(Vector3 const& _cameraWS, Vector3 const& _centreWS) const;

		void clear();

	public:
		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName = TXT("predefinedRoomOcclusionCulling"));

	private:
		struct Element
		{
			ArrayStatic<Plane, 6> forCameraLocationWS;

			ArrayStatic<Plane, 6> quickCullLocationWS; // centre of object is enough

			Element()
			{
				SET_EXTRA_DEBUG_INFO(forCameraLocationWS, TXT("PredefinedRoomOcclusionCulling.forCameraLocationWS"));
				SET_EXTRA_DEBUG_INFO(quickCullLocationWS, TXT("PredefinedRoomOcclusionCulling.quickCullLocationWS"));
			}
		};
		Array<Element> elements;
	};
};
