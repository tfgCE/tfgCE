#include "aiLogicUtil_shootingAccuracy.h"

#include "..\..\..\game\gameSettings.h"

#include "..\..\..\modules\custom\health\mc_health.h"

#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// vars
DEFINE_STATIC_NAME(shootingAccuracyMaxOffAngle);

//

Vector3 Utils::apply_shooting_accuracy(Vector3 const& _velocity, Framework::IModulesOwner* _shooter, Framework::IModulesOwner* _enemy, Optional<float> const & _enemyDistance, Optional<float> const& _defDispersionAngle)
{
	float shootingAccuracyMaxOffAngle = _shooter->get_variables().get_value<float>(NAME(shootingAccuracyMaxOffAngle), 10.0f);
	float shootingAccuracyMinOffAngle = 0.0f;
	float beToSides = 0.0f;
	if (_enemy)
	{
		if (auto* ai = _enemy->get_ai())
		{
			if (ai->is_controlled_by_player())
			{
				if (auto* h = _enemy->get_custom<CustomModules::Health>())
				{
					float healthPt = h->get_health().div_to_float(h->get_max_health());
					float fullHealth = remap_value(healthPt, 0.5f, 0.9f, 0.0f, 1.0f, true);
					float lowHealth = remap_value(healthPt, 0.6f, 0.2f, 0.0f, 1.0f, true);
					Random::Generator rg;
					if (rg.get_chance(fullHealth * GameSettings::get().difficulty.forceHitChanceOnFullHealth))
					{
						shootingAccuracyMaxOffAngle *= (1.0f - 0.97f * clamp(fullHealth * 1.5f, 0.0f, 1.0f)); // give it still a bit of miss
					}
					if (rg.get_chance(lowHealth * GameSettings::get().difficulty.forceMissChanceOnLowHealth))
					{
						shootingAccuracyMinOffAngle = shootingAccuracyMaxOffAngle * clamp(lowHealth * 3.0f, 0.0f, 1.0f); // miss a lot
						beToSides = lowHealth * 0.8f;
					}
				}
			}
		}
	}
	float dispersionAngleDeg = clamp(atan_deg(0.5f / max(1.0f, _enemyDistance.get(1.0f))), shootingAccuracyMinOffAngle, shootingAccuracyMaxOffAngle);
	float rollAngle = Random::get_float(-180.0f, 180.0f);
	rollAngle = sign(rollAngle) * (90.0f + (abs(rollAngle) - 90.0f) * (1.0f - beToSides));
	Quat dispersionAngle = Rotator3(0.0f, 0.0f, rollAngle).to_quat().to_world(Rotator3(Random::get_float(0.0f, dispersionAngleDeg), 0.0f, 0.0f).to_quat());
	Quat velocityDir = _velocity.to_rotator().to_quat();
	Vector3 velocity = velocityDir.to_world(dispersionAngle).get_y_axis() * _velocity.length();
	return velocity;
}

