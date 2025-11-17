#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\core\types\option.h"
#include "..\..\..\core\types\optional.h"

#include "..\..\..\framework\world\doorType.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			/**
			 *	Will switch controls (temporarily) using playerTakenControl to a different object
			 *	Handles the effect of change too
			 *	With "controlsOnly" switches controls immediately (!) but remains seeing from the same body
			 */
			class TakeControl
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ tchar const* get_debug_info() const { return TXT("take controls"); }

			private:
				bool dropControls = false;
				bool controlsOnly = false;
				bool immediate = false;
				bool appearReversedOut = false; // if dropping, will look like taking
				bool appearReversedIn = false; // if mixed both, we may have switch feeling
				Optional<float> blendTime; // if not provided will reset
				Optional<bool> controlBoth; // has already to have taken controls
				Optional<bool> seeThroughEyesOfControlled; // this is useful to explicitly say whose eyes should we use

				Optional<Name> takeControlOverObjectVar;
			};
		};
	};
};
