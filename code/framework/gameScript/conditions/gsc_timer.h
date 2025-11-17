#pragma once

#include "..\gameScriptCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class Timer
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				Name timerID;
				
				Optional<float> lessOrEqual;
				Optional<float> greaterOrEqual;
			};
		}
	}
}