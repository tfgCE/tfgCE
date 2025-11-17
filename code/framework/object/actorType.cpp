#include "actorType.h"

#include "actor.h"

#include "..\library\usedLibraryStored.inl"

#include "..\library\library.h"

#include "..\module\modules.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(ActorType);
LIBRARY_STORED_DEFINE_TYPE(ActorType, actorType);

void ActorType::set_defaults()
{
	base::set_defaults();
	individualityFactor = 0.25f;
	ai.provide_default_type(); // actor should always have ai
}

ActorType::ActorType(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

Object* ActorType::create(String const & _name) const
{
	return new Actor(this, _name);
}

String ActorType::create_instance_name() const
{
	// TODO some rules needed
	return get_name().to_string();
}

bool ActorType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (! _node ||
		_node->get_name() != TXT("actorType"))
	{
		//TODO Utils.Log.error("invalid xml node when loading ActorType");
		return false;
	}
	gameplay.provide_default_type(_lc.get_library()->get_game()->get_customisation().library.defaultGameplayModuleTypeForActorType);
	return base::load_from_xml(_node, _lc);
}

