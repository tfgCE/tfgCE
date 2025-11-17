#include "utils.h"

#include "teaForGod.h"

#include "game\game.h"

#include "..\framework\modulesOwner\modulesOwner.h"
#include "..\framework\module\moduleAppearance.h"
#include "..\framework\module\moduleAppearanceImpl.inl"
#include "..\framework\module\modulePresence.h"
#include "..\framework\object\object.h"
#include "..\framework\world\door.h"
#include "..\framework\world\doorType.h"
#include "..\framework\world\doorInRoom.h"
#include "..\framework\world\room.h"
#include "..\framework\world\environment.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
#define LOG_SET_EMISSIVE_FROM_ENVIRONMENT
#endif

//

using namespace TeaForGodEmperor;

//

DEFINE_STATIC_NAME(lightDir);
DEFINE_STATIC_NAME(sun);
DEFINE_STATIC_NAME(lightModulateColourOnto);
DEFINE_STATIC_NAME(roomEmissiveColour); // can't share emissiveColour as it will override whatever is set in the object
DEFINE_STATIC_NAME(roomEmissivePower); // as above

// room variables
DEFINE_STATIC_NAME(setEmissiveFromEnvironment);
DEFINE_STATIC_NAME(allowEmissiveFromEnvironment);
DEFINE_STATIC_NAME(zeroEmissive);

// appearance uniforms
DEFINE_STATIC_NAME(emissiveColour); // we use emissive directly, without emissive controller!
DEFINE_STATIC_NAME(emissiveBaseColour);
DEFINE_STATIC_NAME(emissivePower);

// material tags
DEFINE_STATIC_NAME(ignoreSetEmissiveFromEnvironment);

//

bool is_material_valid_for_set_emissive_from_environment(::System::MaterialInstance const* mat)
{
	an_assert(mat);
	return !mat->get_material()->get_tags().get_tag(NAME(ignoreSetEmissiveFromEnvironment));
}

//

Vector3 Utils::get_light_dir(Framework::IModulesOwner const* _owner)
{
	if (auto* presence = _owner->get_presence())
	{
		if (auto* room = presence->get_in_room())
		{
			if (auto* environment = room->get_environment())
			{
				if (auto * lightDirParam = environment->get_param(NAME(lightDir)))
				{
					return lightDirParam->as<Vector3>();
				}
			}
		}
	}
	return Vector3::zero;
}

Colour Utils::get_light_colour(Framework::IModulesOwner const* _owner)
{
	if (auto* presence = _owner->get_presence())
	{
		return get_light_colour(presence->get_in_room());
	}
	return Colour::none;
}

Colour Utils::get_light_colour(Framework::Room const* _room)
{
	if (_room)
	{
		if (auto const * environment = _room->get_environment())
		{
			if (auto * lightModulateColourOnto = environment->get_param(NAME(lightModulateColourOnto)))
			{
				return lightModulateColourOnto->as<Colour>();
			}
		}
	}
	return Colour::none;
}

bool Utils::get_emissive_colour(Framework::IModulesOwner const* _owner, REF_ Colour& _colour)
{
	if (auto* presence = _owner->get_presence())
	{
		return get_emissive_colour(presence->get_in_room(), _colour);
	}
	return false;
}

bool Utils::get_emissive_power(Framework::IModulesOwner const* _owner, REF_ float& _power)
{
	if (auto* presence = _owner->get_presence())
	{
		return get_emissive_power(presence->get_in_room(), _power);
	}
	return false;
}

bool Utils::get_emissive_colour(Framework::Room const* _room, REF_ Colour & _colour)
{
	if (_room)
	{
		if (auto const * environment = _room->get_environment())
		{
			if (auto * emissiveColour = environment->get_param(NAME(roomEmissiveColour)))
			{
				_colour = emissiveColour->as<Colour>();
				return true;
			}
		}
	}
	return false;
}

bool Utils::get_emissive_power(Framework::Room const* _room, REF_ float & _power)
{
	if (_room)
	{
		if (auto const * environment = _room->get_environment())
		{
			if (auto * emissivePower = environment->get_param(NAME(roomEmissivePower)))
			{
				_power = emissivePower->as<float>();
				return true;
			}
		}
	}
	return false;
}

float Utils::get_sun(Framework::IModulesOwner const* _owner)
{
	if (auto* presence = _owner->get_presence())
	{
		if (auto* room = presence->get_in_room())
		{
			if (auto* environment = room->get_environment())
			{
				if (auto * sunParam = environment->get_param(NAME(sun)))
				{
					return sunParam->as<float>();
				}
			}
		}
	}
	return 1.0f;
}

