#include "ac_rotateRandomly.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"

#include "..\..\..\framework\game\bullshotSystem.h"

#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(rotateRandomly);

// bullshots system allowances
DEFINE_STATIC_NAME_STR(bsFixedRotateRandomly, TXT("fixed rotate randomly"));

//

REGISTER_FOR_FAST_CAST(RotateRandomly);

RotateRandomly::RotateRandomly(RotateRandomlyData const * _data)
: base(_data)
, rotateRandomlyData(_data)
{
	bone = rotateRandomlyData->bone.get();
	if (rotateRandomlyData->moveAtVar.is_set())
	{
		moveAtVar = rotateRandomlyData->moveAtVar.get();
	}
	if (rotateRandomlyData->stopAtVar.is_set())
	{
		stopAtVar = rotateRandomlyData->stopAtVar.get();
	}
	speed = rotateRandomlyData->speed.get();
	acceleration = rotateRandomlyData->acceleration.get();
	turnInterval = rotateRandomlyData->turnInterval.get();
	oneDir = rotateRandomlyData->oneDir.get();
	sound = rotateRandomlyData->sound.get();
}

RotateRandomly::~RotateRandomly()
{
}

void RotateRandomly::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	bone.look_up(get_owner()->get_skeleton()->get_skeleton());
	moveAtVar.look_up<float>(get_owner()->get_owner()->access_variables());
	stopAtVar.look_up<float>(get_owner()->get_owner()->access_variables());
}

void RotateRandomly::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void RotateRandomly::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(rotateRandomly_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid())
	{
		return;
	}

	int const boneIdx = bone.get_index();
	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform boneLS = poseLS->get_bone(boneIdx);

	// time advance
	if (has_just_activated())
	{
		timeToNextTurn = Random::get_float(turnInterval.min, turnInterval.max);
	}

	float const deltaTime = _context.get_delta_time();

	bool shouldStop = false;
	if (moveAtVar.is_valid() && moveAtVar.get<float>() < 0.5f)
	{
		shouldStop = true;
	}
	if (stopAtVar.is_valid() && stopAtVar.get<float>() > 0.5f)
	{
		shouldStop = true;
	}

	if (shouldStop)
	{
		turnLeft = 0.0f;
	}
	else
	{
		if (turnLeft == 0.0f)
		{
			timeToNextTurn -= deltaTime;
			if ((!turnInterval.is_zero() && timeToNextTurn <= 0.0f) ||
				turnInterval.is_zero())
			{
				turnLeft = (float)Random::get_int_from_range(1, 4) * 90.0f - mod(currentYaw, 90.0f);
				if (oneDir == 0)
				{
					turnLeft *= Random::get_chance(0.5f) ? 1.0f : -1.0f;
				}
				else if (oneDir == -1)
				{
					turnLeft = -turnLeft;
				}
				currentTurnSpeed = Random::get_float(speed.min, speed.max);
			}
		}
	}
	float provideAcceleration = currentSpeed == 0.0f ? 0.0f : -sign(currentSpeed) * min(abs(currentSpeed) / deltaTime, acceleration);
	if (turnLeft != 0.0f)
	{
		if (acceleration != 0.0f)
		{
			// s = v'/2*a to slow down
			float willCoverToSlowDownAddedFrame = abs(currentSpeed) * deltaTime // add one extra frame
				+ sqr(currentSpeed) * (0.5f / acceleration);
			if (willCoverToSlowDownAddedFrame > abs(turnLeft) && ! turnInterval.is_zero())
			{
				// slow down
				provideAcceleration = -sign(currentSpeed) * acceleration;
				float nextVelocity = currentSpeed + provideAcceleration * deltaTime;
				if (nextVelocity * currentSpeed < 0.0f)
				{
					// do not overshoot
					provideAcceleration = -currentSpeed / deltaTime;
				}
			}
			else
			{
				provideAcceleration = sign(turnLeft) * acceleration;
			}
		}
	}

	currentSpeed += provideAcceleration * deltaTime;
	currentSpeed = clamp(currentSpeed, -currentTurnSpeed, currentTurnSpeed);

	float willMove = currentSpeed * deltaTime * get_active();
	currentYaw += willMove;
	if (turnInterval.is_zero())
	{
		turnLeft = 1.0f;
	}
	else
	{
		if (abs(willMove) > 0.0f &&
			abs(willMove) >= abs(turnLeft))
		{
			turnLeft = 0.0f;
			timeToNextTurn = Random::get_float(turnInterval.min, turnInterval.max);
		}
		else
		{
			turnLeft -= willMove;
		}
	}

	{
		bool newRotating = currentSpeed != 0.0f && turnLeft != 0.0f && get_active_target() != 0.0f;
		if ((newRotating ^ rotating) &&
			sound.is_valid())
		{
			if (newRotating)
			{
				if (!soundPlayed.is_set())
				{
					if (auto* s = get_owner()->get_owner()->get_sound())
					{
						soundPlayed = s->play_sound(sound);
					}
				}
			}
			else if (soundPlayed.is_set())
			{
				soundPlayed->stop();
				soundPlayed.clear();
			}
		}
		rotating = newRotating;
	}

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_setting_active(NAME(bsFixedRotateRandomly)))
	{
		currentYaw = 180.0f;
	}
#endif

	boneLS.set_orientation(boneLS.get_orientation().to_world(Rotator3(0.0f, currentYaw, 0.0f).to_quat()));

	// write back
	poseLS->access_bones()[boneIdx] = boneLS;
}

void RotateRandomly::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (bone.is_valid())
	{
		dependsOnParentBones.push_back(bone.get_index());
		providesBones.push_back(bone.get_index());
	}
}

//

REGISTER_FOR_FAST_CAST(RotateRandomlyData);

Framework::AppearanceControllerData* RotateRandomlyData::create_data()
{
	return new RotateRandomlyData();
}

void RotateRandomlyData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(rotateRandomly), create_data);
}

RotateRandomlyData::RotateRandomlyData()
{
}

bool RotateRandomlyData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= moveAtVar.load_from_xml(_node, TXT("moveAtVarID"));
	result &= stopAtVar.load_from_xml(_node, TXT("stopAtVarID"));
	result &= speed.load_from_xml(_node, TXT("speed"));
	result &= acceleration.load_from_xml(_node, TXT("acceleration"));
	result &= turnInterval.load_from_xml(_node, TXT("turnInterval"));
	result &= oneDir.load_from_xml(_node, TXT("oneDir"));
	result &= sound.load_from_xml(_node, TXT("sound"));

	return result;
}

bool RotateRandomlyData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* RotateRandomlyData::create_copy() const
{
	RotateRandomlyData* copy = new RotateRandomlyData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* RotateRandomlyData::create_controller()
{
	return new RotateRandomly(this);
}

void RotateRandomlyData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);
	moveAtVar.fill_value_with(_context);
	stopAtVar.fill_value_with(_context);
	oneDir.fill_value_with(_context);
	sound.fill_value_with(_context);
}

void RotateRandomlyData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void RotateRandomlyData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
