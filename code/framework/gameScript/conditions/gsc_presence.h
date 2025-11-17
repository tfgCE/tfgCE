#pragma once

#include "..\gameScriptCondition.h"
#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class Presence
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				Name objectVar;

				Name objectVarBased; // on objectVar
				Name basedOnObjectVar; // objectVar based on this one

				Name sameRoomAsObjectVar;
				Name sameOriginRoomAsObjectVar;
				
				Name inRoomVar;

				TagCondition inRegionTagged;
				TagCondition inRoomTagged;

				Optional<float> inRoomAtLeast; // checks against doors

				Optional<float> belowZ;
				Optional<Range> xInRange;
				Optional<Range> yInRange;
				Optional<Range> zInRange;

				Optional<float> maxSpeed;
			};
		}
	}
}