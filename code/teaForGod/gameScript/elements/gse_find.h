#pragma once

#include "..\..\..\framework\gameScript\elements\gse_find.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Find
			: public Framework::GameScript::Elements::Find
			{
				typedef Framework::GameScript::Elements::Find base;
			public: // ScriptElement
				override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

			protected:
				virtual Framework::World* get_world(Framework::GameScript::ScriptExecution& _execution) const;

				virtual Framework::Room* get_room_for_find_object(Framework::GameScript::ScriptExecution& _execution) const;

				virtual void find_room(Framework::GameScript::ScriptExecution& _execution, OUT_ Results& _results) const;

			protected:
				bool lockerRoom = false;
				bool inLockerRoom = false;

				bool inPlayersRoom = false;
			};
		};
	};
};
