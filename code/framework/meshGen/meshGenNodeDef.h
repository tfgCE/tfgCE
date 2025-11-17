#pragma once

#include "..\..\core\enums.h"

#include "meshGenSubStepDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationContext;
		struct ElementInstance;

		struct NodeDef
		{
			float get_auto_smooth_normal_dot_limit() const { return autoSmoothNormalDotLimit; }
			
			bool load_from_xml(IO::XML::Node const * _node);
			bool sub_load_from_xml(IO::XML::Node const * _node);

		private:
			float autoSmoothNormalDotLimit = -2.0f; // if above, allows to smooth, has to be set in both edges and nodes, uses greater value (by default all nodes can be smoothed)
		};
	};
};
