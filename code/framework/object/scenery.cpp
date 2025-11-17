#include "scenery.h"

#include "..\module\moduleAppearance.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(scenery);

//

REGISTER_FOR_FAST_CAST(Scenery);

Name const & Scenery::get_object_type_name() const
{
	return NAME(scenery);
}

Scenery::Scenery(SceneryType const * _sceneryType, String const & _name)
: base(_sceneryType, !_name.is_empty() ? _name : _sceneryType->create_instance_name())
, sceneryType(_sceneryType)
{
	scoped_call_stack_info(TXT("Scenery::Scenery"));
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Scenery [new] o%p"), this);
#endif
}

Scenery::~Scenery()
{
#ifdef DEBUG_WORLD_LIFETIME
	output(TXT("Scenery [deleted] o%p"), this);
#endif
}
