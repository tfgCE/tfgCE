#include "sceneryType.h"

#include "scenery.h"

#include "..\library\usedLibraryStored.inl"

#include "..\library\library.h"

#include "..\module\modules.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(SceneryType);
LIBRARY_STORED_DEFINE_TYPE(SceneryType, sceneryType);

void SceneryType::set_defaults()
{
	base::set_defaults();
	individualityFactor = 0.25f;
}

SceneryType::SceneryType(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

Object* SceneryType::create(String const & _name) const
{
	return new Scenery(this, _name);
}

String SceneryType::create_instance_name() const
{
	// TODO some rules needed
	return get_name().to_string();
}

bool SceneryType::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	if (! _node ||
		_node->get_name() != TXT("sceneryType"))
	{
		//TODO Utils.Log.error("invalid xml node when loading SceneryType");
		return false;
	}
	gameplay.provide_default_type(_lc.get_library()->get_game()->get_customisation().library.defaultGameplayModuleTypeForSceneryType);

	roomScenery = _node->get_bool_attribute_or_from_child_presence(TXT("roomScenery"), roomScenery);

	bool result = base::load_from_xml(_node, _lc);
	
	return result;
}

