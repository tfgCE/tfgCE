#pragma once

#include "..\..\..\framework\gameScript\gameScriptCondition.h"

#include "..\..\..\core\tags\tagCondition.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	class Pilgrimage;

	namespace GameScript
	{
		namespace Conditions
		{
			class Persistence
			: public Framework::GameScript::ScriptCondition
			{
				typedef Framework::GameScript::ScriptCondition base;
			public: // ScriptCondition
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);

				implement_ bool check(Framework::GameScript::ScriptExecution const& _execution) const;

			private:
				Optional<bool> tooManyWeapons;
			};
		}
	}
}