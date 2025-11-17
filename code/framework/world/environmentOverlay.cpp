#include "environmentOverlay.h"

#include "..\library\library.h"
#include "..\library\usedLibraryStored.inl"
#include "..\render\environmentProxy.h"
#include "..\world\room.h"

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(EnvironmentOverlay);
LIBRARY_STORED_DEFINE_TYPE(EnvironmentOverlay, environmentOverlay);

EnvironmentOverlay::EnvironmentOverlay(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
}

bool EnvironmentOverlay::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	additive = _node->get_bool_attribute_or_from_child_presence(TXT("additive"), additive);

	blendUsingRoomVar = _node->get_name_attribute_or_from_child(TXT("blendUsingRoomVar"), blendUsingRoomVar);

	for_every(node, _node->children_named(TXT("params")))
	{
		result &= params.load_from_xml(node, _lc);
	}

	return result;
}

bool EnvironmentOverlay::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= params.prepare_for_game(_library, _pfgContext);

	return result;
}

#ifdef AN_DEVELOPMENT
void EnvironmentOverlay::ready_for_reload()
{
	base::ready_for_reload();

	params.clear();
}
#endif

float EnvironmentOverlay::calculate_power(Room const* _room) const
{
	_room = _room->get_room_with_vars_for_environment_overlays();

	float power = 1.0f;
	if (blendUsingRoomVar.is_valid())
	{
		power = _room->get_value<float>(blendUsingRoomVar, power);
	}

	return power;
}

void EnvironmentOverlay::apply_to(Render::EnvironmentProxy& _ep, Room const* _room) const
{
	float power = calculate_power(_room);

	if (additive)
	{
		for_every(param, params.get_params())
		{
			if (auto* p = _ep.access_params().access_param(param->get_name()))
			{
				p->add(power, *param);
			}
		}
	}
	else
	{
		if (power >= 1.0f)
		{
			_ep.access_params().set_from(params);
		}
		else
		{
			for_every(param, params.get_params())
			{
				if (auto* p = _ep.access_params().access_param(param->get_name()))
				{
					p->lerp_with(power, *param);
				}
			}
		}
	}
}
