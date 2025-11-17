#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\core\types\option.h"
#include "..\..\..\core\types\optional.h"

#include "..\..\..\framework\world\doorType.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			/**
			 *	Used to permanently switch controls
			 */
			class Player
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ tchar const* get_debug_info() const { return TXT("player"); }

			private:
				Optional<Name> bindToObjectVar;
				bool keepVRAnchor = false;
				bool findVRAnchor = false;

				bool startVRAnchorOSLock = false;
				bool endVRAnchorOSLock = false;

				Optional<Name> storeObjectVar;

				Optional<Name> considerPlayerObjectVar;
				Optional<Name> considerNotPlayerObjectVar;
			};
		};
	};
};
