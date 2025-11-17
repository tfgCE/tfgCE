#include "modulePortableShield.h"

#include "..\custom\health\mc_health.h"

#include "..\..\..\framework\module\modules.h"

#ifdef AN_CLANG
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(healthDepletionPerSecond);
DEFINE_STATIC_NAME(shieldCoverSocket);
DEFINE_STATIC_NAME(shieldStartColour);
DEFINE_STATIC_NAME(shieldStartLength);
DEFINE_STATIC_NAME(shieldDepletedColour);
DEFINE_STATIC_NAME(shieldDepletedLength);

// shader params
DEFINE_STATIC_NAME(shieldOn);
DEFINE_STATIC_NAME(shieldOverrideColour);

// collision flags
DEFINE_STATIC_NAME(shieldDepleted);

// sounds
DEFINE_STATIC_NAME(start);
DEFINE_STATIC_NAME(loop);
DEFINE_STATIC_NAME(depleted);

//

REGISTER_FOR_FAST_CAST(ModulePortableShield);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModulePortableShield(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModulePortableShieldData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModulePortableShield::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("portable shield")), create_module, create_module_data);
}

ModulePortableShield::ModulePortableShield(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModulePortableShield::~ModulePortableShield()
{
}

void ModulePortableShield::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	healthDepletionPerSecond = Energy::get_from_module_data(this, _moduleData, NAME(healthDepletionPerSecond), healthDepletionPerSecond);
	shieldCoverSocket.set_name(_moduleData->get_parameter<Name>(this, NAME(shieldCoverSocket), shieldCoverSocket.get_name()));
	{
		shieldStartLength = _moduleData->get_parameter<float>(this, NAME(shieldStartLength), shieldStartLength);
		String shieldStartColourString = _moduleData->get_parameter<String>(this, NAME(shieldStartColour));
		if (!shieldStartColourString.is_empty())
		{
			shieldStartColour.parse_from_string(shieldStartColourString);
		}
	}
	{
		shieldDepletedLength = _moduleData->get_parameter<float>(this, NAME(shieldDepletedLength), shieldDepletedLength);
		String shieldDepletedColourString = _moduleData->get_parameter<String>(this, NAME(shieldDepletedColour));
		if (!shieldDepletedColourString.is_empty())
		{
			shieldDepletedColour.parse_from_string(shieldDepletedColourString);
		}
	}
}

void ModulePortableShield::initialise()
{
	base::initialise();

	if (auto * ap = get_owner()->get_appearance())
	{
		shieldCoverSocket.look_up(ap->get_mesh());
	}

	shieldOn = 0.0f;
	shieldOnTarget = 1.0f;
	prevShieldOn = -1.0f;
}

void ModulePortableShield::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	updateShieldOverrideColour = useShieldOverrideColour > 0.0f;
	useShieldOverrideColour = max(0.0f, useShieldOverrideColour - _deltaTime);

	if (auto * h = get_owner()->get_custom<CustomModules::Health>())
	{
		if (!healthDepletionPerSecond.is_zero())
		{
			Energy deplete = healthDepletionPerSecond.timed(_deltaTime, REF_ healthDepletionPerSecondMissingBit);
			EnergyTransferRequest etr(EnergyTransferRequest::Withdraw);
			etr.energyRequested = deplete;
			h->handle_health_energy_transfer_request(etr);
		}
		shieldOnTarget = h->is_alive() ? 1.0f : 0.0f;

		if (!playedStart)
		{
			shieldOverrideColour = shieldStartColour;
			useShieldOverrideColour = shieldStartLength;
			useShieldOverrideColourLength = useShieldOverrideColour;
			prevShieldOverrideColour = -1.0f;
			updateShieldOverrideColour = true;
			if (auto * s = get_owner()->get_sound())
			{
				s->play_sound(NAME(start));
			}
			playedStart = true;
		}

		if (!h->is_alive())
		{
			if (!playedStop)
			{
				if (auto * mc = get_owner()->get_collision())
				{
					mc->push_collision_flags(NAME(shieldDepleted), Collision::Flags::none());
					mc->update_collidable_object();
				}
				shieldOverrideColour = shieldDepletedColour;
				useShieldOverrideColour = shieldDepletedLength;
				useShieldOverrideColourLength = useShieldOverrideColour;
				prevShieldOverrideColour = -1.0f;
				updateShieldOverrideColour = true;
				if (auto * s = get_owner()->get_sound())
				{
					s->stop_sound(NAME(loop));
					s->play_sound(NAME(depleted));
				}
				playedStop = true;
			}
		}
	}

	{
		shieldOn = blend_to_using_speed_based_on_time(shieldOn, shieldOnTarget, 0.2f, _deltaTime);

		if (prevShieldOn != shieldOn ||
			prevShieldOverrideColour != useShieldOverrideColour)
		{
			if (auto * appearance = get_owner()->get_appearance())
			{
				auto& mi = appearance->access_mesh_instance();
				for_count(int, i, mi.get_material_instance_count())
				{
					if (auto * mat = appearance->access_mesh_instance().access_material_instance(i))
					{
						if (auto * spi = mat->get_shader_program_instance())
						{
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldOn));
								if (idx != NONE)
								{
									spi->set_uniform(idx, shieldOn);
								}
							}
							if (updateShieldOverrideColour)
							{
								int idx = spi->get_shader_program()->find_uniform_index(NAME(shieldOverrideColour));
								if (idx != NONE)
								{
									spi->set_uniform(idx, shieldOverrideColour.with_alpha(useShieldOverrideColour / useShieldOverrideColourLength).to_vector4());
								}
							}
						}
					}
				}
			}
		}
		prevShieldOn = shieldOn;
		prevShieldOverrideColour = useShieldOverrideColour;
	}
}

Vector3 ModulePortableShield::get_cover_dir_os() const
{
	if (shieldCoverSocket.is_valid())
	{
		if (auto * ap = get_owner()->get_appearance())
		{
			Transform placement = ap->calculate_socket_os(shieldCoverSocket.get_index());
			return placement.get_axis(Axis::Forward);
		}
	}
	return Vector3::yAxis;
}

//

REGISTER_FOR_FAST_CAST(ModulePortableShieldData);

ModulePortableShieldData::ModulePortableShieldData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModulePortableShieldData::~ModulePortableShieldData()
{
}

