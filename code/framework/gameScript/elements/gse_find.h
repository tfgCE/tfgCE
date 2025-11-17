#pragma once

#include "..\gameScriptElement.h"
#include "..\..\..\core\tags\tagCondition.h"

namespace Framework
{
	class Door;
	class Room;
	class World;
	interface_class IModulesOwner;

	namespace GameScript
	{
		namespace Elements
		{
			class Find
			: public ScriptElement
			{
				typedef ScriptElement base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
				implement_ bool prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext);
				implement_ ScriptExecutionResult::Type execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const;
				implement_ tchar const* get_debug_info() const { return TXT("find"); }

			protected:
				struct Results
				{
					SafePtr<Framework::IModulesOwner> found;
					SafePtr<Framework::Room> foundRoom;
					SafePtr<Framework::Door> foundDoor;

					Array<SafePtr<Framework::IModulesOwner>> allFound;
				};

				virtual World* get_world(ScriptExecution& _execution) const;

				virtual void find_object(ScriptExecution& _execution, OUT_ Results & _results) const;
				virtual Room* get_room_for_find_object(ScriptExecution& _execution) const;
				
				virtual void find_room(ScriptExecution& _execution, OUT_ Results& _results) const;
				
				virtual void find_door(ScriptExecution& _execution, OUT_ Results& _results) const;

			protected:
				TagCondition tagged;
				TagCondition roomTagged;
				TagCondition doorTagged;

				Range3 inRange = Range3::empty;

				Name storeVar;
				
				Name inRoomVar;
				
				Name attachedToVar;

				bool storeOnlyIfFound = false;
				
				bool mayFail = false;

				bool chooseRandomly = false; // choose one randomly, not just the first one

				Name goToLabelOnFail = Name::invalid();

				bool check_conditions_for(ScriptExecution& _execution, Framework::IModulesOwner* imo) const;
			};
		};
	};
};
