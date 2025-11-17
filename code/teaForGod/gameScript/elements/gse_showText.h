#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\core\types\colour.h"
#include "..\..\..\framework\text\localisedString.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class ShowText
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public:
				ShowText() {}

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("show text"); }

			private:
				Name id;
				Colour colour = Colour::white;
				float verticalAlign = 0.5f;
				bool pulse = false;

				Framework::LocalisedString text;

				Rotator3 offset = Rotator3::zero;
				bool relToCamera = false;
			};
		};
	};
};
