#include "modulePresenceData.h"

#include "..\..\core\debug\debugRenderer.h"

using namespace Framework;

//

bool PresenceCollisionTrace::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);
	
	priority = _node->get_int_attribute(TXT("priority"), priority);

	return result;
}

//

REGISTER_FOR_FAST_CAST(ModulePresenceData);

ModulePresenceData::ModulePresenceData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
	presenceTraceFlags = Collision::DefinedFlags::get_presence_trace();
	presenceTraceRejectFlags = Collision::DefinedFlags::get_presence_trace_reject();
}

bool ModulePresenceData::read_parameter_from(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("noPresenceTraces"))
	{
		noBasedOnPresenceTraces = true;
		noGravityPresenceTraces = true;
		return true;
	}
	else if (_node->get_name() == TXT("noBasedOnPresenceTraces"))
	{
		noBasedOnPresenceTraces = true;
		return true;
	}
	else if (_node->get_name() == TXT("noGravityPresenceTraces"))
	{
		noGravityPresenceTraces = true;
		return true;
	}
	else if (_node->get_name() == TXT("presenceTraces"))
	{
		bool result = true;
		presenceTraceFlags.apply(_node->get_string_attribute(TXT("traceFlags")));
		presenceTraceRejectFlags.apply(_node->get_string_attribute(TXT("traceRejectFlags")));
		if (_node->first_child_named(TXT("noBasedOn")))
		{
			noBasedOnPresenceTraces = true;
		}
		for_every(collisionTraceNode, _node->children_named(TXT("basedOn")))
		{
			PresenceCollisionTrace collisionTrace(CollisionTraceInSpace::OS);
			if (collisionTrace.load_from_xml(collisionTraceNode))
			{
				basedOnPresenceTraces.push_back(collisionTrace);
			}
			else
			{
				result = false;
			}
		}
		if (_node->first_child_named(TXT("noGravity")))
		{
			noBasedOnPresenceTraces = true;
		}
		for_every(collisionTraceNode, _node->children_named(TXT("gravity")))
		{
			PresenceCollisionTrace collisionTrace(CollisionTraceInSpace::OS);
			if (collisionTrace.load_from_xml(collisionTraceNode))
			{
				gravityPresenceTraces.push_back(collisionTrace);
			}
			else
			{
				result = false;
			}
		}
		return result;
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

bool ModulePresenceData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (basedOnPresenceTraces.is_empty() && !noBasedOnPresenceTraces &&
		gravityPresenceTraces.is_empty() && !noGravityPresenceTraces)
	{
		warn_loading_xml(_node, TXT("no presence traces provided at all and no \"noPresenceTraces\" info provided"));
	}
	else
	{
		if (basedOnPresenceTraces.is_empty() && !noBasedOnPresenceTraces)
		{
			warn_loading_xml(_node, TXT("no \"based on\" presence traces and no \"noBasedOnPresenceTraces\" info provided"));
		}
		if (! basedOnPresenceTraces.is_empty() && noBasedOnPresenceTraces)
		{
			warn_loading_xml(_node, TXT("there are \"based on\" presence traces and there is \"noBasedOnPresenceTraces\" info provided, why?"));
		}

		if (gravityPresenceTraces.is_empty() && !noGravityPresenceTraces)
		{
			warn_loading_xml(_node, TXT("no gravity presence traces and no \"noGravityPresenceTraces\" info provided"));
		}
		if (!gravityPresenceTraces.is_empty() && noGravityPresenceTraces)
		{
			warn_loading_xml(_node, TXT("there are gravity presence traces and there is \"noGravityPresenceTraces\" info provided, why?"));
		}
	}

	return result;
}
