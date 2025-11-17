#pragma once

#include "..\..\..\core\math\math.h"

struct Plane;

namespace Framework
{
	namespace MeshGeneration
	{
		namespace Modifiers
		{
			struct Selection
			{
			public:
				bool is_empty() const;

				bool is_selected(Vector3 const& _at) const;

				bool load_from_child_node_xml(IO::XML::Node const* _node, tchar const* _childNode = TXT("forSelected"));

			private:
				Array<Box> boxes;
			};
		};
	};
};
