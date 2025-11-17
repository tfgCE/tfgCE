#include "extendedDebug.h"

#include "..\globalSettings.h"

ExtendedDebug * ExtendedDebug::s_extendedDebug = nullptr;

ExtendedDebug::ExtendedDebug()
{
}

void ExtendedDebug::initialise_static()
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	an_assert(s_extendedDebug == nullptr);
	s_extendedDebug = new ExtendedDebug();

#ifdef AN_DEVELOPMENT
	do_debug(Name(TXT("FbxHierarchy")));
	do_debug(Name(TXT("FbxImport")));
#endif
	//do_debug(Name(TXT("Mesh3DExtractor")));
	//do_debug(Name(TXT("WalkersLegMovement")));
	//do_debug(Name(TXT("WalkersLegMovementContinuous")));
#endif
}

void ExtendedDebug::close_static()
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	an_assert(s_extendedDebug != nullptr);
	delete_and_clear(s_extendedDebug);
#endif
}

void ExtendedDebug::do_debug(Name const & _debug)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	an_assert(s_extendedDebug != nullptr);
	s_extendedDebug->doDebug.push_back_unique(_debug);
#endif
}

void ExtendedDebug::dont_debug(Name const & _debug)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	an_assert(s_extendedDebug != nullptr);
	s_extendedDebug->doDebug.remove(_debug);
#endif
}

bool ExtendedDebug::should_debug(Name const & _debug)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	an_assert(s_extendedDebug != nullptr);
	return s_extendedDebug->doDebug.does_contain(_debug);
#else
	return false;
#endif
}