void Utils::calculate_projectile_emissive_colour_base(Colour& projectileEmissiveColour, Colour& projectileEmissiveBaseColour)
{
	if (!projectileEmissiveColour.is_none())
	{
		if (projectileEmissiveBaseColour.is_none())
		{
			projectileEmissiveBaseColour = projectileEmissiveColour;
			float minV = min(projectileEmissiveBaseColour.r, min(projectileEmissiveBaseColour.g, projectileEmissiveBaseColour.b));
			float maxV = max(projectileEmissiveBaseColour.r, max(projectileEmissiveBaseColour.g, projectileEmissiveBaseColour.b));
			//
			float newMaxV = 1.0f + (1.0f - maxV) * 0.25f;
			float min2max = maxV - minV;
			float min2NewMax = newMaxV - minV;
			projectileEmissiveBaseColour = projectileEmissiveColour;
			float keepOriginal = 0.7f;
			if (maxV > minV)
			{
				Colour pt;
				pt.r = (projectileEmissiveColour.r - minV) / min2max;
				pt.g = (projectileEmissiveColour.g - minV) / min2max;
				pt.b = (projectileEmissiveColour.b - minV) / min2max;
				pt.r = 1.0f + keepOriginal * (pt.r - 1.0f);
				pt.g = 1.0f + keepOriginal * (pt.g - 1.0f);
				pt.b = 1.0f + keepOriginal * (pt.b - 1.0f);
				projectileEmissiveColour.r = minV + pt.r * min2NewMax;
				projectileEmissiveColour.g = minV + pt.g * min2NewMax;
				projectileEmissiveColour.b = minV + pt.b * min2NewMax;
			}
			else
			{
				projectileEmissiveColour.r = projectileEmissiveColour.r + keepOriginal * (1.0f - projectileEmissiveColour.r);
				projectileEmissiveColour.g = projectileEmissiveColour.g + keepOriginal * (1.0f - projectileEmissiveColour.g);
				projectileEmissiveColour.b = projectileEmissiveColour.b + keepOriginal * (1.0f - projectileEmissiveColour.b);
			}
		}
		projectileEmissiveColour.a = 0.0f;
		projectileEmissiveBaseColour.a = 1.0f;
	}
}

void Utils::get_projectile_emissives_from_appearance(Framework::ModuleAppearance const* ap, OUT_ Colour& projectileEmissiveColour, OUT_ Colour& projectileEmissiveBaseColour)
{
	// extract emissive colour if not provided
	if (projectileEmissiveColour.is_none())
	{
		projectileEmissiveBaseColour = Colour::none;
		if (auto* mat = ap->get_mesh_instance().get_material_instance(0))
		{
			if (auto* spi = mat->get_shader_program_instance())
			{
				if (auto* sp = spi->get_shader_param(NAME(emissiveColour)))
				{
					if (sp->is_set())
					{
						projectileEmissiveColour = sp->get_as_colour();
					}
				}
				if (auto* sp = spi->get_shader_param(NAME(emissiveBaseColour)))
				{
					if (sp->is_set())
					{
						projectileEmissiveBaseColour = sp->get_as_colour();
					}
				}
			}
		}
	}

}

