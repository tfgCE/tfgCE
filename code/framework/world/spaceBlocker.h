#pragma once

#include "..\..\core\math\math.h"

class DebugVisualiser;

namespace Framework
{
	struct SpaceBlocker
	{
		bool blocks = false;
		bool requiresToBeEmpty = false; // may cross their own "blocks"
		Box box;

#ifdef AN_DEVELOPMENT
		String debugInfo;
#endif

		bool load_from_xml(IO::XML::Node const* _node);

		bool is_none() const { return !blocks && !requiresToBeEmpty; }
		static SpaceBlocker none() { SpaceBlocker sb; sb.blocks = false; sb.requiresToBeEmpty = false; return sb; }

		void log(LogInfoContext& _context) const;

		bool check(SpaceBlocker const& _blocker) const;

#ifdef AN_DEVELOPMENT
		void debug_visualise(DebugVisualiser* dv, Transform const& _placement) const;
#endif
	};

	struct SpaceBlockers
	{
		Array<SpaceBlocker> blockers;

		bool check(SpaceBlockers const& _blockers, Transform const& _placement = Transform::identity) const;
		void add(SpaceBlockers const& _blockers, Transform const& _placement = Transform::identity);

		bool is_empty() const { return blockers.is_empty(); }
		int get_size() const { return blockers.get_size(); }
		void clear() { blockers.clear(); }

		bool load_from_xml(IO::XML::Node const* _node);
		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childNodeName = TXT("spaceBlockers"));

		void debug_draw() const;
		void log(LogInfoContext& _context) const;

#ifdef AN_DEVELOPMENT
		void debug_visualise(DebugVisualiser* dv, Transform const& _placement = Transform::identity) const;
#endif
	};
};
