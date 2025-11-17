#include "overlayInfoElement.h"

#include "overlayInfoSystem.h"

#include "..\..\core\system\core.h"
#include "..\..\framework\game\game.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define AN_DEBUG_FOLLOWING
#endif

//

using namespace TeaForGodEmperor;
using namespace OverlayInfo;

//

REGISTER_FOR_FAST_CAST(Element);

void Element::deactivate()
{
	activeTarget = 0.0f;
	id = Name::invalid();
}

void Element::advance(OverlayInfo::System const* _system, float _deltaTime)
{
	if (autoDeactivateTimeLeft > 0.0f)
	{
		autoDeactivateTimeLeft = max(0.0f, autoDeactivateTimeLeft - _deltaTime);
		if (autoDeactivateTimeLeft == 0.0f)
		{
			deactivate();
		}
	}

	if (auto* g = Framework::Game::get())
	{
		fadedIn = 1.0f - g->get_fade_out();
	}

	active = blend_to_using_speed_based_on_time(active, activeTarget, 0.05f, _deltaTime);

	pulse = 1.0f;
	if (pulsePeriod != 0.0f && pulseStrength != 0.0f)
	{
		pulse *= 1.0f - pulseStrength * (0.5f + 0.5f * sin_deg(360.0f * ::System::Core::get_timer_mod(pulsePeriod)));
	}

	if (!initialRotation.is_set())
	{
		initialRotation = placement.get_orientation().to_rotator();
	}

	if (!startingCameraPlacement.is_set())
	{
		startingCameraPlacement = _system->get_camera_placement();
		Vector3 fwd = startingCameraPlacement.get().get_axis(Axis::Forward).normal_2d();
		if (fwd.is_almost_zero())
		{
			fwd = Vector3::cross(Vector3::zAxis, startingCameraPlacement.get().get_axis(Axis::Right));
			fwd = fwd.normal_2d();
			if (fwd.is_almost_zero())
			{
				fwd = Vector3::yAxis;
			}
		}
		startingCameraPlacementFlat = look_matrix(startingCameraPlacement.get().get_translation(), startingCameraPlacement.get().get_axis(Axis::Forward).normal_2d(), Vector3::zAxis).to_transform();
	}

	if (!keepPlacement)
	{
		if (rotationRelativity == Relativity::Normal)
		{
			for_count(int, ax, 2)
			{
				auto& followCameraWhat = ax == 0 ? followCameraYaw : followCameraPitch;
				if (followCameraWhat.is_set())
				{
					Rotator3 cameraRotation = _system->get_camera_placement().get_orientation().to_rotator();
					float cameraAngle = ax == 0? cameraRotation.yaw : cameraRotation.pitch;
					float shouldBeAtAngle = cameraAngle + (ax == 0? initialRotation.get().yaw : initialRotation.get().pitch);
					float diff = Rotator3::normalise_axis(shouldBeAtAngle - (ax == 0? calculate_rotation(_system).yaw : calculate_rotation(_system).pitch));

					float changeRequired = 0.0f;
					if (diff > followCameraWhat.get())
					{
						changeRequired += diff - followCameraWhat.get();
					}
					else if (diff < -followCameraWhat.get())
					{
						changeRequired += diff - (-followCameraWhat.get());
					}

					if (changeRequired != 0.0f)
					{
						float changeDone = _deltaTime * 90.0f * (changeRequired > 0.0f ? 1.0f : -1.0f);

						if (changeRequired > 0.0f && changeDone > changeRequired)
						{
							changeDone = changeRequired;
						}
						if (changeRequired < 0.0f && changeDone < changeRequired)
						{
							changeDone = changeRequired;
						}

						Rotator3 pAsRot = placement.get_orientation().to_rotator();
						if (ax == 0)
						{
							pAsRot.yaw += changeDone;
						}
						else
						{
							pAsRot.pitch += changeDone;
						}
						placement.set_orientation(pAsRot.to_quat());
					}
				}
			}
		}

		if (followCameraLocation.is_set() &&
			locationRelativity != Relativity::RelativeToAnchor && locationRelativity != Relativity::RelativeToAnchorPY &&
			locationRelativity != Relativity::RelativeToAnchorHUD && locationRelativity != Relativity::RelativeToAnchorHUDPY &&
			locationRelativity != Relativity::RelativeToVRAnchor)
		{
			Vector3 cameraLocation = _system->get_camera_placement().get_translation();
			Vector3 diff = cameraLocation - calculate_location(_system);

			if (locationRelativity == Relativity::Normal)
			{
				float dist = diff.length();
				if (dist > followCameraLocation.get())
				{
					placement.set_translation(placement.get_translation() + startingCameraPlacementFlat.get().location_to_local(diff.normal() * (dist - followCameraLocation.get())));
				}
			}
			if (locationRelativity == Relativity::RelativeToCameraFlat)
			{
				float dist = diff.z;
				if (dist > followCameraLocation.get())
				{
					Vector3 location = placement.get_translation();
					location.z += (dist - followCameraLocation.get());
					placement.set_translation(location);
				}
				else if (dist < followCameraLocation.get())
				{
					Vector3 location = placement.get_translation();
					location.z += (followCameraLocation.get() - dist);
					placement.set_translation(location);
				}
			}
		}

		if (followCameraLocationXY.is_set() && locationRelativity == Relativity::Normal)
		{
			Vector3 cameraLocation = _system->get_camera_placement().get_translation();
			Vector3 diff = (cameraLocation - calculate_location(_system)) * Vector3(1.0f, 1.0f, 0.0f);

			float dist = diff.length();
			if (dist > followCameraLocationXY.get())
			{
				placement.set_translation(placement.get_translation() + startingCameraPlacementFlat.get().location_to_local(diff.normal() * (dist - followCameraLocationXY.get())) * Vector3(1.0f, 1.0f, 0.0f));
			}
		}

		if (followCameraLocationZ.is_set() &&
			(locationRelativity == Relativity::Normal ||
			 locationRelativity == Relativity::RelativeToCameraFlat))
		{
			Vector3 cameraLocation = _system->get_camera_placement().get_translation();
			Vector3 diff = (cameraLocation - calculate_location(_system));
			float dist = diff.z;
			if (dist > followCameraLocationZ.get())
			{
				Vector3 location = placement.get_translation();
				location.z += (dist - followCameraLocationZ.get());
				placement.set_translation(location);
			}
			else if (dist < -followCameraLocationZ.get())
			{
				Vector3 location = placement.get_translation();
				location.z += (dist - (-followCameraLocationZ.get()));
				placement.set_translation(location);
			}
		}

		finalPlacement = calculate_placement(_system);
	}
}

