#pragma once

#include "gameSettings.h"

namespace TeaForGodEmperor
{
	class GameDefinition;
	struct RoomGenerationInfo;

	struct RunSetup
	{
		Tags activeGameModifiers;

		//

		static void update_rgi_requested_generation_tags(Tags const& activeGameModifiers);

		//
		void read_into_this(); // read into this
		void post_read_into_this(); // auto called from read
		void update_using_this(); // will reset to chosen GameDefinition first, then apply game modifiers

		bool load_from_xml(IO::XML::Node const* _node);
		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName = TXT("runSetup"));
		bool save_to_xml(IO::XML::Node* _node) const;
		bool save_to_xml_child_node(IO::XML::Node* _node, tchar const* _childName = TXT("runSetup")) const;
	};
};
