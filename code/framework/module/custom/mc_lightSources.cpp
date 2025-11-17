#include "mc_lightSources.h"

#include "..\modules.h"
#include "..\moduleDataImpl.inl"
#include "..\moduleAppearanceImpl.inl"

#include "..\..\library\library.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\meshGen\meshGenValueDefImpl.inl"
#include "..\..\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace CustomModules;

//

REGISTER_FOR_FAST_CAST(LightSources);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new LightSources(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new LightSourcesData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleCustom> & LightSources::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("lightSources")), create_module, create_module_data);
}

LightSources::LightSources(Framework::IModulesOwner* _owner)
: base(_owner)
{
	SET_EXTRA_DEBUG_INFO(lightSources, TXT("LightSources.lightSources"));
}

LightSources::~LightSources()
{
}

void LightSources::reset()
{
	lightSources.clear();

	base::reset();
}

void LightSources::activate()
{
	base::activate();

	if (lightSourcesData)
	{
		for_every(ls, lightSourcesData->lightSources)
		{
			if (ls->autoPlay)
			{
				add(*ls, ls->explicitFadeOutRequired);
			}
		}
	}

	update(0.0f);
}

void LightSources::initialise()
{
	base::initialise();
}

void LightSources::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	lightSourcesData = fast_cast<LightSourcesData>(_moduleData);
}

