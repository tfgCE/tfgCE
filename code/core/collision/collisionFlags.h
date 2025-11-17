#pragma once

#include "..\types\name.h"
#include "..\types\optional.h"
#include "..\containers\map.h"

#ifdef AN_NAT_VIS
//#define AN_NAT_VIS_COLLISION_FLAGS
#endif

namespace IO
{
	namespace XML
	{
		class Attribute;
		class Node;
	};
};

namespace Collision
{
	class DefinedFlags;

	/**
	 *	Flags
	 *
	 *	User can define DefinedFlags/masks that consist of various flags
	 *
	 *	There are few names that should not be used
	 *		all
	 *		clear
	 *		none
	 *		clean
	 *
	 *	Rejects ignore objects flags, they live someway outside of stuff and may be use to reject anything
	 *	That's why there is "default" flag, all without rejects (as defined + all containing "reject")
	 */
	struct Flags
	{
		uint32 flags;

		Flags() {}
		static Flags const & none() { return s_none; }
		static Flags const & defaults() { return s_default; } // default is
		static Flags const & all() { return s_all; }
		static Flags of_index(int _index);

		inline bool is_empty() const { return flags == 0; }
		inline static bool check(Flags const & _a, Flags const & _b) { return (_a.flags & _b.flags) != 0; }
		inline Flags operator & (Flags const & _other) const { Flags res; res.flags = flags & _other.flags; res.update_for_nat_vis(); return res; }
		inline Flags operator | (Flags const & _other) const { Flags res; res.flags = flags | _other.flags; res.update_for_nat_vis(); return res; }
		inline Flags & operator |= (Flags const & _other) { flags = flags | _other.flags; update_for_nat_vis(); return *this; }
		inline bool operator == (Flags const & _other) const { return flags == _other.flags; }
		inline Flags operator - () const { Flags res; res.flags = ~flags; res.update_for_nat_vis(); return res; }
		inline Flags operator - (Flags const& _other) const { Flags res; res. flags = flags & ~_other.flags; res.update_for_nat_vis(); return res; }
		inline Flags operator -= (Flags const& _other) { flags = flags & ~_other.flags; update_for_nat_vis(); return *this; }

		Flags & clear();
		Flags & add(Name const & _flag);
		Flags & remove(Name const & _flag);

		bool load_from_xml(IO::XML::Node const * _node);
		bool load_from_xml(IO::XML::Node const * _node, tchar const * _attrOrChildName);

		bool apply(String const & _string);

		String to_string() const;

	private:
		static Flags s_none;
		static Flags s_default;
		static Flags s_all;

#ifdef AN_NAT_VIS_COLLISION_FLAGS
		String natVis;
		void update_for_nat_vis();
#else
		inline void update_for_nat_vis() {}
#endif

		friend class DefinedFlags;
	};

	class DefinedFlags
	{
	public:
		static void initialise_static();
		static void close_static();

		DefinedFlags();

		static Flags const & get_existing_or_basic(Name const & _name);
		static bool load_masks_from_xml(IO::XML::Node const * _node);
		static bool check_and_cache();
		static bool has(Name const & _name);

		static Name get_flag_name(int const & _flags);
		static Name const & get_mask_name(int const & _flags);

		static Flags const & get_ik_trace() { return s_ikTrace? s_ikTrace->flags : Flags::all(); }
		static Flags const & get_ik_trace_reject() { return s_ikTraceReject ? s_ikTraceReject->flags : Flags::none(); }
		static Flags const & get_presence_trace() { return s_presenceTrace ? s_presenceTrace->flags : Flags::all(); }
		static Flags const & get_presence_trace_reject() { return s_presenceTraceReject ? s_presenceTraceReject->flags : Flags::none(); }
		static Flags const & get_vision_trace() { return s_visionTrace ? s_visionTrace->flags : Flags::all(); }

	private:
		struct DefinedFlag // it can be mask too!
		{
			Name name;
			Flags flags;
			bool isMask = false;
		};
		static DefinedFlags* s_flags;
		static DefinedFlag const * s_ikTrace;
		static DefinedFlag const * s_ikTraceReject;
		static DefinedFlag const * s_presenceTrace;
		static DefinedFlag const * s_presenceTraceReject;
		static DefinedFlag const * s_visionTrace;

		int basicFlagsDefinedCount; // just simple ones
		Map<Name, DefinedFlag> flags;

		static bool load_mask_from_xml(IO::XML::Node const * _node);

		static void cache_and_update_built_ins();
	};

};

