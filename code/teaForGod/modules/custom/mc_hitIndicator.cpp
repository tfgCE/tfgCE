#include "mc_hitIndicator.h"

#include "health\mc_health.h"

#include "..\..\game\game.h"
#include "..\..\game\playerSetup.h"

#include "..\..\..\core\containers\arrayStack.h"

#include "..\..\..\framework\ai\aiMindExecution.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearanceImpl.inl"
#include "..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

// params
DEFINE_STATIC_NAME(appearanceName);
DEFINE_STATIC_NAME(length);
DEFINE_STATIC_NAME(time);
DEFINE_STATIC_NAME(showHealth);
DEFINE_STATIC_NAME(showHitImpulse);

// ai message name
DEFINE_STATIC_NAME(dealtDamage);

// ai message param
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageDamagerProjectile);
DEFINE_STATIC_NAME(damageInstigator);
DEFINE_STATIC_NAME(hitIndicatorRelativeLocation);
DEFINE_STATIC_NAME(hitIndicatorRelativeDir);

// material params
DEFINE_STATIC_NAME(hits);
DEFINE_STATIC_NAME(hit0);
DEFINE_STATIC_NAME(hit1);
DEFINE_STATIC_NAME(hit2);
DEFINE_STATIC_NAME(hit3);
DEFINE_STATIC_NAME(hit4);
DEFINE_STATIC_NAME(hit5);
DEFINE_STATIC_NAME(hit6);
DEFINE_STATIC_NAME(hit7);
DEFINE_STATIC_NAME(hitCount);
DEFINE_STATIC_NAME(healthPt);
DEFINE_STATIC_NAME(hitPt);
DEFINE_STATIC_NAME(healthPulse);

//

REGISTER_FOR_FAST_CAST(HitIndicator);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new HitIndicator(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & HitIndicator::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("hitIndicator")), create_module);
}

HitIndicator::HitIndicator(Framework::IModulesOwner* _owner)
: base(_owner)
{
}

HitIndicator::~HitIndicator()
{
	stop_observing_all_presence();
}

void HitIndicator::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		appearanceName = _moduleData->get_parameter<Name>(this, NAME(appearanceName), appearanceName);
		length = _moduleData->get_parameter<float>(this, NAME(length), length);
		time = _moduleData->get_parameter<float>(this, NAME(time), time);
		showHealth = _moduleData->get_parameter<bool>(this, NAME(showHealth), showHealth);
		showHitImpulse = _moduleData->get_parameter<bool>(this, NAME(showHitImpulse), showHitImpulse);
	}
}

void HitIndicator::reset()
{
	base::reset();

	mark_requires(all_customs__advance_post);

	stop_observing_all_presence();

	hits.clear();
}

void HitIndicator::initialise()
{
	base::initialise();

	mark_requires(all_customs__advance_post);

	stop_observing_all_presence();

	hits.clear();
}

void HitIndicator::handle_message(Framework::AI::Message const & _message)
{
	an_assert(_message.get_name() == NAME(dealtDamage));
	if (auto * damageInstigator = _message.get_param(NAME(damageInstigator)))
	{
		if (damageInstigator->get_as<SafePtr<Framework::IModulesOwner>>().get() == get_owner()->get_top_instigator())
		{
			// do not indicate own hits with hit indicators, unless projectile
			bool ignore = true;
			if (auto* damageDamagerProjectile = _message.get_param(NAME(damageDamagerProjectile)))
			{
				if (damageDamagerProjectile->get_as<bool>())
				{
					ignore = false;
				}
			}
			if (ignore)
			{
				return;
			}
		}
	}
	float damagePt = 0.5f;
	if (auto * damage = _message.get_param(NAME(damage)))
	{
		Energy dmg = damage->get_as<Damage>().damage;
		if (auto* owner = get_owner())
		{
			if (auto* h = owner->get_custom<CustomModules::Health>())
			{
				damagePt = dmg.div_to_float(h->get_max_health());
			}
		}
	}

	if (auto* hitIndicatorRelativeDirParam = _message.get_param(NAME(hitIndicatorRelativeDir)))
	{
		indicate_relative_dir(damagePt, hitIndicatorRelativeDirParam->get_as<Vector3>());
	}
	else if (auto* hitIndicatorRelativeLocationParam = _message.get_param(NAME(hitIndicatorRelativeLocation)))
	{
		indicate_relative_location(damagePt, hitIndicatorRelativeLocationParam->get_as<Vector3>());
	}
}

