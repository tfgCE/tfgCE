#include "physicalMaterial.h"

#include "..\modulesOwner\modulesOwner.h"
#include "..\module\modulePresence.h"
#include "..\module\moduleSound.h"

#include "..\..\core\collision\loadingContext.h"

using namespace Framework;

REGISTER_FOR_FAST_CAST(PhysicalMaterial);
LIBRARY_STORED_DEFINE_TYPE(PhysicalMaterial, physicalMaterial);

Map<Name, PhysicalMaterial*>* PhysicalMaterial::s_defaults = nullptr;

PhysicalMaterial::PhysicalMaterial(Library * _library, LibraryName const & _name)
: base(_library, _name)
{
	be<PhysicalMaterial>();
}

PhysicalMaterial::~PhysicalMaterial()
{
	if (s_defaults)
	{
		while (auto * key = s_defaults->does_contain(this))
		{
			s_defaults->remove(*key);
		}
		if (s_defaults->is_empty())
		{
			delete_and_clear(s_defaults);
		}
	}
}

bool PhysicalMaterial::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("default")))
	{
		Name forId = _node->get_name_attribute(TXT("for"));
		if (!s_defaults)
		{
			s_defaults = new Map<Name, PhysicalMaterial*>();
			(*s_defaults)[forId] = this;
		}
	}

	for_every(node, _node->children_named(TXT("collision")))
	{
		for_every(soundNode, node->children_named(TXT("sound")))
		{
			Name id = soundNode->get_name_attribute(TXT("id"));
			if (id.is_valid())
			{
				collisionSounds.push_back(id);
			}
			else
			{
				error_loading_xml(soundNode, TXT("no id provided"));
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("footStep")))
	{
		for_every(soundNode, node->children_named(TXT("sound")))
		{
			Name id = soundNode->get_name_attribute(TXT("id"));
			if (id.is_valid())
			{
				footStepSounds.push_back(id);
			}
			else
			{
				error_loading_xml(soundNode, TXT("no id provided"));
				result = false;
			}
		}
	}

	Collision::LoadingContext loadingContext;
	result &= Collision::PhysicalMaterial::load_from_xml(_node, loadingContext);

	return result;
}

bool PhysicalMaterial::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);
	
	return result;
}

bool PhysicalMaterial::load_from_xml(IO::XML::Node const * _node, Collision::LoadingContext const & _loadingContext)
{
	an_assert(false, TXT("should not be used to load from child"));
	return false;
}

bool PhysicalMaterial::load_from_xml(IO::XML::Attribute const * _attr, Collision::LoadingContext const & _loadingContext)
{
	an_assert(false, TXT("should not be used to load from attribute"));
	return false;
}

void PhysicalMaterial::log_usage_info(LogInfoContext & _context) const
{
	Collision::PhysicalMaterial::log_usage_info(_context);
	LOG_INDENT(_context);
	_context.log(TXT("physical material: %S"), get_name().to_string().to_char());
}

void PhysicalMaterial::play_collision(IModulesOwner * _owner, Room const * _inRoom, Optional<Vector3> const & _location, Optional<float> const & _volume) const
{
	if (auto * ms = _owner->get_sound())
	{
		SoundPlayParams soundPlayParams;
		if (_volume.is_set())
		{
			soundPlayParams.volume = Range(_volume.get());
		}
		for_every(sound, collisionSounds)
		{
			if (_inRoom && _location.is_set())
			{
				if (ms->play_sound_in_room(*sound, _inRoom, Transform(_location.get(), Quat::identity), soundPlayParams))
				{
					// no need to play anymore sounds
					break;
				}
			}
			else
			{
				if (ms->play_sound(*sound, NP, _owner->get_presence()->get_centre_of_presence_os(), soundPlayParams))
				{
					// no need to play anymore sounds
					break;
				}
			}
		}
	}
}

void PhysicalMaterial::play_foot_step(IModulesOwner * _owner, Room const * _inRoom, Transform const & _placement, Optional<float> const & _volume) const
{
	if (auto * ms = _owner->get_sound())
	{
		SoundPlayParams soundPlayParams;
		if (_volume.is_set())
		{
			soundPlayParams.volume = Range(_volume.get());
		}
		for_every(sound, footStepSounds)
		{
			if (ms->play_sound_in_room(*sound, _inRoom, _placement, soundPlayParams))
			{
				// no need to play anymore sounds
				break;
			}
		}
	}
}

PhysicalMaterial * PhysicalMaterial::get(Collision::PhysicalMaterial * _pm, Name const & _for)
{
	if (PhysicalMaterial* pm = cast(_pm))
	{
		return pm;
	}

	return get(_for);
}

PhysicalMaterial const * PhysicalMaterial::get(Collision::PhysicalMaterial const * _pm, Name const & _for)
{
	if (PhysicalMaterial const * pm = cast(_pm))
	{
		return pm;
	}

	return get(_for);
}

PhysicalMaterial * PhysicalMaterial::get(Name const & _for)
{
	if (!s_defaults)
	{
		return nullptr;
	}

	if (auto* pm = s_defaults->get_existing(_for))
	{
		return *pm;
	}
	else
	{
		return nullptr;
	}
}