void Utils::set_emissives_from_room_to(Framework::IModulesOwner* imo, Framework::Room* _room)
{
	if (auto* o = imo->get_as_object())
	{
		if (! o->get_tags().get_tag(NAME(setEmissiveFromEnvironment)))
		{
			return;
		}
	}
	else
	{
		return;
	}

	if (! _room)
	{
		_room = imo->get_presence()->get_in_room();
	}
	if (_room && !_room->get_environment())
	{
		// no environment set yet, we might be during creation of the map
		SafePtr<Framework::IModulesOwner> keepIMO;
		SafePtr<Framework::Room> keepRoom;
		keepIMO = imo;
		keepRoom = _room;
#ifdef LOG_SET_EMISSIVE_FROM_ENVIRONMENT
		output(TXT("[setEmissiveFromEnvironment] postponing set for object o%p \"%S\" in \"%S\""), imo, imo->ai_get_name().to_char(), _room->get_name().to_char());
#endif
		Game::get()->add_async_world_job(TXT("set emissive from to object"), [keepIMO, keepRoom]()
			{
				auto* imo = keepIMO.get();
				auto* room = keepRoom.get();
#ifdef LOG_SET_EMISSIVE_FROM_ENVIRONMENT
				output(TXT("[setEmissiveFromEnvironment] postponed! for object o%p r%p"), imo, room);
#endif
				if (imo && room)
				{
					set_emissives_from_room_to(imo, room);
				}
			});
		return;
	}

	if (_room)
	{
		if (_room->get_value<bool>(NAME(allowEmissiveFromEnvironment), DEFAULT_ALLOW_EMISSIVE_FROM_ENVIRONMENT))
		{
			Colour colour = Utils::get_light_colour(_room);
			Utils::get_emissive_colour(_room, REF_ colour);
			float power = colour.a;
			Utils::get_emissive_power(_room, REF_ power);
#ifdef LOG_SET_EMISSIVE_FROM_ENVIRONMENT
			output(TXT("[setEmissiveFromEnvironment] to object %S [%.3f] \"%S\" in \"%S\""), colour.to_string().to_char(), power,
				imo->ai_get_name().to_char(),
				_room->get_name().to_char());
#endif
			for_every_ptr(appearance, imo->get_appearances())
			{
				appearance->do_for_all_appearances([colour, power](Framework::ModuleAppearance* _appearance)
				{
					_appearance->set_shader_param(NAME(emissiveColour), colour.with_alpha(1.0f).to_vector4(), [](::System::MaterialInstance const* mat) { return is_material_valid_for_set_emissive_from_environment(mat); });
					_appearance->set_shader_param(NAME(emissivePower), power, [](::System::MaterialInstance const* mat) { return is_material_valid_for_set_emissive_from_environment(mat); });
				});
			}
		}
		if (_room->get_value<bool>(NAME(zeroEmissive), DEFAULT_ZERO_EMISSIVE_FROM_ENVIRONMENT))
		{
			for_every_ptr(instance, _room->get_appearance().get_instances())
			{
				for_count(int, i, instance->get_material_instance_count())
				{
					if (auto* mi = instance->get_material_instance(i))
					{
						if (is_material_valid_for_set_emissive_from_environment(mi))
						{
							mi->set_uniform(NAME(emissivePower), 0.0f);
						}
					}
				}
			}
		}
	}
	else
	{
		warn_dev_investigate(TXT("no room to set emissives from, use after an object is placed or provide a room"));
	}
}

void Utils::set_emissives_from_room_to(Framework::DoorInRoom* dir, Framework::Room* _room)
{
	if (!dir->get_tags().get_tag(NAME(setEmissiveFromEnvironment)) &&
		!dir->get_door()->get_tags().get_tag(NAME(setEmissiveFromEnvironment)))
	{
		return;
	}

	if (! _room)
	{
		_room = dir->get_in_room();
	}
	if (_room)
	{
		if (_room->get_value<bool>(NAME(allowEmissiveFromEnvironment), DEFAULT_ALLOW_EMISSIVE_FROM_ENVIRONMENT))
		{
			Colour colour = Utils::get_light_colour(_room);
			Utils::get_emissive_colour(_room, REF_ colour);
			float power = colour.a;
			Utils::get_emissive_power(_room, REF_ power);
#ifdef LOG_SET_EMISSIVE_FROM_ENVIRONMENT
			output(TXT("[setEmissiveFromEnvironment] to door in room %S [%.3f] \"%S\" in \"%S\""), colour.to_string().to_char(), power,
				dir->get_door() && dir->get_door()->get_door_type()?
				dir->get_door()->get_door_type()->get_name().to_string().to_char() : TXT("--"),
				_room->get_name().to_char());
#endif
			for_every_ptr(instance, dir->access_mesh().access_instances())
			{
				for_count(int, i, instance->get_material_instance_count())
				{
					if (auto* mat = instance->get_material_instance(i))
					{
						if (is_material_valid_for_set_emissive_from_environment(mat))
						{
							mat->set_uniform(NAME(emissiveColour), colour.with_alpha(1.0f).to_vector4());
							mat->set_uniform(NAME(emissivePower), power);
						}
					}
				}
			}
		}
		if (_room->get_value<bool>(NAME(zeroEmissive), DEFAULT_ZERO_EMISSIVE_FROM_ENVIRONMENT))
		{
			for_every_ptr(instance, dir->access_mesh().access_instances())
			{
				for_count(int, i, instance->get_material_instance_count())
				{
					if (auto* mat = instance->get_material_instance(i))
					{
						if (is_material_valid_for_set_emissive_from_environment(mat))
						{
							mat->set_uniform(NAME(emissivePower), 0.0f);
						}
					}
				}
			}
		}
	}
}

