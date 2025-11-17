#pragma once

#include "..\..\..\framework\gameScript\gameScriptCondition.h"

#include "..\..\..\core\tags\tagCondition.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Conditions
		{
			class GameDefinition
			: public Framework::GameScript::ScriptCondition
			{
				typedef Framework::GameScript::ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

				implement_ bool check(Framework::GameScript::ScriptExecution const& _execution) const;

			private:
				TagCondition tagged;
			};
		}
	}
}