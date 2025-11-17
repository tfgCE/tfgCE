#pragma once

#include "..\gameScriptElement.h"
#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class TeleportAllUsingVRAnchor
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("teleport all using vr anchor"); }

			private:
				Name mainObjectVar; // used for vr anchor, etc

				Name fromRoomVar;
				Name toRoomVar;

				TagCondition tagged;

				Name atPOI; // for sliding movement we use relation to POI in one room and another, for normal we want to teleport as close as possible

				bool useRoomVRAnchors = true; // this is default as we may get rotated vr anchors, we should teleport only when we have really strict vr anchor placements
			};
		};
	};
};