Rotator3 Element::calculate_rotation(OverlayInfo::System const* _system) const
{
	if (rotationRelativity == Relativity::RelativeToVRAnchor)
	{
		return placement.get_orientation().to_rotator(); // everything is against vr anchor anyway!
	}
	else if (rotationRelativity == Relativity::RelativeToAnchor)
	{
		return _system->get_anchor_placement().get_orientation().to_world(placement.get_orientation()).to_rotator();
	}
	else if (rotationRelativity == Relativity::RelativeToAnchorPY)
	{
		return _system->get_anchor_py_placement().get_orientation().to_world(placement.get_orientation()).to_rotator();
	}
	else if (rotationRelativity == Relativity::RelativeToAnchorHUD)
	{
		return _system->get_anchor_hud_placement().get_orientation().to_world(placement.get_orientation()).to_rotator();
	}
	else if (rotationRelativity == Relativity::RelativeToAnchorHUDPY)
	{
		return _system->get_anchor_hud_py_placement().get_orientation().to_world(placement.get_orientation()).to_rotator();
	}
	else if (rotationRelativity == Relativity::RelativeToCamera)
	{
		return _system->get_camera_placement().get_orientation().to_world(placement.get_orientation()).to_rotator();
	}
	else if (rotationRelativity == Relativity::RelativeToCameraFlat)
	{
		return _system->get_camera_placement().get_orientation().to_rotator() * Rotator3(0.0f, 1.0f, 0.0f) + placement.get_orientation().to_rotator();
	}
	else
	{
		an_assert(rotationRelativity == Relativity::Normal);
		return startingCameraPlacement.get().get_orientation().to_rotator() * Rotator3(0.0f, 1.0f, 0.0f) + placement.get_orientation().to_rotator();
	}
}

Vector3 Element::calculate_location(OverlayInfo::System const* _system) const
{
	if (rotationRelativity == Relativity::RelativeToVRAnchor)
	{
		return placement.get_translation(); // everything is against vr anchor anyway!
	}
	else if (locationRelativity == Relativity::RelativeToAnchor)
	{
		return _system->get_anchor_placement().location_to_world(placement.get_translation());
	}
	else if (locationRelativity == Relativity::RelativeToAnchorPY)
	{
		return _system->get_anchor_py_placement().location_to_world(placement.get_translation());
	}
	else if (locationRelativity == Relativity::RelativeToAnchorHUD)
	{
		return _system->get_anchor_hud_placement().location_to_world(placement.get_translation());
	}
	else if (locationRelativity == Relativity::RelativeToAnchorHUDPY)
	{
		return _system->get_anchor_hud_py_placement().location_to_world(placement.get_translation());
	}
	else if (locationRelativity == Relativity::RelativeToCamera)
	{
		return _system->get_camera_placement().location_to_world(placement.get_translation());
	}
	else if (locationRelativity == Relativity::RelativeToCameraFlat)
	{
		Transform flatCameraPlacement = look_matrix(_system->get_camera_placement().get_translation(), _system->get_camera_placement().get_axis(Axis::Forward).normal_2d(), Vector3::zAxis).to_transform();
		return flatCameraPlacement.location_to_world(placement.get_translation());
	}
	else
	{
		an_assert(locationRelativity == Relativity::Normal);
		return startingCameraPlacementFlat.get().location_to_world(placement.get_translation());
	}
}

Transform Element::calculate_placement(OverlayInfo::System const* _system) const
{
	if (update_placement)
	{
		return update_placement(_system, this);
	}
	Rotator3 r = calculate_rotation(_system);
	Vector3 l = calculate_location(_system);
	l += r.get_forward() * dist;
	return Transform(l, r.to_quat());
}

float Element::calculate_additional_scale(float _distance, Optional<float> const & _useAdditionalScale)
{
	auto* v3d = ::System::Video3D::get();
	float additionalScale = 0.5f * tan_deg(v3d->get_fov() * 0.5f) * _distance;
	return lerp(_useAdditionalScale.get(1.0f), 1.0f, additionalScale);
}

void Element::apply_vr_displacement(Transform const& _vrAnchorDisplacement, bool _force)
{
	if (applyVRAnchorDisplacement || _force)
	{
		if (locationRelativity == Relativity::RelativeToVRAnchor)
		{
			placement.set_translation(_vrAnchorDisplacement.location_to_local(placement.get_translation()));
		}
		if (rotationRelativity == Relativity::RelativeToVRAnchor)
		{
			placement.set_orientation(_vrAnchorDisplacement.to_local(placement.get_orientation()));
		}
	}
}
