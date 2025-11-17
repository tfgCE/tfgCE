#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class Sample;
};

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class VoiceoverSpeak
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public:
				VoiceoverSpeak() {}

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("voiceover speak"); }

				implement_ void load_on_demand_if_required();

			private:
				Name actor;
				Framework::UsedLibraryStored<Framework::Sample> sample;
				String devLine; // <voiceoverSpeak actor="who"><devLine/>hello world</voiceoverSpeak>
				bool shouldWait = true;
				Optional<int> audioDuck;
			};
		};
	};
};
