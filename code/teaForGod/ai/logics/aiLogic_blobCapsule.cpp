#include "aiLogic_blobCapsule.h"

#include "..\..\modules\custom\mc_carrier.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleMovementPath.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// timer
DEFINE_STATIC_NAME(dropAndDestroy);

// temporary objects
DEFINE_STATIC_NAME_STR(onImpact, TXT("on impact"));
DEFINE_STATIC_NAME_STR(onStart, TXT("on start"));

// variables/parameters
DEFINE_STATIC_NAME(capsuleSpeed);

// sound
DEFINE_STATIC_NAME(engine);

// poi
DEFINE_STATIC_NAME(blobStart);

//

REGISTER_FOR_FAST_CAST(BlobCapsule);

BlobCapsule::BlobCapsule(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
}

BlobCapsule::~BlobCapsule()
{
}

void BlobCapsule::learn_from(SimpleVariableStorage& _parameters)
{
	base::learn_from(_parameters);
	
	speed = _parameters.get_value<float>(NAME(capsuleSpeed), speed);
}

LATENT_FUNCTION(BlobCapsule::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(float, distance);

	auto * self = fast_cast<BlobCapsule>(logic);
	auto * imo = mind->get_owner_as_modules_owner();

	LATENT_BEGIN_CODE();

	{
		for_every_ptr(a, imo->get_appearances())
		{
			a->be_visible(false);
		}
	}

	LATENT_WAIT(Random::get_float(0.0f, 3.0f));

	// find starting point - random atm
	{
		Transform endPlacement = imo->get_presence()->get_placement();
		Vector3 startingLoc = endPlacement.get_translation() + Vector3(0.0f, 0.0f, 100.0f);
		if (auto* room = imo->get_presence()->get_in_room())
		{
			Framework::PointOfInterestInstance* foundPOI;
			if (room->find_any_point_of_interest(NAME(blobStart), foundPOI))
			{
				startingLoc = foundPOI->calculate_placement().get_translation();
			}
		}

		Optional<Vector3> towards;
		if (auto * room = imo->get_presence()->get_in_room())
		{
			if (AVOID_CALLING_ACK_ room->was_recently_seen_by_player())
			{
				auto & playerPlacement = room->get_recently_seen_by_player_placement();
				if (playerPlacement.is_set())
				{
					Transform cameraPlacement = playerPlacement.get();

					towards = cameraPlacement.get_translation();
				}
			}
		}

		if (towards.is_set())
		{
			// get onto the plane and position towards "towards"
			Vector3 endUp = endPlacement.get_axis(Axis::Z);
			Vector3 towardsInPlane = endPlacement.get_translation() + (towards.get() - endPlacement.get_translation()).drop_using(endUp);
			endPlacement = look_at_matrix(endPlacement.get_translation(), towardsInPlane, endUp).to_transform();
		}
		else
		{
			endPlacement = endPlacement.to_world(Transform(Vector3::zero, Rotator3(0.0f, Random::get_float(-180.0f, 180.0f), 0.0f).to_quat()));
		}

		Vector3 towardsEnd = endPlacement.get_translation() - startingLoc;

		Transform startPlacement = matrix_from_up_right(startingLoc, -towardsEnd.normal(), endPlacement.get_axis(Axis::X)).to_transform();

		BezierCurve<Vector3> curve;
		curve.p0 = startingLoc;
		curve.p3 = endPlacement.get_translation();
		curve.p2 = curve.p3;
		curve.p1 = lerp(0.6f, curve.p0, curve.p3);
		curve.p1.z = lerp(0.2f, curve.p0.z, curve.p3.z);
		curve.p2.z = lerp(0.5f, curve.p0.z, curve.p3.z);

		distance = curve.distance();

		if (auto *mmp = fast_cast<Framework::ModuleMovementPath>(imo->get_movement()))
		{
			mmp->use_curve_single_path(startPlacement, endPlacement, curve, 0.0f);
			mmp->on_curve_single_path_reorient_after(0.995f);
			mmp->on_curve_single_path_align_axis_along_t(Axis::Z, true);
			mmp->set_speed(self->speed);
			mmp->set_acceleration(100000.0f);
		}
	}

	{
		for_every_ptr(a, imo->get_appearances())
		{
			a->be_visible(true);
		}
	}

	if (auto* to = imo->get_temporary_objects())
	{
		to->spawn_all(NAME(onStart));
	}

	if (auto* s = imo->get_sound())
	{
		s->play_sound(NAME(engine));
	}

	while (true)
	{
		if (auto *mmp = fast_cast<Framework::ModuleMovementPath>(imo->get_movement()))
		{
			float t = mmp->get_t();
			float prevT = t;
			t += LATENT_DELTA_TIME * self->speed / distance;
			if (t >= 1.0f)
			{
				t = 1.0f;
				if (prevT < t)
				{
					if (auto* mc = imo->get_custom<CustomModules::Carrier>())
					{
						mc->give_zero_velocity_to_dropped();
					}
					if (auto* to = imo->get_temporary_objects())
					{
						to->spawn_all(NAME(onImpact));
					}
					if (auto* s = imo->get_sound())
					{
						s->stop_looped_sounds();
					}
					// one frame to give chance for particles to appear properly, everything be placed nicely etc
					imo->set_timer(1, NAME(dropAndDestroy),
						[](Framework::IModulesOwner* _imo)
					{
						// will auto drop on destroy
						if (_imo)
						{
							if (auto* o = _imo->get_as_object())
							{
								o->destroy();
							}
						}
					});

				}
			}
			mmp->set_t(t);
		}
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

