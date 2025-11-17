#include "mc_carrier.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
#include "..\..\..\framework\ai\aiLogic.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#endif

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\world\room.h"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

REGISTER_FOR_FAST_CAST(Carrier);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new Carrier(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new CarrierData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & Carrier::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("carrier")), create_module, create_module_data);
}

Carrier::Carrier(Framework::IModulesOwner* _owner)
	: base(_owner)
{
}

Carrier::~Carrier()
{
}

void Carrier::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	carrierData = fast_cast<CarrierData>(_moduleData);
	if (_moduleData)
	{
		atCarryPoint.set_size(carrierData->carryPoints.get_size());
	}
}

bool Carrier::can_carry_more() const
{
	for_every(cp, atCarryPoint)
	{
		if (!cp->is_set())
		{
			return true;
		}
	}
	return false;
}

bool Carrier::carry(Framework::IModulesOwner* _imo)
{
	for_every(cp, atCarryPoint)
	{
		if (!cp->is_set())
		{
#ifdef AN_USE_AI_LOG
			ai_log(get_owner()->get_ai()->get_mind()->get_logic(), TXT("carry \"%S\""), _imo->ai_get_name().to_char());
#endif
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("carrier \"%S\" carry \"%S\""), get_owner()->ai_get_name().to_char(), _imo->ai_get_name().to_char());
#endif
			*cp = _imo;

			auto & carryPoint = carrierData->carryPoints[for_everys_index(cp)];
			if (carryPoint.socket.is_valid())
			{
				_imo->get_presence()->attach_to_socket(get_owner(), carryPoint.socket);
			}
			else
			{
				_imo->get_presence()->attach_to(get_owner());
			}
			if (carryPoint.suspendAI)
			{
				if (auto* ai = _imo->get_ai())
				{
					ai->suspend();
				}
			}
			if (carryPoint.hidden)
			{
				for_every_ptr(a, _imo->get_appearances())
				{
					a->be_visible(false);
				}
			}
			if (carryPoint.carriedVarID.is_valid())
			{
				_imo->access_variables().access<float>(carryPoint.carriedVarID) = 1.0f;
			}
			return true;
		}
	}

	return false;
}

void Carrier::drop(Framework::IModulesOwner* _imo)
{
	for_every(cp, atCarryPoint)
	{
		if (cp->get() == _imo)
		{
#ifdef AN_USE_AI_LOG
			ai_log(get_owner()->get_ai()->get_mind()->get_logic(), TXT("drop \"%S\""), _imo->ai_get_name().to_char());
#endif
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("carrier \"%S\" drop \"%S\""), get_owner()->ai_get_name().to_char(), _imo->ai_get_name().to_char());
#endif
			_imo->get_presence()->detach();
#ifdef AN_ALLOW_EXTENSIVE_LOGS
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("\"%S\" placement \"%S\" %S"),
				_imo->ai_get_name().to_char(),
				_imo->get_presence()->get_in_room()? _imo->get_presence()->get_in_room()->get_name().to_char() : TXT("--"),
				_imo->get_presence()->get_placement().get_translation().to_string().to_char());
#endif

			*cp = nullptr;

			auto & carryPoint = carrierData->carryPoints[for_everys_index(cp)];
			if (carryPoint.suspendAI)
			{
				if (auto* ai = _imo->get_ai())
				{
					ai->resume();
				}
			}
			if (carryPoint.hidden)
			{
				for_every_ptr(a, _imo->get_appearances())
				{
					a->be_visible();
				}
			}
			if (carryPoint.carriedVarID.is_valid())
			{
				_imo->access_variables().access<float>(carryPoint.carriedVarID) = 0.0f;
			}
			if (giveZeroVelocityToDropped)
			{
				_imo->get_presence()->set_velocity_linear(Vector3::zero);
				_imo->get_presence()->set_velocity_rotation(Rotator3::zero);
			}
			return;
		}
	}
}

void Carrier::on_owner_destroy()
{
#ifdef AN_USE_AI_LOG
	ai_log(get_owner()->get_ai()->get_mind()->get_logic(), TXT("owner destroy"));
#endif

	for_every(cp, atCarryPoint)
	{
		if (cp->get())
		{
			drop(cp->get());
		}
	}

	base::on_owner_destroy();
}

//

REGISTER_FOR_FAST_CAST(CarrierData);

CarrierData::CarrierData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

CarrierData::~CarrierData()
{
}

bool CarrierData::read_parameter_from(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	if (_node->get_name() == TXT("carryPoint"))
	{
		CarryPoint cp;
		if (cp.load_from_xml(_node, _lc))
		{
			carryPoints.push_back(cp);
			return true;
		}
		else
		{
			error_loading_xml(_node, TXT("can't load carry point"));
			return false;
		}
	}
	else
	{
		return base::read_parameter_from(_node, _lc);
	}
}

//

bool CarrierData::CarryPoint::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	socket = _node->get_name_attribute(TXT("socket"), socket);

	carriedVarID = _node->get_name_attribute(TXT("carriedVarID"), carriedVarID);

	suspendAI = _node->get_bool_attribute_or_from_child_presence(TXT("suspendAI"), suspendAI);
	hidden = _node->get_bool_attribute_or_from_child_presence(TXT("hidden"), hidden);

	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("no id provided"));

	return result;
}
