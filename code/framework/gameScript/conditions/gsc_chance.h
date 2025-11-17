#pragma once

#include "..\gameScriptCondition.h"

#include "..\..\..\core\math\math.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class Chance
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				float chance = 0.5f;
			};
		}
	}
}