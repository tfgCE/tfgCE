#pragma once

#include "..\..\..\framework\gameScript\elements\gse_teleport.h"

namespace TeaForGodEmperor
{
	namespace GameScript
	{
		namespace Elements
		{
			class Teleport
			: public Framework::GameScript::Elements::Teleport
			{
				typedef Framework::GameScript::Elements::Teleport base;
			public: // ScriptElement
				implement_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

			protected:
				override_ Framework::Room* get_to_room(Framework::GameScript::ScriptExecution& _execution) const;

				override_ void alter_target_placement(Optional<Transform>& _targetPlacement) const;

			protected:
				bool toLockerRoom = false;
			};
		};
	};
};