void LightSources::update(float _deltaTime)
{
	bool requiresAdvancement = false;

	float maxRadius = 0.0f;

	auto* presence = get_owner()->get_presence();

	if (!lightSources.is_empty())
	{
		Concurrency::ScopedSpinLock lock(lightSourcesLock);

		ARRAY_STACK(int, lightSourcesToRemove, lightSources.get_size());
		for_every(lightSource, lightSources)
		{
			if (lightSource->pulsePeriod.is_set())
			{
				lightSource->pulseTimeLeft -= _deltaTime;
				if (lightSource->pulseTimeLeft <= 0.0f)
				{
					lightSource->pulseTimeLeft = rg.get(lightSource->pulsePeriod.get());
					lightSource->pulseTargetPower = rg.get(lightSource->pulsePower.get(Range(1.0f)));
					lightSource->pulseCurrentBlendTime = rg.get(lightSource->pulseBlendTime.get(Range(0.1f)));
				}
				lightSource->pulseCurrentPower = blend_to_using_time(lightSource->pulseCurrentPower, lightSource->pulseTargetPower, lightSource->pulseCurrentBlendTime, _deltaTime);
				requiresAdvancement = true;
			}
			else
			{
				lightSource->pulseCurrentPower = 1.0f;
			}
			if (lightSource->flickerPeriod.is_set())
			{
				lightSource->flickerTimeLeft -= _deltaTime;
				if (lightSource->flickerTimeLeft <= 0.0f)
				{
					lightSource->flickerTimeLeft = rg.get(lightSource->flickerPeriod.get());
					lightSource->flickerCurrentPower = rg.get(lightSource->flickerPower.get(Range(1.0f)));
					lightSource->flickerCurrentOffset = Vector3(rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f), rg.get_float(-1.0f, 1.0f)) * lightSource->flickerOffset.get(Vector3::zero);
				}
				requiresAdvancement = true;
			}
			else
			{
				lightSource->flickerCurrentPower = 1.0f;
			}
			if (lightSource->getColourFromVar.is_valid())
			{
				// set once, only if provided, if not, ignore
				Colour colour = get_owner()->get_variables().get_value<Colour>(lightSource->getColourFromVar, Colour::none);
				if (!colour.is_none())
				{
					lightSource->requested.colour = colour;
				}
				lightSource->getColourFromVar = Name::invalid();
			}
			if (lightSource->fadeInTime.is_set())
			{
				if (lightSource->fadeInTime.get() > 0.0f)
				{
					lightSource->state = clamp(lightSource->state + _deltaTime / lightSource->fadeInTime.get(), 0.0f, 1.0f);
				}
				else
				{
					lightSource->state = 1.0f;
				}
				requiresAdvancement = true;
				if (lightSource->state >= 1.0f)
				{
					lightSource->fadeInTime.clear();
				}
			}
			else if (lightSource->sustainTime.is_set())
			{
				lightSource->sustainTime = lightSource->sustainTime.get() - _deltaTime;
				if (lightSource->sustainTime.get() <= 0.0f)
				{
					lightSource->sustainTime.clear();
				}
				requiresAdvancement = true;
			}
			else if (lightSource->explicitFadeOutRequired ||
					 lightSource->fadeOutOnParticleDeactivate)
			{
				requiresAdvancement = true;
			}
			else if (lightSource->fadeOutTime.is_set())
			{
				if (lightSource->fadeOutTime.get() > 0.0f)
				{
					lightSource->state = clamp(lightSource->state - _deltaTime / lightSource->fadeOutTime.get(), 0.0f, 1.0f);
				}
				else
				{
					lightSource->state = 0.0f;
				}
				requiresAdvancement = true;
				if (lightSource->state == 0.0f)
				{
					lightSourcesToRemove.push_back(for_everys_index(lightSource));
				}
			}
			if (lightSource->useVelocity.is_set() || 
				lightSource->locationFromSocket.is_valid())
			{
				requiresAdvancement = true;
			}
			lightSource->timeAlive += _deltaTime;
			float useScale = lightSource->useScale.get(1.0f);
			Transform placement(lightSource->requested.location, Quat::identity);
			placement.set_translation(lightSource->useLocation.get(placement.get_translation()) * useScale);
			if (lightSource->locationFromSocket.is_valid())
			{
				if (auto* a = get_owner()->get_appearance())
				{
					Transform socketOS = a->calculate_socket_os(lightSource->locationFromSocket);
					socketOS.set_translation(socketOS.location_to_world(placement.get_translation()));
					placement = socketOS;
				}
			}
			placement.set_translation(placement.location_to_world(lightSource->useVelocity.get(Vector3::zero) * lightSource->timeAlive * useScale));
			lightSource->current = lightSource->requested;
			lightSource->current.power *= lightSource->state;
			lightSource->current.power *= (Framework::LightSource::allowFlickeringLights ? lightSource->flickerCurrentPower : 1.0f);
			lightSource->current.power *= lightSource->pulseCurrentPower;
			if (lightSource->requested.flickeringLight && !Framework::LightSource::allowFlickeringLights)
			{
				lightSource->current.power = 0.0f;
			}
			lightSource->current.distance *= useScale;
			lightSource->current.location = placement.location_to_world(lightSource->flickerCurrentOffset);
			if (presence)
			{
				float radius = (lightSource->current.location - presence->get_centre_of_presence_os().get_translation()).length();
				radius += lightSource->current.distance;
				maxRadius = max(maxRadius, radius);
			}
		}

		for_every_reverse(idx, lightSourcesToRemove)
		{
			lightSources.remove_at(*idx);
		}
	}

	if (presence)
	{
		presence->set_light_based_presence_radius(maxRadius);
	}

	if (requiresAdvancement)
	{
		mark_requires(all_customs__advance_post);
	}
	else
	{
		mark_no_longer_requires(all_customs__advance_post);
	}
}

void LightSources::advance_post(float _deltaTime)
{
	rarer_post_advance_if_not_visible();

	update(_deltaTime);
}

bool LightSources::has_active(Name const& _id) const
{
	Concurrency::ScopedSpinLock lock(lightSourcesLock);

	for_every(ls, lightSources)
	{
		if (ls->id == _id)
		{
			return ls->fadeOutTime.is_set()? false : true;
		}
	}

	return false;
}

