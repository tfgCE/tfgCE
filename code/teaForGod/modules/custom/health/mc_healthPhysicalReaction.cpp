#include "mc_healthPhysicalReaction.h"

#include "..\..\..\game\damage.h"

#include "..\..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\module\moduleDataImpl.inl"
#include "..\..\..\..\framework\world\world.h"

using namespace TeaForGodEmperor;
using namespace CustomModules;

//

DEFINE_STATIC_NAME(acceptAdjustedDamage);
DEFINE_STATIC_NAME(acceptUnadjustedDamage);
DEFINE_STATIC_NAME(reactAfterAdjustedDamage);
DEFINE_STATIC_NAME(affectVelocityRotationYawAfterDamage);
DEFINE_STATIC_NAME(affectVelocityRotationYawAfterAdjustedDamage);
DEFINE_STATIC_NAME(affectVelocityRotationYawAfterUnadjustedDamage);
DEFINE_STATIC_NAME(affectVelocityLinearAfterDamage);
DEFINE_STATIC_NAME(affectVelocityLinearAfterAdjustedDamage);
DEFINE_STATIC_NAME(affectVelocityLinearAfterUnadjustedDamage);

// ai message name
DEFINE_STATIC_NAME(rotateOnDamage);

// ai message params
DEFINE_STATIC_NAME(damage);
DEFINE_STATIC_NAME(damageUnadjusted);
DEFINE_STATIC_NAME(damageSource);

//

REGISTER_FOR_FAST_CAST(HealthPhysicalReaction);

static Framework::ModuleCustom* create_module(Framework::IModulesOwner* _owner)
{
	return new HealthPhysicalReaction(_owner);
}

Framework::RegisteredModule<Framework::ModuleCustom> & HealthPhysicalReaction::register_itself()
{
	return Framework::Modules::custom.register_module(String(TXT("healthPhysicalReaction")), create_module, HealthData::create_module_data);
}

HealthPhysicalReaction::HealthPhysicalReaction(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

HealthPhysicalReaction::~HealthPhysicalReaction()
{
}

void HealthPhysicalReaction::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		acceptAdjustedDamage = _moduleData->get_parameter<bool>(this, NAME(acceptAdjustedDamage), acceptAdjustedDamage);
		acceptUnadjustedDamage = _moduleData->get_parameter<bool>(this, NAME(acceptUnadjustedDamage), acceptUnadjustedDamage);
		reactAfterAdjustedDamage = Energy::get_from_module_data(this, _moduleData, NAME(reactAfterAdjustedDamage), reactAfterAdjustedDamage.get(Energy(-1)));
		if (reactAfterAdjustedDamage.get() < Energy(0))
		{
			reactAfterAdjustedDamage.clear();
		}
		affectVelocityRotationYawAfterAdjustedDamage = _moduleData->get_parameter<Range>(this, NAME(affectVelocityRotationYawAfterDamage), affectVelocityRotationYawAfterAdjustedDamage);
		affectVelocityRotationYawAfterAdjustedDamage = _moduleData->get_parameter<Range>(this, NAME(affectVelocityRotationYawAfterAdjustedDamage), affectVelocityRotationYawAfterAdjustedDamage);
		affectVelocityRotationYawAfterUnadjustedDamage = _moduleData->get_parameter<Range>(this, NAME(affectVelocityRotationYawAfterDamage), affectVelocityRotationYawAfterUnadjustedDamage);
		affectVelocityRotationYawAfterUnadjustedDamage = _moduleData->get_parameter<Range>(this, NAME(affectVelocityRotationYawAfterUnadjustedDamage), affectVelocityRotationYawAfterUnadjustedDamage);
		affectVelocityLinearAfterAdjustedDamage = _moduleData->get_parameter<Range3>(this, NAME(affectVelocityLinearAfterDamage), affectVelocityLinearAfterAdjustedDamage);
		affectVelocityLinearAfterAdjustedDamage = _moduleData->get_parameter<Range3>(this, NAME(affectVelocityLinearAfterAdjustedDamage), affectVelocityLinearAfterAdjustedDamage);
		affectVelocityLinearAfterUnadjustedDamage = _moduleData->get_parameter<Range3>(this, NAME(affectVelocityLinearAfterDamage), affectVelocityLinearAfterUnadjustedDamage);
		affectVelocityLinearAfterUnadjustedDamage = _moduleData->get_parameter<Range3>(this, NAME(affectVelocityLinearAfterUnadjustedDamage), affectVelocityLinearAfterUnadjustedDamage);
	}
}

