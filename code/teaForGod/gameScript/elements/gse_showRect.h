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
			class ShowRect
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public:
				ShowRect() {}

			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("show rect"); }

			private:
				Name id;
				Colour colour = Colour::white;
				bool pulse = false;

				Rotator3 offset = Rotator3::zero;
				Vector2 size = Vector2(5.0f, 5.0f); // angles
				Vector2 alignPt = Vector2::half;
			};
		};
	};
};