void HitIndicator::make_hit_being_relative(Hit& hit, Framework::IModulesOwner* _tryBeingRelativeTo)
{
	if (_tryBeingRelativeTo)
	{
		if (auto* p = _tryBeingRelativeTo->get_presence())
		{
			if (p->get_in_room() == get_owner()->get_presence()->get_in_room())
			{
				hit.relativeToPresence = p;
				if (p != get_owner()->get_presence())
				{
					hit.relativeToPresence->add_presence_observer(this);
				}
				if (hit.loc.is_set())
				{
					hit.locRelative = hit.relativeToPresence->get_placement().location_to_local(hit.loc.get());
				}
				if (hit.dir.is_set())
				{
					hit.dirRelative = hit.relativeToPresence->get_placement().vector_to_local(hit.dir.get());
				}
			}
		}
	}
}

static float scale_damage_pt_to_hit_impulse(float _pt)
{
	_pt = clamp(_pt / 0.5f, 0.0f, 1.0f); // should scale 0%-50% of max health to have full intensity
	return lerp(_pt, 0.6f, 1.0f); // to make it noticeable
}

static float scale_damage_pt_to_hit_impulse_sustain(float _pt)
{
	return lerp(_pt, 0.3f, 2.0f);
}

void HitIndicator::indicate_relative_location(float _damagePt, Vector3 const & _locationRel, IndicateParams const& _params)
{
	Hit hit;
	hit.loc = get_owner()->get_presence()->get_placement().location_to_world(_locationRel);
	make_hit_being_relative(hit, _params.tryBeingRelativeTo.get(nullptr));
	hit.colourOverride = _params.colourOverride.get(Colour::none);
	hit.delay = _params.delay.get(0.0f);
	hit.reversed = _params.reversed.get(false);
	Concurrency::ScopedSpinLock lock(hitsLock);
	hits.push_back(hit);
	if (_params.damage.get(true))
	{
		hitImpulseTarget = max(hitImpulseTarget, scale_damage_pt_to_hit_impulse(_damagePt));
		hitImpulseSustain = max(hitImpulseSustain, scale_damage_pt_to_hit_impulse_sustain(_damagePt));
	}
}

void HitIndicator::indicate_location(float _damagePt, Vector3 const & _locationWS, IndicateParams const& _params)
{
	Hit hit;
	hit.loc = _locationWS;
	make_hit_being_relative(hit, _params.tryBeingRelativeTo.get(nullptr));
	hit.colourOverride = _params.colourOverride.get(Colour::none);
	hit.delay = _params.delay.get(0.0f);
	hit.reversed = _params.reversed.get(false);
	Concurrency::ScopedSpinLock lock(hitsLock);
	hits.push_back(hit);
	if (_params.damage.get(true))
	{
		hitImpulseTarget = max(hitImpulseTarget, scale_damage_pt_to_hit_impulse(_damagePt));
		hitImpulseSustain = max(hitImpulseSustain, scale_damage_pt_to_hit_impulse_sustain(_damagePt));
	}
}

void HitIndicator::indicate_relative_dir(float _damagePt, Vector3 const & _relativeDir, IndicateParams const& _params)
{
	Hit hit;
	hit.dir = get_owner()->get_presence()->get_placement().vector_to_world(_relativeDir).normal();
	make_hit_being_relative(hit, _params.tryBeingRelativeTo.get(nullptr));
	hit.colourOverride = _params.colourOverride.get(Colour::none);
	hit.delay = _params.delay.get(0.0f);
	hit.reversed = _params.reversed.get(false);
	Concurrency::ScopedSpinLock lock(hitsLock);
	hits.push_back(hit);
	if (_params.damage.get(true))
	{
		hitImpulseTarget = max(hitImpulseTarget, scale_damage_pt_to_hit_impulse(_damagePt));
		hitImpulseSustain = max(hitImpulseSustain, scale_damage_pt_to_hit_impulse_sustain(_damagePt));
	}
}

