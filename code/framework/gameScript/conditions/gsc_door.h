#pragma once

#include "..\gameScriptCondition.h"

#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Conditions
		{
			class Door
			: public ScriptCondition
			{
				typedef ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(ScriptExecution const& _execution) const;

			private:
				Name doorVar;

				Optional<bool> isClosed;
				Optional<bool> isOpen;
			};
		}
	}
}