bool LightSources::has_any_light_source_active() const
{
	Concurrency::ScopedSpinLock lock(lightSourcesLock);

	return !lightSources.is_empty();
}

NamedLightSource* LightSources::access(Name const& _id)
{
	Concurrency::ScopedSpinLock lock(lightSourcesLock);

	for_every(ls, lightSources)
	{
		if (ls->id == _id)
		{
			return ls;
		}
	}

	return nullptr;
}

void LightSources::mark_requires_update()
{
	mark_requires(all_customs__advance_post);
}

void LightSources::add(Name const& _id, LightSource const& _lightSource, Optional<Name> const& _getColourFromVar,
	Optional<float> const& _fadeInTime, Optional<float> const& _sustainTime, Optional<float> const& _fadeOutTime)
{
	NamedLightSource ls;
	ls.id = _id;
	ls.requested = _lightSource;
	ls.getColourFromVar = _getColourFromVar.get(Name::invalid());
	ls.fadeInTime = _fadeInTime;
	ls.sustainTime= _sustainTime;
	ls.fadeOutTime = _fadeOutTime;
	add(ls);
}

void LightSources::add(Name const& _id, bool _explicitFadeOutRequired)
{
	an_assert(lightSourcesData);
	for_every(ls, lightSourcesData->lightSources)
	{
		if (ls->id == _id)
		{
			add(*ls, _explicitFadeOutRequired);
		}
	}
}

void LightSources::add(NamedLightSource const& _lightSource, bool _explicitFadeOutRequired)
{
	if (_lightSource.disallowOnVar.is_valid() &&
		get_owner()->get_variables().get_value<bool>(_lightSource.disallowOnVar, false))
	{
		return;
	}

	{
		Concurrency::ScopedSpinLock lock(lightSourcesLock);

		bool addNew = true;
		for_every(ls, lightSources)
		{
			if (ls->id == _lightSource.id)
			{
				if (_lightSource.sustainTime.is_set())
				{
					ls->sustainTime = ls->sustainTime.get(0.0f) + _lightSource.sustainTime.get();
				}
				addNew = false;
			}
		}
		if (addNew)
		{
			lightSources.grow_size(1);
			auto& ls = lightSources.get_last();
			ls = _lightSource;
			ls.current = ls.requested;
			ls.current.power = 0.0f; // disable yet
			ls.pulseTimeLeft = 0.0f;
			ls.pulseCurrentPower = 1.0f;
			ls.flickerTimeLeft = 0.0f;
			ls.flickerCurrentPower = 1.0f;
			ls.flickerCurrentOffset = Vector3::zero;
			ls.state = ls.fadeInTime.get(0.0f) == 0.0f ? 1.0f : 0.0f;
			ls.explicitFadeOutRequired = _explicitFadeOutRequired;
			if (auto* v = ls.location.find(get_owner()->get_variables()))
			{
				ls.useLocation = *v;
			}
			if (auto* v = ls.velocity.find(get_owner()->get_variables()))
			{
				ls.useVelocity = *v;
			}
			if (auto* v = ls.scale.find(get_owner()->get_variables()))
			{
				ls.useScale = *v;
			}
		}
	}

	mark_requires(all_customs__advance_post);
}

void LightSources::fade_out(Name const& _id, Optional<float> const& _fadeOutTime)
{
	internal_fade_out(_id, false, _fadeOutTime);
}

void LightSources::fade_out_all_on_particles_deactivation(Optional<float> const& _fadeOutTime)
{
	Concurrency::ScopedSpinLock lock(lightSourcesLock, true);

	for_every(ls, lightSources)
	{
		if (ls->fadeOutOnParticleDeactivate)
		{
			fade_out(ls->id, _fadeOutTime);
		}
	}
}

void LightSources::remove(Name const& _id, Optional<float> const& _fadeOutTime)
{
	internal_fade_out(_id, true, _fadeOutTime);
}