void HitIndicator::on_presence_changed_room(Framework::ModulePresence* _presence, Framework::Room * _intoRoom, Transform const & _intoRoomTransform, Framework::DoorInRoom* _exitThrough, Framework::DoorInRoom* _enterThrough, Framework::DoorInRoomArrayPtr const * _throughDoors)
{
	Concurrency::ScopedSpinLock lock(hitsLock);
	for_every(hit, hits)
	{
		if (hit->relativeToPresence)
		{
			// loc and dir should be set with last valid values
			hit->locRelative.clear();
			hit->dirRelative.clear();
			// and stop observing
			if (hit->relativeToPresence != get_owner()->get_presence())
			{
				hit->relativeToPresence->remove_presence_observer(this);
			}
			hit->relativeToPresence = nullptr;
		}
		if (observingPresence == _presence)
		{
			if (hit->loc.is_set())
			{
				hit->loc = _intoRoomTransform.location_to_local(hit->loc.get());
			}
			if (hit->dir.is_set())
			{
				hit->dir = _intoRoomTransform.vector_to_local(hit->dir.get());
			}
		}
	}
}

void HitIndicator::on_presence_removed_from_room(Framework::ModulePresence* _presence, Framework::Room* _room)
{
	Concurrency::ScopedSpinLock lock(hitsLock);
	if (observingPresence == _presence)
	{
		for_every(hit, hits)
		{
			if (hit->relativeToPresence)
			{
				if (hit->relativeToPresence != get_owner()->get_presence())
				{
					hit->relativeToPresence->remove_presence_observer(this);
				}
				hit->relativeToPresence = nullptr;
			}
		}
		hits.clear();
	}
	for_every(hit, hits)
	{
		if (hit->relativeToPresence == _presence)
		{
			// loc and dir should be set with last valid values
			hit->locRelative.clear();
			hit->dirRelative.clear();
			// and stop observing
			if (hit->relativeToPresence != get_owner()->get_presence())
			{
				hit->relativeToPresence->remove_presence_observer(this);
			}
			hit->relativeToPresence = nullptr;
		}
	}
}

void HitIndicator::on_presence_destroy(Framework::ModulePresence* _presence)
{
	if (observingPresence == _presence)
	{
		stop_observing_all_presence();
		return;
	}
	Concurrency::ScopedSpinLock lock(hitsLock);
	for_every(hit, hits)
	{
		if (hit->relativeToPresence == _presence)
		{
			// loc and dir should be set with last valid values
			hit->locRelative.clear();
			hit->dirRelative.clear();
			// and stop observing
			if (hit->relativeToPresence != get_owner()->get_presence())
			{
				hit->relativeToPresence->remove_presence_observer(this);
			}
			hit->relativeToPresence = nullptr;
		}
	}
}

void HitIndicator::stop_observing_all_presence()
{
	if (observingPresence)
	{
		observingPresence->remove_presence_observer(this);
		observingPresence = nullptr;
	}
	Concurrency::ScopedSpinLock lock(hitsLock);
	for_every(hit, hits)
	{
		if (hit->relativeToPresence)
		{
			// loc and dir should be set with last valid values
			hit->locRelative.clear();
			hit->dirRelative.clear();
			// and stop observing
			if (hit->relativeToPresence != get_owner()->get_presence())
			{
				hit->relativeToPresence->remove_presence_observer(this);
			}
			hit->relativeToPresence = nullptr;
		}
	}
}

