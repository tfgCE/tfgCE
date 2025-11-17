#pragma once

#include "..\gameScriptCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class VarsEqual
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				Name varA;
				Name varB;
			};
		}
	}
}