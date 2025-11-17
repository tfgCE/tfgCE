#pragma once

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\name.h"

namespace Framework
{
	struct LayoutSocket
	{
		Name name;
		Transform placement = Transform::identity;

		bool load_from_xml(IO::XML::Node const* _node);
	};
};
