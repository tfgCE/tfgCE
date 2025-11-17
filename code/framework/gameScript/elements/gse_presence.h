#pragma once

#include "..\gameScriptElement.h"
#include "..\..\library\usedLibraryStored.h"

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class Presence
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("presence"); }

			private:
				Name objectVar;

				Optional<bool> zeroVRAnchor;

				Optional<bool> lockAsBase;
				Optional<bool> detach;

				Optional<Name> attachAsIsToObjectVar;
				Optional<Name> attachToObjectVar;
				Optional<Name> attachToSocket;
				Optional<Transform> relativePlacement; // for attachement

				Optional<Name> storeInRoomVar;

				Optional<Vector3> setVelocityWS;
				Optional<Vector3> velocityImpulseWS;
				Optional<Rotator3> setVelocityRotation;
				Optional<Rotator3> orientationImpulse;
			};
		};
	};
};
