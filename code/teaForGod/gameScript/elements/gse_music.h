#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Music
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("music"); }

			private:
				Optional<bool> allowNonGameMusic;
				bool requestNone = false;
				bool requestAmbient = false;
				bool requestIncidental = false;
				bool requestIncidentalOverlay = false;
				bool requestCombatAuto = false;
				Optional<Name> requestMusicTrack;
				Optional<Name> requestIncidentalMusicTrack;
			};
		};
	};
};
