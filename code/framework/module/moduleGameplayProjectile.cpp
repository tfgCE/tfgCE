#include "moduleGameplayProjectile.h"

#include "modules.h"
#include "moduleDataImpl.inl"

#include "..\appearance\controllers\ac_particlesUtils.h"
#include "..\object\temporaryObject.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
	#ifdef ALLOW_DETAILED_DEBUG
		#define INSPECT_PROJECTILE_LIFETIME
		#define INSPECT_PROJECTILE_HITS
	#endif
#endif

//

using namespace Framework;

//

DEFINE_STATIC_NAME(timeToLive);
DEFINE_STATIC_NAME(startingScale);
DEFINE_STATIC_NAME(scaleBlendTime);
DEFINE_STATIC_NAME(pulseScale);
DEFINE_STATIC_NAME(pulseScalePeriod);

//

REGISTER_FOR_FAST_CAST(ModuleGameplayProjectile);

static ModuleGameplay* create_module(IModulesOwner* _owner)
{
	return new ModuleGameplayProjectile(_owner);
}

RegisteredModule<ModuleGameplay> & ModuleGameplayProjectile::register_itself()
{
	return Modules::gameplay.register_module(String(TXT("projectile")), create_module);
}

ModuleGameplayProjectile::ModuleGameplayProjectile(IModulesOwner* _owner)
: base(_owner)
{
}

ModuleGameplayProjectile::~ModuleGameplayProjectile()
{
}

void ModuleGameplayProjectile::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	if (timeToLive >= 0.0f)
	{
		timeToLive -= _deltaTime;
		if (timeToLive <= 0.0f)
		{
			scaleTarget = 0.0f;
		}
	}

	if (scale != scaleTarget)
	{
		scale = blend_to_using_speed_based_on_time(scale, scaleTarget, scale < scaleTarget? 0.07f : scaleBlendTime, _deltaTime);
	}

	{
		float newActualScale = scale;
		
		if (pulseScalePeriod != 0.0f && pulseScale != 0.0f)
		{
			pulseScaleAtPt += _deltaTime / pulseScalePeriod;
			pulseScaleAtPt = mod(pulseScaleAtPt, 1.0f);
			newActualScale *= (1.0f + sin_deg(360.0f * pulseScaleAtPt) * pulseScale);
		}
		if (actualScale != newActualScale)
		{
			actualScale = newActualScale;
			get_owner()->get_presence()->set_scale(max(0.001f, actualScale));
		}
	}

	if (scale <= 0.0f && scaleTarget <= 0.0f)
	{
#ifdef INSPECT_PROJECTILE_LIFETIME
		output(TXT("[projectile] to'%p cease to exist due to scale"), get_owner());
#endif
		todo_hack(TXT("sometimes they stay for no reason"));
		get_owner()->get_presence()->set_scale(0.001f);
		Framework::ParticlesUtils::desire_to_deactivate(get_owner());
		get_owner()->cease_to_exist(true);
	}

	if (auto* collision = get_owner()->get_collision())
	{
		auto const & collidedWith = collision->get_collided_with();
		for_every(ci, collidedWith)
		{
			if (!process_hit(*ci))
			{
				break;
			}
		}
	}
}

void ModuleGameplayProjectile::reset()
{
	scale = startingScale;
	scaleTarget = 1.0f;
	actualScale = 1.0f;
	pulseScaleAtPt = 0.0f;

	base::reset();
}

void ModuleGameplayProjectile::setup_with(ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	if (_moduleData)
	{
		timeToLive = _moduleData->get_parameter<float>(this, NAME(timeToLive), timeToLive);
		startingScale = _moduleData->get_parameter<float>(this, NAME(startingScale), startingScale);
		scaleBlendTime = _moduleData->get_parameter<float>(this, NAME(scaleBlendTime), scaleBlendTime);
		pulseScale = _moduleData->get_parameter<float>(this, NAME(pulseScale), pulseScale);
		pulseScalePeriod = _moduleData->get_parameter<float>(this, NAME(pulseScalePeriod), pulseScalePeriod);
	}
	scale = startingScale;
}

bool ModuleGameplayProjectile::process_hit(CollisionInfo const & _ci)
{
#ifdef INSPECT_PROJECTILE_LIFETIME
	output(TXT("[projectile] to'%p cease to exist due to hit"), get_owner());
#endif
#ifdef INSPECT_PROJECTILE_HITS
	output(TXT("[projectile] to'%p hit o%p"), get_owner(), _ci.collidedWithObject);
#endif
	todo_hack(TXT("sometimes they stay for no reason"));
	get_owner()->get_presence()->set_scale(0.001f);
	get_owner()->cease_to_exist(true);
	return false;
}
