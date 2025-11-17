#pragma once

#include "..\gameScriptElement.h"
#include "..\..\ai\movementParameters.h"
#include "..\..\library\usedLibraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Locomotion
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("locomotion"); }

			private:
				Name objectVar;
				
				bool stop = false;
				bool dontControl = false;

				bool move3D = false;
				bool moveDirectly = false; // doesn't look for a path

				Name moveToObjectVar; // will find nav path to get there
				Optional<Vector3> moveToLoc; // direct movement

				Optional<float> moveToRadius;
				Framework::MovementParameters movementParameters;
			};
		};
	};
};
