#pragma once

#include "..\types\name.h"

#ifdef AN_ALLOW_EXTENDED_DEBUG
	#define IF_EXTENDED_DEBUG(_name) \
		DEFINE_STATIC_NAME(_name); \
		if (ExtendedDebug::should_debug(NAME(_name)))
#else
	#define IF_EXTENDED_DEBUG(_name) \
		if (0)
#endif

class ExtendedDebug
{
public:
	static void initialise_static();
	static void close_static();

	static void do_debug(Name const & _debug);
	static void dont_debug(Name const & _debug);

	static bool should_debug(Name const & _debug);
private:
	ExtendedDebug();

	static ExtendedDebug * s_extendedDebug;

	Array<Name> doDebug;
};
