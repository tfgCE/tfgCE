#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\game\energy.h"

#include "..\..\..\core\math\math.h"
#include "..\..\..\core\types\hand.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class HitIndicator
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("hit indicator"); }

			private:
				// at which it is coming
				Optional<Hand::Type> atHand;
				Range relAngle = Range(0.0f);
				Range relVerticalAngle = Range(0.0f);
				Optional<Colour> colour;
				Optional<float> delay;
				Optional<bool> damage;
				Optional<bool> reversed;
			};
		};
	};
};
