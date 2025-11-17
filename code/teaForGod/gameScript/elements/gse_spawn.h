#pragma once

#include "..\..\..\framework\gameScript\gameScriptElement.h"

#include "..\..\..\framework\library\usedLibraryStored.h"

namespace Framework
{
	class ActorType;
	class ItemType;
	class SceneryType;
};

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Spawn
			: public Framework::GameScript::ScriptElement
			{
				typedef Framework::GameScript::ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ Framework::GameScript::ScriptExecutionResult::Type execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("spawn"); }

			private:
				Vector3 relativeLocation = Vector3(0.0f, 1.0f, 1.0f);
				Rotator3 relativeRotation = Rotator3(0.0f, 180.0f, 0.0f); // by default face the player (it is calculated on load basing on relativeLocation)
				bool withinNav = false;
				bool inLocker = false;

				Name inRoomVar;
				Optional<Transform> placementWS; // used only if inRoomVar provided
				Optional<Name> placeAtPOI; // used only if inRoomVar provided
				Optional<Name> placeAtPOIVar; // used only if inRoomVar provided

				Framework::UsedLibraryStored<Framework::ItemType> itemType;
				Framework::UsedLibraryStored<Framework::ActorType> actorType;
				Framework::UsedLibraryStored<Framework::SceneryType> sceneryType;

				Name storeVar;

				SimpleVariableStorage parameters;
			};
		};
	};
};
