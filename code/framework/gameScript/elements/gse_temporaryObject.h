#pragma once

#include "..\gameScriptElement.h"

#include "..\..\library\usedLibraryStored.h"
#include "..\..\sound\soundSample.h"

namespace Framework
{
	class TemporaryObjectType;
};

namespace Framework
{
	namespace GameScript
	{
		namespace Elements
		{
			class TemporaryObject
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("temporary object"); }

			private:
				bool allowToFail = false;

				Name objectVar;
				Name instigatorObjectVar; // useful for some particles
				Name start; // one in objectVar
				Name storeAsVar; // will store started in var
				Name storeAbsolutePlacementAsVar; // will store the chosen placement in var

				TagCondition systemTagRequired;

				Framework::UsedLibraryStored<Framework::TemporaryObjectType> spawn;
				Optional<Name> inRoomVar; // in room
				Optional<Name> poi; // in room
				Optional<Name> nearestPOI; // in room
				Optional<Range> nearestPOIAdditionalDistance;
				Optional<Name> occupyPOI; // will look only for non occupied POI (using POI manager) and if found, will occupy
				Optional<float> occupyPOITime;
				Optional<Name> socket;
				Optional<Name> socketVar;
				Optional<bool> attached;
				Transform relativePlacement = Transform::identity;
				Range3 offsetDir = Range3::empty;
				Range3 offsetLocation = Range3::empty;
				Vector3 relativeVelocity = Vector3::zero;
				Name relativePlacementVar; // vec3/transform
				Name absolutePlacementVar; // vec3/transform

				SimpleVariableStorage variables;
				Array<Name> modulesAffectedByVariables;

				bool stop = false; // pointed by objectVar

			};
		};
	};
};
