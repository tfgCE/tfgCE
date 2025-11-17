#pragma once

#include "..\..\core\enums.h"

#include "meshGenSubStepDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationContext;
		struct ElementInstance;

		struct SurfaceDef
		{
			float get_max_edge_length() const { return maxEdgeLength; }
			SubStepDef const * get_sub_step_override() const { return subStep.is_set() ? &subStep.get() : nullptr; }
			bool should_add_inner_grid() const { return addInnerGrid; }
			bool should_allow_simple_flat() const { return useSimpleFlat != Allowance::Disallow; }
			bool should_force_simple_flat() const { return useSimpleFlat == Allowance::Force; }

			Optional<SubStepDef> & access_sub_step_override() { return subStep; }

			bool load_from_xml(IO::XML::Node const * _node);
			bool sub_load_from_xml(IO::XML::Node const * _node);

		private:
			float maxEdgeLength = 0.0f;
			Optional<SubStepDef> subStep; // allows to override_ per edge
			bool addInnerGrid = false; // auto adds points using highest substep division
			Allowance::Type useSimpleFlat = Allowance::Allow; // allow being simple flat

		};
	};
};
