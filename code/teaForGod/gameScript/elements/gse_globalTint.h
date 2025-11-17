#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\game\energy.h"

#include "..\..\..\core\math\math.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class GlobalTint
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("global tint"); }

			private:
				Optional<float> active;
				Optional<Colour> current;
				Optional<float> targetActive;
				Optional<Colour> targetColour;
				Optional<float> blendTime;
				bool clear = false;
			};
		};
	};
};