void HealthPhysicalReaction::reset()
{
	base::reset();

	adjustedDamageAccumulated = Energy(0);
}

void HealthPhysicalReaction::initialise()
{
	base::initialise();

	adjustedDamageAccumulated = Energy(0);
}

void HealthPhysicalReaction::on_deal_damage(Damage const & _damage, Damage const & _unadjustedDamage, DamageInfo & _info)
{
	base::on_deal_damage(_damage, _unadjustedDamage, _info);

	if (should_affect_velocity())
	{
		bool adjustedDamage = _damage.damage < _unadjustedDamage.damage.mul(0.9f);

		bool react = false;
		if (adjustedDamage)
		{
			adjustedDamageAccumulated += _unadjustedDamage.damage;
			react = !reactAfterAdjustedDamage.is_set() || adjustedDamageAccumulated >= reactAfterAdjustedDamage.get();
		}
		else
		{
			react = acceptUnadjustedDamage;
		}
		
		if (react)
		{
			adjustedDamageAccumulated = Energy(0);

			if (auto * presence = get_owner()->get_presence())
			{
				{
					auto const & affectVelocityRotationYawAfterDamage = adjustedDamage ? affectVelocityRotationYawAfterAdjustedDamage : affectVelocityRotationYawAfterUnadjustedDamage;

					Rotator3 current = presence->get_velocity_rotation();
					Rotator3 affect = Rotator3::zero;
					affect.yaw = affectVelocityRotationYawAfterDamage.get_at(Random::get_float(0.0f, 1.0f));
					// check whether to reinforce curent dir rotation or choose random
					if (current.yaw >= affect.yaw * 0.5f)
					{
						// remain
					}
					else if (current.yaw <= -affect.yaw * 0.5f)
					{
						affect = -affect;
					}
					else
					{
						affect.yaw *= Random::get_chance(0.5f) ? 1.0f : -1.0f;
					}
					current.yaw = affect.yaw;
					presence->set_velocity_rotation(current);
				}
				{
					auto const & affectVelocityLinearAfterDamage = adjustedDamage ? affectVelocityLinearAfterAdjustedDamage : affectVelocityLinearAfterUnadjustedDamage;

					Vector3 current = presence->get_velocity_linear();
					Vector3 affect = Vector3::zero;
					affect.x = affectVelocityLinearAfterDamage.x.get_at(Random::get_float(0.0f, 1.0f)) * (Random::get_chance(0.5f)? 1.0f : 1.0f);
					affect.y = affectVelocityLinearAfterDamage.y.get_at(Random::get_float(0.0f, 1.0f)) * (Random::get_chance(0.5f)? 1.0f : 1.0f);
					affect.z = affectVelocityLinearAfterDamage.z.get_at(Random::get_float(0.0f, 1.0f));
					
					current += presence->get_placement().vector_to_world(affect);
					presence->set_velocity_linear(current);
				}

				if (Framework::AI::Message* message = presence->get_in_world()->create_ai_message(NAME(rotateOnDamage)))
				{
					message->to_ai_object(get_owner());
					message->access_param(NAME(damage)).access_as<Damage>() = _damage;
					message->access_param(NAME(damageUnadjusted)).access_as<Damage>() = _unadjustedDamage;
					message->access_param(NAME(damageSource)).access_as<SafePtr<Framework::IModulesOwner>>() = _info.source;
				}
			}
		}
	}
}
