#include "ac_blinds.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"

#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(blinds);

//

REGISTER_FOR_FAST_CAST(Blinds);

Blinds::Blinds(BlindsData const * _data)
	: base(_data)
	, blindsData(_data)
{
	// setup everything on initialise
}

Blinds::~Blinds()
{
}

struct TempRailCorner
{
	Vector3 location;
	float cornerRadius;

	// out / auto

	float offset = 0.0f; // due to corner radius

	Vector3 toPrev = Vector3::zero;
	Vector3 toNext = Vector3::zero;
};

void Blinds::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	speed = blindsData->speed.get();
	rotateAngle = blindsData->rotateAngle.get();
	blindLength = blindsData->blindLength.get();
	blindThickness = blindsData->blindThickness.get();

	inOpenVar = blindsData->inOpenVar.get();
	if (blindsData->outOpenVar.is_set())
	{
		outOpenVar = blindsData->outOpenVar.get();
	}

	blindsData->startSound.update_if_set(startSound);
	blindsData->loopSound.update_if_set(loopSound);
	blindsData->stopSound.update_if_set(stopSound);
	
	auto* skeleton = get_owner()->get_skeleton()->get_skeleton();

	relativeToBone = blindsData->relativeToBone.get();
	relativeToBone.look_up(skeleton);

	// prepare blinds
	blinds.set_size(blindsData->blinds.get_size());
	for_every(blind, blinds)
	{
		auto const & source = blindsData->blinds[for_everys_index(blind)];
		blind->bone = source.bone.get();
		blind->bone.look_up(skeleton);
		if (source.mirrored.is_set())
		{
			blind->mirrored = source.mirrored.get();
			blind->mirrored.look_up(skeleton);
		}
	}

	// prepare temp rail corners
	Array<TempRailCorner> tempRailCorners;
	tempRailCorners.make_space_for(blindsData->rails.get_size());
	for_every(rail, blindsData->rails)
	{
		TempRailCorner trc;
		trc.location = rail->location.get();
		trc.cornerRadius = rail->radius.get();
		tempRailCorners.push_back(trc);
	}

	if (tempRailCorners.get_size() > 1)
	{
		tempRailCorners[0].toNext = (tempRailCorners[1].location - tempRailCorners[0].location).normal();
		int l = tempRailCorners.get_size() - 1;
		tempRailCorners[l].toPrev = (tempRailCorners[l - 1].location - tempRailCorners[l].location).normal();
	}

	// calculate offsets for them
	int predictedSegmentCount = tempRailCorners.get_size() - 1;
	for_range(int, i, 1, tempRailCorners.get_size() - 2)
	{
		auto & prev = tempRailCorners[i - 1];
		auto & curr = tempRailCorners[i];
		auto & next = tempRailCorners[i + 1];

		curr.toPrev = (prev.location - curr.location).normal();
		curr.toNext = (next.location - curr.location).normal();

		if (curr.cornerRadius == 0.0f)
		{
			curr.offset = 0.0f;
		}
		else
		{
			// calculate angle between curves at the point where they meet
			float const cosAngle = clamp(Vector3::dot(curr.toPrev, curr.toNext), -1.0f, 1.0f);
			if (cosAngle > -0.99f)
			{
				// cosAngle takes -1 if tangents are aligned in same direction, 1 if they are in opposite, note "-prevTangent"!
				float const angle = acos_deg(cosAngle);
				float const hangle = angle * 0.5f;
				/*
				*				A			A centre of round corner
				*			   /|\			B point at prev edge (prev)
				*			r / | \			C point at next/current edge (edge)
				*			 /  |  \		D point where one edge ends and other begins
				*			B_  |  _C		<DBA (and <DCA) are 90'
				*			 x`-D-`x		x is distance from D where normal curve should end
				*			prev edge		<CDB is our calculated angle (angle)
				*							<ADB is half of our angle (hangle)
				*
				*							therefore:
				*							tg(hangle) = r / x
				*							x = r / tg(hangle)
				*/

				curr.offset = curr.cornerRadius / max(SMALL_MARGIN, tan_deg(hangle));
				
				++predictedSegmentCount;
			}
			else
			{
				curr.offset = 0.0f;
			}
		}
	}

	// prepare segments (calculate total length)
	TempRailCorner* prevTRC = nullptr;
	segments.clear();
	segments.make_space_for(predictedSegmentCount);
	totalLength = 0.0f;
	for_every(trc, tempRailCorners)
	{
		if (prevTRC)
		{
			if (prevTRC->offset > 0.0f)
			{
				// add curve
				Segment segment;
				segment.curve.p0 = prevTRC->location + prevTRC->toPrev * prevTRC->offset;
				segment.curve.p1 = prevTRC->location;
				segment.curve.p2 = prevTRC->location;
				segment.curve.p3 = prevTRC->location + prevTRC->toNext * prevTRC->offset;
				segment.curve.make_roundly_separated();

				segment.length = segment.curve.length();
				totalLength += segment.length;

				// on which side we should go to get centre
				Vector3 toCentre = prevTRC->toPrev.rotated_right();
				if (Vector3::dot(toCentre, prevTRC->toNext) < 0.0f)
				{
					toCentre = -toCentre;
				}
				segment.centre = segment.curve.p0 + toCentre * prevTRC->cornerRadius;
				segment.radius = prevTRC->cornerRadius;

				segments.push_back(segment);
			}

			{	// straight segment
				Segment segment;
				segment.curve.p0 = prevTRC->location + prevTRC->toNext * prevTRC->offset;
				segment.curve.p3 = trc->location + trc->toPrev * trc->offset;
				segment.curve.make_linear();
				
				segment.length = segment.curve.length();
				totalLength += segment.length;

				segment.radius = 0.0f;

				segments.push_back(segment);
			}
		}
		else
		{
			an_assert(trc->offset == 0.0f);
		}

		prevTRC = trc;
	}
	if (!segments.is_empty())
	{
		postSegmentsTangent = (segments.get_last().curve.p3 - segments.get_last().curve.p2).normal();
	}

	// other
	rotatedSeparation = rotateAngle != 0.0f ? blindThickness / tan_deg(rotateAngle) : 0.0f;

	// setup blinds now (where do they stop)
	{
		float takenSpace = 0.0f;
		for_every_reverse(blind, blinds)
		{
			blind->startAtDist = (float)(for_everys_index(blind)) * blindLength;
			blind->stopAtDist = totalLength - takenSpace;
			takenSpace += rotatedSeparation;
		}
	}

	float blindsLength = (float)blinds.get_size() * blindLength;

	if (blindsData->openDist.is_set())
	{
		openDist = blindsData->openDist.get();
	}
	else
	{
		openDist = totalLength - (float)(blinds.get_size() - 1) * rotatedSeparation;
	}

	if (blindsData->rotateDist.is_set())
	{
		rotateDist = blindsData->rotateDist.get();
	}
	else
	{
		rotateDist = blindsLength <= totalLength - openDist ? 0.0f : totalLength * 0.2f;
	}

	if (blindsData->additionalSeparationDist.is_set())
	{
		additionalSeparationDist = blindsData->additionalSeparationDist.get();
	}
	else
	{
		additionalSeparationDist = 0.0f;
	}

	openDist += additionalSeparationDist * (float)(blinds.get_size() - 1);

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	inOpenVar.look_up<float>(varStorage);
	outOpenVar.look_up<float>(varStorage);
}

