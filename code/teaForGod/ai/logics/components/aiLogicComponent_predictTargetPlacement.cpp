#include "aiLogicComponent_predictTargetPlacement.h"

#include "..\aiLogic_npcBase.h"

#include "..\..\..\..\framework\module\modulePresence.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

void PredictTargetPlacement::set_projectile_speed(float _speed)
{
	projectileSpeed = _speed;
	predictedForTargetWS.clear();
}

void PredictTargetPlacement::set_max_off_angle(Optional<float> const& _angle)
{
	maxOffAngle = _angle;
	predictedForTargetWS.clear();
}

Vector3 PredictTargetPlacement::predict(Vector3 const& _targetWS, Framework::IModulesOwner* _owner, Framework::RelativeToPresencePlacement const& _target)
{
	if (!inUse)
	{
		return _targetWS;
	}

	if (predictedForTargetWS.is_set() &&
		(predictedForTargetWS.get() - _targetWS).length_squared_2d() < sqr(0.01f))
	{
		// reuse
		return predictedTargetWS;
	}
	
	predictedForTargetWS = _targetWS;
	predictedTargetWS = _targetWS;

	Vector3 ownerLoc = _owner->get_presence()->get_placement().get_translation();
	Vector3 toTargetWS = (predictedTargetWS - ownerLoc);
	float toTargetDist = toTargetWS.length();

	if (projectileSpeed.is_set() && toTargetDist > 3.0f) // no need to predict if closer than that
	{
		Vector3 toTargetDirWS = toTargetDist != 0.0f? toTargetWS / toTargetDist : Vector3::zero;

		// predict the target's placement

		/*
		Vector3 projVelocity = toTargetDirWS * projectileSpeed.get();

		if (addOwnersVelocityForProjectiles)
		{
			if (auto* imo = _params.owner)
			{
				projVelocity += imo->get_presence()->get_velocity_linear();
			}
		}
		*/

		Vector3 targetVelWS = Vector3::zero;
		if (auto* imo = _target.get_target())
		{
			Vector3 targetVelTS = imo->get_presence()->get_velocity_linear();
			{	// use previous placement as it will incorporate base movement as well
				Transform placementTS = imo->get_presence()->get_placement();
				Transform prevPlacementTS = placementTS.to_world(imo->get_presence()->get_prev_placement_offset());
				float prevDeltaTime = imo->get_presence()->get_prev_placement_offset_delta_time();
				if (prevDeltaTime != 0.0f)
				{
					Vector3 offsetLocationTS = placementTS.get_translation() - prevPlacementTS.get_translation();
					targetVelTS = offsetLocationTS / prevDeltaTime;
				}
			}
			targetVelWS = _target.vector_from_target_to_owner(targetVelTS);
		}

		float dist = toTargetDist;
		float distVel = Vector3::dot(targetVelWS, toTargetDirWS);

		// distHit = dist + distVel * t
		// distHit = 0 + projSpeed * t
		// dist + distVel * t = projSpeed * t
		// (projSpeed - distVel) * t = dist
		// t = dist / (projSpeed - distVel)

		float t = dist / max(0.01f, (projectileSpeed.get() - distVel));
		t = clamp(t, 0.0f, 20.0f);

		Vector3 correctedTargetLocWS = predictedTargetWS + targetVelWS * t;

		if (maxOffAngle.is_set())
		{
			float useCorrectedTargetLocWS = 1.0f;

			Rotator3 targetDirRotWS = toTargetDirWS.to_rotator();
			Rotator3 correctedTargetDirRotWS = correctedTargetLocWS.to_rotator();

			Rotator3 diff = (correctedTargetDirRotWS - targetDirRotWS).normal_axes();
			float diffLen = diff.length();
			if (diffLen > maxOffAngle.get())
			{
				useCorrectedTargetLocWS = maxOffAngle.get() / diffLen;
			}

			correctedTargetLocWS = lerp(useCorrectedTargetLocWS, predictedTargetWS, correctedTargetLocWS);
		}

		predictedTargetWS = correctedTargetLocWS;
	}

	return predictedTargetWS;
}
