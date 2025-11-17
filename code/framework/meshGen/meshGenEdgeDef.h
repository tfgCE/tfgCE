#pragma once

#include "..\..\core\enums.h"

#include "meshGenSubStepDef.h"

namespace Framework
{
	namespace MeshGeneration
	{
		struct GenerationContext;
		struct ElementInstance;

		struct EdgeDef
		{
			SubStepDef const & get_sub_step() const { return subStep; }
			bool should_allow_round_separation() const { return useRoundSeparation != Allowance::Disallow; }
			bool should_force_round_separation() const { return useRoundSeparation == Allowance::Force; }
			float get_auto_smooth_normal_dot_limit() const { return autoSmoothNormalDotLimit; }
			Name const & get_auto_smooth_surface_group() const { return autoSmoothSurfaceGroup; }

			bool load_from_xml(IO::XML::Node const * _node);
			bool sub_load_from_xml(IO::XML::Node const * _node);

		private:
			SubStepDef subStep;
			Allowance::Type useRoundSeparation = Allowance::Allow;
			float autoSmoothNormalDotLimit = 2.0f; // if above, allows to smooth, has to be set in both edges and nodes, uses greater value (by default, no edge can be smoothed)
			Name autoSmoothSurfaceGroup;
		};
	};
};