void Blinds::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Blinds::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	// always, even if not active, as currentYaw may be non 0

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(blinds_finalPose);
#endif

	auto const * mesh = get_owner()->get_mesh();

	if (!mesh)
	{
		return;
	}

	auto* moduleSound = get_owner()->get_owner()->get_sound();

	float atDistTarget = openDist * inOpenVar.get<float>();
	float prevAtDist = atDist;
	atDist = blend_to_using_speed(atDist, atDistTarget, speed, _context.get_delta_time());
	if (outOpenVar.is_valid())
	{
		outOpenVar.access<float>() = atDist / openDist;
	}

	if (moduleSound && loopSound.is_valid())
	{
		if ((atDist > 0.0f && prevAtDist == 0.0f) ||
			(atDist < openDist && prevAtDist == openDist))
		{
			if (!loopSoundPlayed.get() || !loopSoundPlayed->is_active())
			{
				loopSoundPlayed = moduleSound->play_sound(loopSound);
			}
		}
		if (atDist == 0.0f || atDist == openDist)
		{
			if (loopSoundPlayed.get() && loopSoundPlayed->is_active())
			{
				loopSoundPlayed->stop();
			}
		}
	}

	float at = atDist;
	for_every(blind, blinds)
	{
#ifdef AN_DEVELOPMENT
#ifndef AN_CLANG
		float minDist = (float)(for_everys_index(blind)) * blindLength;
#endif
#endif
		float moveBack = additionalSeparationDist * (float)(blinds.get_size() - 1);
		blind->at = clamp(at - moveBack, blind->startAtDist, blind->stopAtDist);
		if (moduleSound)
		{
			float const threshold = 0.0001f;

			if (startSound.is_valid() &&
				((blind->at > blind->startAtDist + threshold && blind->prevAt <= blind->startAtDist + threshold) ||
				 (blind->at < blind->stopAtDist - threshold && blind->prevAt >= blind->stopAtDist - threshold)))
			{
				moduleSound->play_sound(startSound);
			}
			if (stopSound.is_valid() &&
				((blind->at <= blind->startAtDist + threshold && blind->prevAt > blind->startAtDist + threshold) ||
				 (blind->at >= blind->stopAtDist - threshold && blind->prevAt < blind->stopAtDist - threshold)))
			{
				moduleSound->play_sound(stopSound);
			}
		}
		blind->prevAt = blind->at;
		blind->endAt = blind->at + blindLength; todo_note(TXT("we actually have to calculate this properly, intersection with curves, etc!"));
		at = at + blindLength + additionalSeparationDist;
	}

	Meshes::Pose * poseLS = _context.access_pose_LS();

	Transform relativeToBoneMS = relativeToBone.is_valid()? poseLS->get_bone_ms_from_ls(relativeToBone.get_index()) : Transform::identity;

	for_every(blind, blinds)
	{
		int const boneIdx = blind->bone.get_index();

		Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);

		Vector3 newAt = calculate_at(blind->at);
		Vector3 newEndAt = calculate_at(blind->endAt);

		Vector3 fwd = newEndAt - newAt;

		if (rotateDist > 0.0f)
		{
			float const startRotatingAt = blind->stopAtDist - rotateDist;
			float const shouldRotate = clamp((blind->at - startRotatingAt) / (rotateDist), 0.0f, 1.0f);
			float const rotateYawNow = rotateAngle * shouldRotate;
			fwd.rotate_by_yaw(rotateYawNow);
		}

		Transform newBoneMS = look_at_matrix(newAt, newAt + fwd, Vector3::zAxis).to_transform(); // hardcoded up
		
		newBoneMS = relativeToBoneMS.to_world(newBoneMS);

		Transform diff = boneMS.to_local(newBoneMS);

		Transform boneLS = poseLS->get_bone(boneIdx);
		poseLS->access_bones()[boneIdx] = boneLS.to_world(diff);

		if (blind->mirrored.is_valid())
		{
			int const boneIdx = blind->mirrored.get_index();

			Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);

			Transform newBoneMSMirrored = look_at_matrix(newBoneMS.get_translation() * Vector3(-1.0f, 1.0f, 1.0f), newBoneMS.location_to_world(Vector3::yAxis) * Vector3(-1.0f, 1.0f, 1.0f), Vector3::zAxis).to_transform(); // hardcoded up

			Transform diff = boneMS.to_local(newBoneMSMirrored);

			Transform boneLS = poseLS->get_bone(boneIdx);
			poseLS->access_bones()[boneIdx] = boneLS.to_world(diff);
		}
	}

	/*
	int const boneIdx = bone.get_index();


	// time advance
	if (has_just_activated())
	{
		timeToNextTurn = Random::get_float(turnInterval.min, turnInterval.max);
	}

	float const deltaTime = _context.get_delta_time();

	if (turnLeft == 0.0f)
	{
		timeToNextTurn -= deltaTime;
		if (timeToNextTurn <= 0.0f)
		{
			turnLeft = (float)Random::get_int_from_range(1, 4) * 90.0f - mod(currentYaw, 90.0f);
			turnLeft *= Random::get_chance(0.5f) ? 1.0f : -1.0f;
			currentTurnSpeed = Random::get_float(speed.min, speed.max);
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
			if (willCoverToSlowDownAddedFrame > abs(turnLeft))
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

	boneLS.set_orientation(boneLS.get_orientation().to_world(Rotator3(0.0f, currentYaw, 0.0f).to_quat()));

	// write back
	poseLS->access_bones()[boneIdx] = boneLS;
	*/
}

