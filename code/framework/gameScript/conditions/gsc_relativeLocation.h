#pragma once

#include "..\gameScriptCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class RelativeLocation
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				Name toObjectVar;
				Name toSocket;
				
				Name checkObjectVar;
				Name checkSocket;

				// one of below has to be added, otherwise an error during loading (implement more!)
				Optional<float> xLessThan;
				Optional<float> xGreaterThan;
				Optional<float> yLessThan;
				Optional<float> yGreaterThan;
			};
		}
	}
}