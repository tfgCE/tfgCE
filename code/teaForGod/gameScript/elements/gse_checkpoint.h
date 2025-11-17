#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\core\tags\tagCondition.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	class PilgrimageDevice;

	namespace Story
	{
		class Scene;
	};

	namespace GameScript
	{
		namespace Elements
		{
			/**
			 *	Note that it can be used also with room tagged "checkpointRoom", then we store tagCondition "checkpointRoomTagged" from that room variables
			 *	Use restartInRoomTagged only if you want to force restarting there.
			 */
			class Checkpoint
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("make checkpoint"); }

			private:
				TagCondition restartInRoomTagged; // check above
			};
		};
	};
};