void Utils::force_blend_emissives_for_room(Framework::Room* _room, Optional<Colour> const & _emissiveColour, Optional<Colour> const & _emissiveBaseColour, Optional<float> const & _emissivePower, float _blendAmount)
{
	for_every_ptr(instance, _room->access_appearance_for_rendering().access_instances())
	{
		for_count(int, i, instance->get_material_instance_count())
		{
			if (auto* mi = instance->get_material_instance(i))
			{
				if (_emissiveColour.is_set())
				{
					Colour tv = _emissiveColour.get().with_alpha(1.0f);
					Colour cv = tv;
					bool blendSet = _blendAmount == 1.0f;
					if (auto* spi = mi->get_shader_program_instance())
					{
						if (auto* sp = spi->get_shader_param(NAME(emissiveColour)))
						{
							if (sp->is_set())
							{
								cv = sp->get_as_colour();
								blendSet = true;
							}
						}
					}
					if (blendSet)
					{
						mi->set_uniform(NAME(emissiveColour), lerp(_blendAmount, cv, tv).to_vector4());
					}
				}
				if (_emissiveBaseColour.is_set())
				{
					Colour tv = _emissiveBaseColour.get().with_alpha(1.0f);
					Colour cv = tv;
					bool blendSet = _blendAmount == 1.0f;
					if (auto* spi = mi->get_shader_program_instance())
					{
						if (auto* sp = spi->get_shader_param(NAME(emissiveBaseColour)))
						{
							if (sp->is_set())
							{
								cv = sp->get_as_colour();
								blendSet = true;
							}
						}
					}
					if (blendSet)
					{
						mi->set_uniform(NAME(emissiveBaseColour), lerp(_blendAmount, cv, tv).to_vector4());
					}
				}
				if (_emissivePower.is_set())
				{
					float tv = _emissivePower.get();
					float cv = tv;
					bool blendSet = _blendAmount == 1.0f;
					if (auto* spi = mi->get_shader_program_instance())
					{
						if (auto* sp = spi->get_shader_param(NAME(emissivePower)))
						{
							if (sp->is_set())
							{
								cv = sp->get_as_float();
								blendSet = true;
							}
						}
					}
					if (blendSet)
					{
						mi->set_uniform(NAME(emissivePower), lerp(_blendAmount, cv, tv));
					}
				}
			}
		}
	}
}

void Utils::set_emissives_for_room_and_all_inside(Framework::Room* _room)
{
	if (_room->get_variables().get_value(NAME(setEmissiveFromEnvironment), DEFAULT_SET_EMISSIVE_FROM_ENVIRONMENT))
	{
		Colour colour = Utils::get_light_colour(_room);
		Utils::get_emissive_colour(_room, REF_ colour);
		float power = colour.a;
		Utils::get_emissive_power(_room, REF_ power);
#ifdef LOG_SET_EMISSIVE_FROM_ENVIRONMENT
		output(TXT("[setEmissiveFromEnvironment] to room %S [%.3f] \"%S\""), colour.to_string().to_char(), power, 
			_room->get_name().to_char());
#endif
		for_every_ptr(instance, _room->access_appearance_for_rendering().access_instances())
		{
			for_count(int, i, instance->get_material_instance_count())
			{
				if (auto* mi = instance->get_material_instance(i))
				{
					if (is_material_valid_for_set_emissive_from_environment(mi))
					{
						mi->set_uniform(NAME(emissiveColour), colour.with_alpha(1.0f).to_vector4());
						mi->set_uniform(NAME(emissivePower), power);
					}
				}
			}
		}
	}
	if (_room->get_variables().get_value(NAME(zeroEmissive), DEFAULT_ZERO_EMISSIVE_FROM_ENVIRONMENT))
	{
		for_every_ptr(instance, _room->access_appearance_for_rendering().access_instances())
		{
			for_count(int, i, instance->get_material_instance_count())
			{
				if (auto* mi = instance->get_material_instance(i))
				{
					if (is_material_valid_for_set_emissive_from_environment(mi))
					{
						mi->set_uniform(NAME(emissivePower), 0.0f);
					}
				}
			}
		}
	}

	for_every_ptr(obj, _room->get_all_objects())
	{
		set_emissives_from_room_to(obj, _room);
	}

	for_every_ptr(dir, _room->get_all_doors())
	{
		set_emissives_from_room_to(dir, _room);
	}
}

void Utils::split_multi_lines(REF_ List<String>& _list)
{
	for_every(l, _list)
	{
		while (l->does_contain('~'))
		{
			int at = l->find_first_of('~');
			String addBefore = l->get_left(at);
			String prefix;
			if (!addBefore.is_empty() && addBefore[0] == '#')
			{
				int at = addBefore.find_first_of('#', 1);
				if (at != NONE)
				{
					prefix = addBefore.get_left(at + 1);
				}
			}
			_list.insert(for_everys_iterator(l), addBefore);
			*l = prefix + l->get_sub(at + 1);
		}
	}
}
