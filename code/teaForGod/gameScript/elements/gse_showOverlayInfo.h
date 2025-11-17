#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\library\usedLibraryStored.h"
#include "..\..\..\framework\text\localisedString.h"

namespace Framework
{
	class Font;
};

namespace TeaForGodEmperor
{
	namespace Story
	{
		class Scene;
	};

	namespace GameScript
	{
		namespace Elements
		{
			class ShowOverlayInfo
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("show overlay info"); }

				implement_ void load_on_demand_if_required();

			private:
				Optional<Name> show; // rolled, always clears everything else
				
				Optional<Name> roomVar;
				Optional<Name> atPOI; // will go from world to vr anchor
				Optional<Name> atVRAnchor; // will go from world to vr anchor

				Optional<float> scrollInitialWait;
				Optional<float> scrollPostWait;
				Optional<Range> scrollRange;
				Optional<float> scrollSpeed;
				Optional<float> scale;
				Framework::UsedLibraryStored<Framework::Font> font;
				Optional<bool> forceBlend;
			};
		};
	};
};