void HitIndicator::advance_post(float _deltaTime)
{
	if (!observingPresence)
	{
		if ((observingPresence = get_owner()->get_presence()))
		{
			observingPresence->add_presence_observer(this);
		}
	}

	if (auto * ai = get_owner()->get_ai())
	{
		if (auto * aiMind = ai->get_mind())
		{
			if (mind != aiMind)
			{
				mind = aiMind;
				mind->access_execution().register_message_handler(NAME(dealtDamage), this);
			}
		}
	}

	/*
	if (hits.is_empty())
	{
		Hit hit;
		hit.dir = Vector3(Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f), Random::get_float(-1.0f, 1.0f)).normal();
		hit.dir = -Vector3::yAxis;
		hits.push_back(hit);
	}
	*/

	{
		Concurrency::ScopedSpinLock lock(hitsLock);
		if (!hits.is_empty())
		{
			ARRAY_STACK(int, removeIdx, hits.get_size());
			for_every(hit, hits)
			{
				if (hit->delay > 0.0f)
				{
					hit->delay -= _deltaTime;
				}
				else
				{
					hit->at += _deltaTime / time;
					if (hit->at > 1.0f)
					{
						removeIdx.push_back(for_everys_index(hit));
					}
				}
			}
			for_every_reverse(remove, removeIdx)
			{
				if (auto* p = hits[*remove].relativeToPresence)
				{
					if (p != get_owner()->get_presence())
					{
						p->remove_presence_observer(this);
					}
				}
				hits.remove_fast_at(*remove);
			}
		}
	}

	float healthPt = 1.0f;
	if (auto* h = get_owner()->get_custom<CustomModules::Health>())
	{
		healthPt = Game::calculate_health_pt_for_effects(h);
		healthPt = sqr(healthPt); // so it's more visible
	}

	if (auto* ps = PlayerSetup::get_current_if_exists())
	{
		if (ps->get_preferences().hitIndicatorType == HitIndicatorType::Off)
		{
			Concurrency::ScopedSpinLock lock(hitsLock);

			for_every(hit, hits)
			{
				if (hit->relativeToPresence)
				{
					if (hit->relativeToPresence != get_owner()->get_presence())
					{
						hit->relativeToPresence->remove_presence_observer(this);
					}
					hit->relativeToPresence = nullptr;
				}
			}
			hits.clear();
		}
		if (!ps->get_preferences().healthIndicator || ! showHealth)
		{
			healthPt = 1.0f;
		}
		else
		{
			healthPt = 1.0f - (1.0f - healthPt) * clamp(ps->get_preferences().healthIndicatorIntensity/* * 0.5f*/, 0.0f, 1.0f);
		}
	}

	float hitPt = 0.0f;
	float const hitAtIndicatorLength = 0.02f;
	float const hitAtIndicatorLengthInv = 1.0f / hitAtIndicatorLength;
	{
		Concurrency::ScopedSpinLock lock(hitsLock);
		for_every(hit, hits)
		{
			if (hit->delay <= 0.0f)
			{
				float const useAt = hit->reversed ? 1.0f - hit->at : hit->at;
				hitPt = max(hitPt, clamp(6.0f * (1.0f - useAt * hitAtIndicatorLengthInv), 0.0f, 1.0f));
			}
		}
	}

	{
		float healthPulseSpeed = 0.25f + 0.75f * (1.0f - healthPt);
		healthPulsePt = mod(healthPulsePt + healthPulseSpeed * _deltaTime, 1.0f);
	}

	{
		if (!PlayerPreferences::are_currently_flickering_lights_allowed() || ! showHitImpulse)
		{
			hitImpulseTarget = 0.0f;
			hitImpulse = 0.0f;
		}
		if (hitImpulse < hitImpulseTarget)
		{
			hitImpulse = blend_to_using_speed_based_on_time(hitImpulse, hitImpulseTarget, 0.05f, _deltaTime);
		}
		else
		{
			hitImpulseTarget = 0.0f;
			hitImpulseSustain = max(0.0f, hitImpulseSustain - _deltaTime);
			if (hitImpulseSustain <= 0.0f)
			{
				hitImpulse = blend_to_using_time(hitImpulse, hitImpulseTarget, 0.6f, _deltaTime);
			}
		}
	}

	float const healthPulse = 0.5f + 0.5f * sin_deg(360.0f * healthPulsePt);

	globalDesaturate = Colour::none;
	globalDesaturateChangeInto = Colour::none;

	if (auto* appearance = get_owner()->get_appearance_named(appearanceName))
	{
		Concurrency::ScopedSpinLock lock(hitsLock);

		if (hits.is_empty() && healthPt == 1.0f && hitPt == 0.0f && hitImpulse == 0.0f)
		{
			appearance->be_visible(false);
		}
		else
		{
			appearance->be_visible(true);

			auto & meshInstance = appearance->access_mesh_instance();
			auto * materialInstance = meshInstance.access_material_instance(0);
			an_assert(materialInstance);

			ARRAY_STACK(Matrix44, matrices, hits.get_size());
			for_every(hit, hits)
			{
				Matrix44 hitAsMatrix;
				Vector3 locCameraSpace = Vector3::zero;
				if (hit->relativeToPresence)
				{
					if (hit->locRelative.is_set())
					{
						hit->loc = hit->relativeToPresence->get_placement().location_to_world(hit->locRelative.get());
					}
					if (hit->dirRelative.is_set())
					{
						hit->dir = hit->relativeToPresence->get_placement().vector_to_world(hit->dirRelative.get());
					}
				}
				if (auto* p = get_owner()->get_presence())
				{
					Transform camera = p->get_placement().to_world(p->get_eyes_relative_look());
					if (hit->loc.is_set())
					{
						locCameraSpace = camera.location_to_local(hit->loc.get());
					}
					if (hit->dir.is_set())
					{
						locCameraSpace = camera.vector_to_local(-hit->dir.get() * 10000.0f);
					}
				}
				hitAsMatrix.m00 = locCameraSpace.x;
				hitAsMatrix.m01 = locCameraSpace.y;
				hitAsMatrix.m02 = locCameraSpace.z;
				/*
				 *		0---------------1
				 *		eC  s			|
				 *		|	  e	C s		|
				 *		|			e  Cs
				 *	atNDS uses cosinus ti calculate at in normalised-dot-space
				 *	startNDS and endNDS travel with correctedAt to calculate lengthNDS that can be used in normalised-dot-space)
				 */
				float const useAt = hit->reversed? 1.0f - hit->at : hit->at;
				float atNDS = 0.5f - 0.5f * cos_deg(useAt * 180.0f);
				float startNDS = 0.5f - 0.5f * cos_deg(useAt * (1.0f - length) * 180.0f);
				float endNDS = 0.5f - 0.5f * cos_deg((length + (useAt * (1.0f - length))) * 180.0f);
				float lengthNDS = endNDS - startNDS;
				// now calculate start and end in dot-space but start has to start at 1.0 and end bellow -1.0 (by lengthNDS)
				float startDS = 1.0f + 2.0f * (-atNDS - atNDS * lengthNDS);
				float endDS = 1.0f + 2.0f * (-atNDS + (1.0f - atNDS) * lengthNDS);
				float lengthDS = endDS - startDS;
				hitAsMatrix.m10 = startDS;
				hitAsMatrix.m11 = endDS;
				hitAsMatrix.m20 = 1.0f / (lengthDS * 0.1f);
				hitAsMatrix.m21 = 1.0f / lengthDS;
				float power = 1.0f;
				if (hit->delay > 0.0f)
				{
					power = 0.0f;
				}
				else
				{
					if (auto* ps = PlayerSetup::get_current_if_exists())
					{
					if (ps->get_preferences().hitIndicatorType == HitIndicatorType::NotAtHitSource)
						{
							float threshold = 0.02f;
							power *= clamp((atNDS - threshold) / (1.0f - threshold), 0.0f, 1.0f);
						}
						if (ps->get_preferences().hitIndicatorType == HitIndicatorType::StrongerTowardsHitSource)
						{
							float threshold = 0.9f;
							power *= 1.0f - atNDS * threshold;
						}
						if (ps->get_preferences().hitIndicatorType == HitIndicatorType::WeakerTowardsHitSource)
						{
							float threshold = 0.9f;
							power *= 1.0f - threshold + atNDS * threshold;
						}
						if (ps->get_preferences().hitIndicatorType == HitIndicatorType::SidesOnly)
						{
							float threshold = 0.9f;
							power *= 1.0f - threshold + atNDS * threshold;
							power *= 1.0f - atNDS * threshold;
							power = clamp(power * 2.5f, 0.0f, 1.0f);
						}
					}
					{	// start and end
						float threshold = 0.015f;
						power *= clamp(atNDS / threshold, 0.0f, 1.0f);
						power *= clamp((1.0f - atNDS) / threshold, 0.0f, 1.0f);
					}
				}
				hitAsMatrix.m13 = power;
				hitAsMatrix.m30 = hit->colourOverride.r;
				hitAsMatrix.m31 = hit->colourOverride.g;
				hitAsMatrix.m32 = hit->colourOverride.b;
				hitAsMatrix.m33 = hit->colourOverride.a;
				matrices.push_back(hitAsMatrix);
			}
			if (matrices.get_size() > 0)
			{
				if (::System::Video3D::get()->has_limited_support_for_indexing_in_shaders())
				{
					if (matrices.get_size() > 0) materialInstance->set_uniform(NAME(hit0), matrices[0]);
					if (matrices.get_size() > 1) materialInstance->set_uniform(NAME(hit1), matrices[1]);
					if (matrices.get_size() > 2) materialInstance->set_uniform(NAME(hit2), matrices[2]);
					if (matrices.get_size() > 3) materialInstance->set_uniform(NAME(hit3), matrices[3]);
					if (matrices.get_size() > 4) materialInstance->set_uniform(NAME(hit4), matrices[4]);
					if (matrices.get_size() > 5) materialInstance->set_uniform(NAME(hit5), matrices[5]);
					if (matrices.get_size() > 6) materialInstance->set_uniform(NAME(hit6), matrices[6]);
					if (matrices.get_size() > 7) materialInstance->set_uniform(NAME(hit7), matrices[7]);
				}
				else
				{
					materialInstance->set_uniform(NAME(hits), matrices.get_data(), matrices.get_size());
				}
			}
			materialInstance->set_uniform(NAME(hitCount), matrices.get_size());
			// no healthPt, use global desaturate
			materialInstance->set_uniform(NAME(hitPt), hitPt);
			materialInstance->set_uniform(NAME(healthPulse), healthPulse);

			// desaturate based
			{
				float useHitImpulse = hitImpulse;
				if (auto* ps = PlayerSetup::get_current_if_exists())
				{
					useHitImpulse *= ps->get_preferences().hitIndicatorIntensity;
				}
				float indHealth = 1.0f - healthPt;
				Colour keepColour = hardcoded Colour(1.40f, 0.10f, 0.07f); // more red! although this is now ignored by the current shader, it does
				Colour changeIntoColour = Colour::lerp(useHitImpulse, hardcoded Colour(0.25f, 0.45f, 1.20f), hardcoded Colour(1.50f, 0.05f, 0.05f));
				globalDesaturate = Colour(1.0f, 1.0f, 1.0f) - keepColour; // the colour itself is not used right now
				globalDesaturate.a = clamp(2.5f * indHealth, 0.0f, 1.0f);
				globalDesaturate.a *= (1.0f - lerp(healthPt, 0.4f, 0.7f) * healthPulse);
				globalDesaturate.a = lerp(useHitImpulse, globalDesaturate.a, 1.0f);
				globalDesaturateChangeInto = changeIntoColour;
				globalDesaturateChangeInto.a = 0.0f;
				if (PlayerPreferences::are_currently_flickering_lights_allowed())
				{
					globalDesaturateChangeInto.a += 0.8f * clamp(4.0f * (indHealth - 0.25f), 0.0f, 1.0f); // h < 25%
					globalDesaturateChangeInto.a += 0.5f * 2.0f * max(0.0f, indHealth - 0.5f); // h < 50%
					globalDesaturateChangeInto.a += 0.4f * max(0.0f, indHealth - 0.9f); // h < 90%
					globalDesaturateChangeInto.a *= 0.6f;
					globalDesaturateChangeInto.a = clamp(globalDesaturateChangeInto.a, 0.0f, 1.0f);
					globalDesaturateChangeInto.a = lerp(useHitImpulse, globalDesaturateChangeInto.a, 1.0f);
				}
			}
		}
	}
}
