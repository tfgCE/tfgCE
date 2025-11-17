#include "gse_forceCamera.h"

#include "..\..\game\game.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\framework\module\modulePresence.h"

#include "..\..\..\core\random\random.h"
#include "..\..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(wait);
DEFINE_STATIC_NAME(startCameraPlacement);
DEFINE_STATIC_NAME(startCameraFOV);
DEFINE_STATIC_NAME(wasAtObjectVar);
DEFINE_STATIC_NAME(wasAtRelPlacement);

//

bool ForceCamera::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	placement.load_from_xml(_node, TXT("placement"));
	fov.load_from_xml(_node, TXT("fov"));

	inRoom.load_from_xml_attribute_or_child_node(_node, TXT("inRoom"));
	inRoomVar = _node->get_name_attribute_or_from_child(TXT("inRoomVar"), inRoomVar);

	atObjectVar = _node->get_name_attribute_or_from_child(TXT("atObjectVar"), atObjectVar);
	atRelPlacement.load_from_xml(_node, TXT("atRelPlacement"));

	drop = _node->get_bool_attribute_or_from_child_presence(TXT("drop"), drop);

	blend = _node->get_name_attribute_or_from_child(TXT("blend"), blend);
	rotationBlend = _node->get_name_attribute_or_from_child(TXT("rotationBlend"), rotationBlend);
	
	attachToObjectVar = _node->get_name_attribute_or_from_child(TXT("attachToObjectVar"), attachToObjectVar);

	blendTime = _node->get_float_attribute_or_from_child(TXT("blendTime"), blendTime);
	time = _node->get_float_attribute_or_from_child(TXT("time"), time);

	if (time == 0.0f)
	{
		time = max(blendTime, time);
	}

	if (auto const* node = _node->first_child_named(TXT("blendCurve")))
	{
		BezierCurve<BezierCurveTBasedPoint<float>> c;
		c.p0.t = 0.0f;
		c.p0.p = 0.0f;
		c.p3.t = 1.0f;
		c.p3.p = 1.0f;
		c.make_linear();
		c.make_roundly_separated();
		if (c.load_from_xml(node))
		{
			blendCurve = c;
		}
	}

	if (auto const* node = _node->first_child_named(TXT("locationCurve")))
	{
		BezierCurve<BezierCurveTBasedPoint<Vector3>> c;
		c.p0.t = 0.0f;
		c.p0.p = Vector3::zero;
		c.p3.t = 1.0f;
		c.p3.p = Vector3::zero;
		c.make_linear();
		c.make_roundly_separated();
		if (c.load_from_xml(node))
		{
			locationCurve = c;
		}
	}

	if (auto const* node = _node->first_child_named(TXT("orientationCurve")))
	{
		BezierCurve<BezierCurveTBasedPoint<Rotator3>> c;
		c.p0.t = 0.0f;
		c.p0.p = Rotator3::zero;
		c.p3.t = 1.0f;
		c.p3.p = Rotator3::zero;
		c.make_linear();
		c.make_roundly_separated();
		if (c.load_from_xml(node))
		{
			orientationCurve = c;
		}
	}

	offsetPlacementRelativelyToParam = Name::invalid();
	for_every(node, _node->children_named(TXT("offsetPlacementRelatively")))
	{
		offsetPlacementRelativelyToParam = node->get_name_attribute(TXT("toParam"));
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type ForceCamera::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	float& waitVar = _execution.access_variables().access<float>(NAME(wait));
	Transform& startCameraPlacementVar = _execution.access_variables().access<Transform>(NAME(startCameraPlacement));
	float& startCameraFOVVar = _execution.access_variables().access<float>(NAME(startCameraFOV));
	Name& wasAtObjectVar = _execution.access_variables().access<Name>(NAME(wasAtObjectVar));
	Transform& wasAtRelPlacement = _execution.access_variables().access<Transform>(NAME(wasAtRelPlacement));

	float useDeltaTime = 0.0f;
	if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
	{
		waitVar = 0.0f;
	}
	else
	{
		useDeltaTime = TutorialSystem::check_active()? ::System::Core::get_raw_delta_time() : ::System::Core::get_delta_time();
		waitVar += useDeltaTime;
	}

	if (auto* game = Game::get_as<Game>())
	{
		if (drop)
		{
			game->clear_forced_camera();

			wasAtObjectVar = Name::invalid();

			return Framework::GameScript::ScriptExecutionResult::Continue;
		}
		else
		{
			bool inUse = game->is_forced_camera_in_use();
			Transform currentPlacement = game->get_forced_camera_placement();
			Framework::Room * currentInRoom = game->get_forced_camera_in_room();
			float currentFOV = game->get_forced_camera_fov();

			if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
			{
				startCameraPlacementVar = currentPlacement;
				startCameraFOVVar = currentFOV;
			}

			float useNew = blendTime != 0.0f && inUse ? clamp(waitVar / blendTime, 0.0f, 1.0f) : 1.0f;
			float pt = time != 0.0f ? clamp(waitVar / time, 0.0f, 1.0f) : 1.0f;

			Transform newPlacement = currentPlacement;
			if (placement.is_set())
			{
				newPlacement = placement.get();
			}
			float newFOV = currentFOV;
			if (fov.is_set())
			{
				newFOV = fov.get();
			}
			if (locationCurve.is_set())
			{
				float t = 0.0f;
				t = BezierCurveTBasedPoint<Vector3>::find_t_at(pt, locationCurve.get(), nullptr, 0.0001f);
				Vector3 loc = locationCurve.get().calculate_at(t).p;

				newPlacement.set_translation(loc);
			}
			if (orientationCurve.is_set())
			{
				float t = 0.0f;
				t = BezierCurveTBasedPoint<Rotator3>::find_t_at(pt, orientationCurve.get(), nullptr, 0.0001f);
				Rotator3 rot = orientationCurve.get().calculate_at(t).p;

				newPlacement.set_orientation(rot.to_quat());
			}

			float useNewRotation;
			{
				float t = useNew;
				useNew = BlendCurve::apply(blend, t);
				useNewRotation = rotationBlend.is_valid() ? BlendCurve::apply(rotationBlend, t) : useNew;
			}

			if (blendCurve.is_set())
			{
				useNew = BezierCurveTBasedPoint<float>::find_t_at(useNew, blendCurve.get(), nullptr, 0.0001f);
			}

			if (inRoomVar.is_valid())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
				{
					currentInRoom = exPtr->get();
				}
			}

			if (!inRoom.is_empty() && game->get_world())
			{
				for_every_ptr(r, game->get_world()->get_all_rooms())
				{
					if (inRoom.check(r->get_tags()))
					{
						currentInRoom = r;
						break;
					}
				}
			}

			{
				if (offsetPlacementRelativelyToParam.is_valid() && currentInRoom)
				{
					Transform offsetPlacement = currentInRoom->get_variables().get_value<Transform>(offsetPlacementRelativelyToParam, Transform::identity);
					newPlacement = offsetPlacement.to_world(newPlacement);
				}
			}

			Transform useStartPlacement = startCameraPlacementVar;

			if (wasAtObjectVar.is_valid())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(wasAtObjectVar))
				{
					if (auto* wasAtObject = exPtr->get())
					{
						Transform wasAtObjectPlacement = wasAtObject->get_presence()->get_placement().to_world(wasAtRelPlacement);
						useStartPlacement = look_matrix(useStartPlacement.get_translation(), (wasAtObjectPlacement.get_translation() - useStartPlacement.get_translation()).normal(), useStartPlacement.get_axis(Axis::Up)).to_transform();
					}
				}
			}

			if (atObjectVar.is_valid())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(atObjectVar))
				{
					if (auto* atObject = exPtr->get())
					{
						Transform atObjectPlacement = atObject->get_presence()->get_placement().to_world(atRelPlacement.get(Transform::identity));
						newPlacement = look_matrix(newPlacement.get_translation(), (atObjectPlacement.get_translation() - newPlacement.get_translation()).normal(), newPlacement.get_axis(Axis::Up)).to_transform();
					}
				}
			}

			if (useNewRotation != useNew)
			{
				newPlacement.set_translation(useNew < 1.0f ? Vector3::lerp(useNew, useStartPlacement.get_translation(), newPlacement.get_translation()) : newPlacement.get_translation());
				newPlacement.set_orientation(useNewRotation < 1.0f ? Quat::lerp(useNewRotation, useStartPlacement.get_orientation(), newPlacement.get_orientation()) : newPlacement.get_orientation());
			}
			else
			{
				newPlacement = useNew < 1.0f ? Transform::lerp(useNew, useStartPlacement, newPlacement) : newPlacement;
			}
			newFOV = newFOV * useNew + (1.0f - useNew) * startCameraFOVVar;

			Framework::IModulesOwner* attachTo = nullptr;
			
			if (attachToObjectVar.is_valid())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(attachToObjectVar))
				{
					attachTo = exPtr->get();
				}
			}

			game->set_forced_camera(newPlacement, currentInRoom, newFOV, attachTo);
		}
	}

	if (waitVar >= time)
	{
		wasAtObjectVar = atObjectVar;
		wasAtRelPlacement = atRelPlacement.get(Transform::identity);
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}
