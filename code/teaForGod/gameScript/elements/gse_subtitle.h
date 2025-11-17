#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"
#include "..\..\..\framework\text\localisedString.h"

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
			class Subtitle
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("subtitle"); }

			private:
				Name useTextId;
				Framework::LocalisedString text;
				Optional<Colour> colour;
				Optional<Colour> backgroundColour;
				float time = 5.0f;
			};
		};
	};
};