void LightSources::internal_fade_out(Name const& _id, bool _clearId, Optional<float> const& _fadeOutTime)
{
	Concurrency::ScopedSpinLock lock(lightSourcesLock, true);

	for_every(ls, lightSources)
	{
		if (ls->id == _id)
		{
			if (_clearId)
			{
				ls->id = Name::invalid();
			}
			ls->fadeInTime.clear();
			ls->sustainTime.clear();
			ls->explicitFadeOutRequired = false;
			ls->fadeOutOnParticleDeactivate = false;
			if (_fadeOutTime.is_set())
			{
				ls->fadeOutTime = _fadeOutTime;
			}
			if (! ls->fadeOutTime.is_set())
			{
				ls->fadeOutTime = 0.1f;
			}
			mark_requires(all_customs__advance_post);
		}
	}
}

void LightSources::for_every_light_source(std::function<void(LightSource const& _ls)> _do) const
{
	Concurrency::ScopedSpinLock lock(lightSourcesLock);

	for_every(ls, lightSources)
	{
		if (ls->current.power > 0.0f)
		{
			_do(ls->current);
		}
	}
}

//

REGISTER_FOR_FAST_CAST(LightSourcesData);

LightSourcesData::LightSourcesData(LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

bool LightSourcesData::read_parameter_from(IO::XML::Attribute const* _attr, LibraryLoadingContext& _lc)
{
	return base::read_parameter_from(_attr, _lc);
}

bool LightSourcesData::read_parameter_from(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	if (_node->get_name() == TXT("lightSource"))
	{
		// handled in load_from_xml
		return true;
	}
	return base::read_parameter_from(_node, _lc);
}

void LightSourcesData::prepare_to_unload()
{
	base::prepare_to_unload();

	lightSources.clear();
}

bool LightSourcesData::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	for_every(node, _node->children_named(TXT("lightSource")))
	{
		NamedLightSource nls;
		if (nls.load_from_xml(node, _lc))
		{
			lightSources.push_back(nls);
		}
		else
		{
			error_loading_xml(node, TXT("problem loading light source"));
		}
	}

	return result;
}

//

bool NamedLightSource::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	autoPlay = _node->get_bool_attribute_or_from_child_presence(TXT("autoPlay"), autoPlay);

	disallowOnVar = _node->get_name_attribute(TXT("disallowOnVar"), disallowOnVar);

	getColourFromVar = _node->get_name_attribute(TXT("getColourFromVar"), getColourFromVar);

	result &= requested.load_from_xml(_node, _lc);
	location.load_from_xml_child_node(_node, TXT("location"));
	velocity.load_from_xml_child_node(_node, TXT("velocity"));
	scale.load_from_xml_child_node(_node, TXT("scale"));
	locationFromSocket = _node->get_name_attribute(TXT("locationFromSocket"), locationFromSocket);

	explicitFadeOutRequired = _node->get_bool_attribute_or_from_child_presence(TXT("explicitFadeOutRequired"), explicitFadeOutRequired); // for autoplay

	fadeInTime.load_from_xml(_node, TXT("fadeInTime"));
	sustainTime.load_from_xml(_node, TXT("sustainTime"));
	fadeOutTime.load_from_xml(_node, TXT("fadeOutTime"));

	fadeOutOnParticleDeactivate = _node->get_bool_attribute_or_from_child_presence(TXT("fadeOutOnParticleDeactivate"), fadeOutOnParticleDeactivate);

	pulsePeriod.load_from_xml(_node, TXT("pulsePeriod"));
	pulsePower.load_from_xml(_node, TXT("pulsePower"));
	pulseBlendTime.load_from_xml(_node, TXT("pulseBlendTime"));

	flickerPeriod.load_from_xml(_node, TXT("flickerPeriod"));
	flickerPower.load_from_xml(_node, TXT("flickerPower"));
	flickerOffset.load_from_xml(_node, TXT("flickerOffset"));

	return result;
}