void Blinds::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	if (inOpenVar.is_valid())
	{
		dependsOnVariables.push_back(inOpenVar.get_name());
	}
	if (outOpenVar.is_valid())
	{
		providesVariables.push_back(outOpenVar.get_name());
	}

	for_every(blind, blinds)
	{
		if (blind->bone.is_valid())
		{
			dependsOnParentBones.push_back(blind->bone.get_index());
			providesBones.push_back(blind->bone.get_index());
		}
		if (blind->mirrored.is_valid())
		{
			dependsOnParentBones.push_back(blind->mirrored.get_index());
			providesBones.push_back(blind->mirrored.get_index());
		}
	}
}

Vector3 Blinds::calculate_at(float _at) const
{
	for_every(segment, segments)
	{
		if (_at <= segment->length)
		{
			return segment->curve.calculate_at(clamp(_at / segment->length, 0.0f, 1.0f));
		}
		_at -= segment->length;
	}

	return segments.get_last().curve.p3 + postSegmentsTangent * _at;
}

//

REGISTER_FOR_FAST_CAST(BlindsData);

Framework::AppearanceControllerData* BlindsData::create_data()
{
	return new BlindsData();
}

void BlindsData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(blinds), create_data);
}

BlindsData::BlindsData()
{
}

bool BlindsData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= speed.load_from_xml(_node, TXT("speed"));
	result &= blindLength.load_from_xml(_node, TXT("blindLength"));
	result &= blindThickness.load_from_xml(_node, TXT("blindThickness"));

	result &= rotateAngle.load_from_xml(_node, TXT("rotateAngle"));

	result &= relativeToBone.load_from_xml(_node, TXT("relativeToBone"));

	result &= inOpenVar.load_from_xml(_node, TXT("inOpenVarID"));
	result &= outOpenVar.load_from_xml(_node, TXT("outOpenVarID"));

	result &= startSound.load_from_xml(_node, TXT("startSound"));
	result &= loopSound.load_from_xml(_node, TXT("loopSound"));
	result &= stopSound.load_from_xml(_node, TXT("stopSound"));

	result &= openDist.load_from_xml(_node, TXT("openDist"));
	result &= rotateDist.load_from_xml(_node, TXT("rotateDist"));
	result &= additionalSeparationDist.load_from_xml(_node, TXT("additionalSeparationDist"));

	if (!speed.is_set())
	{
		error_loading_xml(_node, TXT("no \"speed\" provided"));
		result = false;
	}
	if (!blindLength.is_set())
	{
		error_loading_xml(_node, TXT("no \"blindLength\" provided"));
		result = false;
	}
	if (!blindThickness.is_set())
	{
		error_loading_xml(_node, TXT("no \"blindThickness\" provided"));
		result = false;
	}
	if (!inOpenVar.is_set())
	{
		error_loading_xml(_node, TXT("no \"inOpenVarID\" provided"));
		result = false;
	}

	for_every(node, _node->children_named(TXT("rail")))
	{
		RailCorner rc;
		if (rc.load_from_xml(node, _lc))
		{
			rails.push_back(rc);
		}
		else
		{
			result = false;
		}
	}

	for_every(node, _node->children_named(TXT("blind")))
	{
		BlindBone bb;
		if (bb.load_from_xml(node, _lc))
		{
			blinds.push_back(bb);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool BlindsData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* BlindsData::create_copy() const
{
	BlindsData* copy = new BlindsData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* BlindsData::create_controller()
{
	return new Blinds(this);
}

void BlindsData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	speed.fill_value_with(_context);
	blindLength.fill_value_with(_context);
	blindThickness.fill_value_with(_context);

	rotateAngle.fill_value_with(_context);

	relativeToBone.fill_value_with(_context);
	
	inOpenVar.fill_value_with(_context);
	outOpenVar.fill_value_with(_context);

	startSound.fill_value_with(_context);
	loopSound.fill_value_with(_context);
	stopSound.fill_value_with(_context);

	openDist.fill_value_with(_context);
	rotateDist.fill_value_with(_context);
	additionalSeparationDist.fill_value_with(_context);

	for_every(rail, rails)
	{
		rail->apply_mesh_gen_params(_context);
	}

	for_every(blind, blinds)
	{
		blind->apply_mesh_gen_params(_context);
	}
}

void BlindsData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void BlindsData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	error(TXT("blinds reskinning is disallowed!"));
	an_assert(false);
}

//

bool BlindsData::RailCorner::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= location.load_from_xml(_node, TXT("location"));
	result &= radius.load_from_xml(_node, TXT("radius"));

	return result;
}

void BlindsData::RailCorner::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	location.fill_value_with(_context);
	radius.fill_value_with(_context);
}

//

bool BlindsData::BlindBone::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= bone.load_from_xml(_node, TXT("bone"));
	result &= mirrored.load_from_xml(_node, TXT("mirrored"));

	return result;
}

void BlindsData::BlindBone::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	bone.fill_value_with(_context);
	mirrored.fill_value_with(_context);
}