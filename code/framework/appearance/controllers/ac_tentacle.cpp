#include "ac_tentacle.h"

#include "ikUtils.h"

#include "..\appearanceControllerPoseContext.h"
#include "..\appearanceControllers.h"

#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\meshGen\meshGenValueDefImpl.inl"
#include "..\..\module\moduleAppearance.h"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

//#define USE_STIFFNESS

//

using namespace Framework;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(tentacle);

//

REGISTER_FOR_FAST_CAST(Tentacle);

Tentacle::Tentacle(TentacleData const * _data)
: base(_data)
, tentacleData(_data)
, isValid(false)
{
	bones.grow_size(2);
	bones[0].bone = tentacleData->startBone.get();
	bones[1].bone = tentacleData->endBone.get();
	if (tentacleData->footBone.is_set())
	{
		footBone = tentacleData->footBone.get();
	}
	targetPlacementMSVar = tentacleData->targetPlacementMSVar.get();
	upDirMSVar = tentacleData->upDirMSVar.get(Name::invalid());
	upDirMS = tentacleData->upDirMS.get(Vector3::zero);
	angleLimit = tentacleData->angleLimit.get();
	alignTowardsNextSegment = tentacleData->alignTowardsNextSegment.get();
	if (tentacleData->randomiseInSphere.is_set()) { randomiseInSphere = tentacleData->randomiseInSphere.get(); }
	if (tentacleData->relativeToStartBone.is_set()) { relativeToStartBone = tentacleData->relativeToStartBone.get(); }
	if (tentacleData->applyVelocity.is_set()) { applyVelocity = tentacleData->applyVelocity.get(); }
	if (tentacleData->applyAccelerationWS.is_set()) { applyAccelerationWS = tentacleData->applyAccelerationWS.get(); }
}

Tentacle::~Tentacle()
{
}

void Tentacle::initialise(ModuleAppearance* _owner)
{
	base::initialise(_owner);

	isValid = true;

	if (Meshes::Skeleton const * skeleton = get_owner()->get_skeleton()->get_skeleton())
	{
		// build chain
		bones[0].bone.look_up(skeleton);
		parentBone = Meshes::BoneID(skeleton, skeleton->get_parent_of(bones[0].bone.get_index()));
		if (bones.get_size() > 2)
		{
			bones[1] = bones.get_last();
			bones.set_size(2);
		}
		bones[1].bone.look_up(skeleton);
		if (footBone.is_name_valid())
		{
			footBone.look_up(skeleton);
		}
		int lookForParentOfIdx = bones.get_last().bone.get_index();
		while (true)
		{
			int parentIdx = skeleton->get_parent_of(lookForParentOfIdx);
			if (parentIdx == NONE)
			{
				error(TXT("invalid tentacle"));
				isValid = false;
				break;
			}
			if (parentIdx == bones[0].bone.get_index())
			{
				break;
			}
			bones.insert_at(1, BoneInfo());
			bones[1].bone = Meshes::BoneID(skeleton, parentIdx);
			lookForParentOfIdx = parentIdx;
		}

		for (int i = 0; i < bones.get_size(); ++i)
		{
			bones[i].placementMS = skeleton->get_bones_default_placement_MS()[i];
		}

		for (int i = 0; i < bones.get_size() - 1; ++i)
		{
			auto& b = bones[i];
			auto& n = bones[i + 1];
			b.lengthToNext = (b.placementMS.get_translation() - n.placementMS.get_translation()).length();
			n.lengthToPrev = b.lengthToNext;
		}

		if (footBone.is_valid())
		{
			int footParentIdx = skeleton->get_parent_of(footBone.get_index());
			if (footParentIdx != bones.get_last().bone.get_index())
			{
				error(TXT("foot bone's parent has to be end bone"));
				isValid = false;
			}
		}
		isValid &= parentBone.is_valid();
		for_every(bone, bones)
		{
			isValid &= bone->bone.is_valid();
			bone->importanceWeight = ((float)for_everys_index(bone) / (float)(bones.get_size() - 1)) * 2.0f - 1.0f;
		}

		if (!isValid)
		{
			error(TXT("invalid bones in tentacle"));
		}
	}
	else
	{
		isValid = false;
	}

	auto & varStorage = get_owner()->access_controllers_variable_storage();
	targetPlacementMSVar.look_up<Transform>(varStorage);
	upDirMSVar.look_up<Vector3>(varStorage);
}

void Tentacle::advance_and_adjust_preliminary_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Tentacle::calculate_final_pose(REF_ AppearanceControllerPoseContext & _context)
{
	base::calculate_final_pose(_context);

	if (!isValid)
	{
		return;
	}

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(tentacle_finalPose);
#endif

	debug_filter(ac_tentacle);
	debug_context(this->get_owner()->get_owner()->get_presence()->get_in_room());
	debug_push_transform(this->get_owner()->get_owner()->get_presence()->get_placement());

	float deltaTime = _context.get_delta_time();
	float invDeltaTime = deltaTime != 0.0f ? 1.0f / deltaTime : 0.0f;

	Meshes::Pose * poseLS = _context.access_pose_LS();

	auto& startBone = bones[0];
	auto& endBone = bones[bones.get_size() - 1];

	Transform parentBoneMS = poseLS->get_bone_ms_from_ls(parentBone.get_index());
	startBone.placementMS = parentBoneMS.to_world(poseLS->get_bone(startBone.bone.get_index()));
	startBone.velocityMS = Vector3::zero;

	Transform targetPlacementMS = targetPlacementMSVar.get<Transform>();

	/*
	// !@# until we provide a valid placement
	{
		float s = 0.35f;
		float f = 0.35f;
		float h = -0.35f;
		if (targetPlacementMSVar.get_name().to_string().does_contain(TXT("_fr")))
		{
			targetPlacementMS.set_translation(Vector3(s, f, h));
		}
		if (targetPlacementMSVar.get_name().to_string().does_contain(TXT("_br")))
		{
			targetPlacementMS.set_translation(Vector3(s, -f, h));
		}
		if (targetPlacementMSVar.get_name().to_string().does_contain(TXT("_fl")))
		{
			targetPlacementMS.set_translation(Vector3(-s, f, h));
		}
		if (targetPlacementMSVar.get_name().to_string().does_contain(TXT("_bl")))
		{
			targetPlacementMS.set_translation(Vector3(-s, -f, h));
		}
		targetPlacementMS.set_orientation(look_matrix33(Vector3::yAxis, Vector3::zAxis).to_quat());
	}
	*/

	if (has_just_activated())
	{
		for (int i = 1; i < bones.get_size(); ++i)
		{
			auto& b = bones[i];
			b.placementMS = poseLS->get_bone_ms_from_ls(b.bone.get_index());
			b.velocityMS = Vector3::zero;
		}
		for (int i = 0; i < bones.get_size() - 1; ++i)
		{
			auto& b = bones[i];
			auto& n = bones[i + 1];
			b.lengthToNext = (b.placementMS.get_translation() - n.placementMS.get_translation()).length();
			n.lengthToPrev = b.lengthToNext;
		}

		if (randomiseInSphere.is_set())
		{
			if (randomiseInSphere.get().get_radius() > 0.0f)
			{
				Random::Generator rg;
				rg.advance_seed(this);
				for (int i = 1; i < bones.get_size(); ++i)
				{
					auto& b = bones[i];
					Vector3 dir;
					dir.x = rg.get_float(-1.0f, 1.0f);
					dir.y = rg.get_float(-1.0f, 1.0f);
					dir.z = rg.get_float(-1.0f, 1.0f);
					dir = dir.normal();
					b.placementMS.set_translation(randomiseInSphere.get().get_centre() + dir * rg.get_float(0.0f, randomiseInSphere.get().get_radius()));
					b.velocityMS = Vector3::zero;
				}
			}
			randomiseInSphere.clear();
		}
	}
	else
	{
		if (relativeToStartBone)
		{
			for (int i = 1; i < bones.get_size() - 1; ++i)
			{
				auto& b = bones[i];
				b.placementMS = startBone.placementMS.to_world(b.placementMS);
				b.velocityMS = startBone.placementMS.vector_to_world(b.velocityMS);
			}
		}
	}

	endBone.velocityMS = (targetPlacementMS.get_translation() - endBone.placementMS.get_translation()) * invDeltaTime;
	endBone.placementMS = targetPlacementMS;

	if (applyAccelerationWS.is_set())
	{
		Vector3 applyAccelerationMS = get_owner()->get_ms_to_ws_transform().vector_to_local(applyAccelerationWS.get());
		applyAccelerationMS *= deltaTime;
		
		for_every(b, bones)
		{
			b->velocityMS += applyAccelerationMS;
		}
	}
	
	if (applyVelocity)
	{
		auto * b = bones.begin() + 1;
		for (int boneIdx = 1; boneIdx < bones.get_size() - 1; ++boneIdx, ++b)
		{
			Vector3 locMS = b->placementMS.get_translation();
			locMS += b->velocityMS * deltaTime;
			b->placementMS.set_translation(locMS);
		}
	}

	{
		struct ItBone
		{
			Vector3 forwardMS;
			Vector3 prevForwardMS;
			Vector3 locationMS;
			float offsetPrev;
			float offsetNext;
		};
		ARRAY_STACK(ItBone, itBones, bones.get_size());
		itBones.set_size(bones.get_size());
		{
			auto* iBone = itBones.begin();
			for_every(bone, bones)
			{
				iBone->locationMS = bone->placementMS.get_translation();
				iBone->forwardMS = bone->placementMS.get_axis(Axis::Forward);
				iBone->offsetPrev = -bone->lengthToPrev;
				iBone->offsetNext = bone->lengthToNext;
				++iBone;
			}
		}
		float angleLimitDot = cos_deg(angleLimit);
		{
			int iterationCount = 10;
			for_count(int, iteration, iterationCount + 1)
			{
				/**
				 *	Rules:
				 *		1. first and last stay as they are
				 *		2. each one is aligned in such a way that forward points on line prev->next, up is aligned with the prev's up (up alignment is done at the end!)
				 *		3. tries to pull into location between the prev's and the next's forwards*distance
				 */

				// calculate forward dirs (extra iteration to keep the code here)
				{
					auto* pb = itBones.begin();
					auto* cb = pb + 1;
					auto* nb = cb + 1;
					auto const * b = bones.begin() + 1;
					for (int boneIdx = 1; boneIdx < itBones.get_size() - 1; ++boneIdx, ++pb, ++cb, ++nb, ++b)
					{
						// broken into separate as we will be fiddling with weights
						float nextWeight = (0.5f + b->importanceWeight * 0.2f);
						cb->forwardMS = (cb->locationMS - pb->locationMS).normal() * (1.0f - nextWeight)
									  + (nb->locationMS - cb->locationMS).normal() * nextWeight;
						cb->forwardMS.normalise();
					}
				}

				if (iteration < iterationCount - 1)
				{
					// iterate locations
					{
						auto* pb = itBones.begin();
						auto* cb = pb + 1;
						auto* nb = cb + 1;
						auto const* b = bones.begin() + 1;
						for(int boneIdx = 1; boneIdx < itBones.get_size() - 1; ++ boneIdx, ++pb, ++cb, ++nb, ++b)
						{
							float const useNext = 0.5f;
							float const usePrev = 1.0f - useNext;

							Vector3 toBeWhereItShould;
							{
								Vector3 shouldBeAtN;
								Vector3 shouldBeAtP;
								if (angleLimit > 0.0f)
								{
									{
										Vector3 requestedForwardMS = (nb->locationMS - cb->locationMS).normal();
										requestedForwardMS = requestedForwardMS.keep_within_angle_normalised(nb->forwardMS, angleLimitDot);
										shouldBeAtN = nb->locationMS + requestedForwardMS * nb->offsetPrev;
									}
									{
										Vector3 requestedForwardMS = (cb->locationMS - pb->locationMS).normal();
										requestedForwardMS = requestedForwardMS.keep_within_angle_normalised(pb->forwardMS, angleLimitDot);
										shouldBeAtP = pb->locationMS + requestedForwardMS * pb->offsetNext;
									}
								}
								else
								{
									shouldBeAtN = nb->locationMS + nb->forwardMS * nb->offsetPrev;
									shouldBeAtP = pb->locationMS + pb->forwardMS * pb->offsetNext;
								}
								Vector3 shouldBeAt = shouldBeAtN * useNext + shouldBeAtP * usePrev;
								toBeWhereItShould = shouldBeAt - cb->locationMS;
							}

#ifdef USE_STIFFNESS
							Vector3 moveDueToStiffness = Vector3::zero;
							float stiffnessDeg = 10.0f;
							float stiffness = cos_deg(stiffnessDeg);
							for_count(int, check, 2)
							{
								float signForward = check == 0? 1.0f : -1.0f;
								auto* ob = check == 0 ? pb : nb;
								Vector3 fwdTowardsCB = ob->forwardMS * signForward;
								float actStiff = Vector3::dot(fwdTowardsCB, (cb->locationMS - ob->locationMS).normal());
								if (actStiff < stiffness)
								{
									float p2c = (cb->locationMS - ob->locationMS).length();
									float alongDist = Vector3::dot(fwdTowardsCB, (cb->locationMS - ob->locationMS));
									float maxDist = sin_deg(stiffnessDeg) * p2c;
									Vector3 beAt = ob->locationMS + fwdTowardsCB * alongDist;
									float actualDist = (beAt - cb->locationMS).length();
									moveDueToStiffness += (beAt - cb->locationMS) * ((actualDist - maxDist) / p2c);
									debug_draw_arrow(true, Colour::magenta, cb->locationMS, cb->locationMS + (beAt - cb->locationMS) * ((actualDist - maxDist) / p2c));
									debug_draw_arrow(true, Colour::c64Grey1, cb->locationMS + (beAt - cb->locationMS) * ((actualDist - maxDist) / p2c), beAt);
								}
							}
#endif

							Vector3 maintainDistToPrev;
							{
								Vector3 toPrev = pb->locationMS - cb->locationMS;
								maintainDistToPrev = toPrev.normal() * (toPrev.length() - abs(cb->offsetPrev));
							}
							Vector3 maintainDistToNext;
							{
								Vector3 toNext = nb->locationMS - cb->locationMS;
								maintainDistToNext = toNext.normal() * (toNext.length() - abs(cb->offsetNext));
							}

							float nextWeight = (0.5f + b->importanceWeight * 0.1f);

							cb->locationMS += (Vector3::zero
											 + toBeWhereItShould * 0.5f
											 + maintainDistToPrev * (1.0f + (1.0f - nextWeight))
											 + maintainDistToNext * (1.0f + nextWeight)
#ifdef USE_STIFFNESS
											 + moveDueToStiffness * 0.7f
#endif
											 //+ upDirMS * 0.3f
											 )
											* 0.2f;

							//if (boneIdx == 2 && iteration == iterationCount - 2)
							//{
							//	debug_draw_arrow(true, Colour::yellow, nb->locationMS, shouldBeAtN);
							//	debug_draw_arrow(true, Colour::yellow, pb->locationMS, shouldBeAtP);
							//}
						}
					}
				}
			}
		}

		if (alignTowardsNextSegment > 0.0f)
		{
			auto* pb = itBones.begin();
			auto* cb = pb + 1;
			auto* nb = cb + 1;
			auto const* b = bones.begin() + 1;
			for (int bIdx = 1; bIdx < itBones.get_size() - 1; ++bIdx, ++pb, ++cb, ++nb, ++b)
			{
				Vector3 cf = (cb->forwardMS).normal();
				Vector3 nf = (nb->locationMS - cb->locationMS).normal();
				cb->forwardMS = (cf * (1.0f - alignTowardsNextSegment) + nf * alignTowardsNextSegment).normal();
			}
		}

		// update placements (align up dirs)
		{
			auto* iBone = itBones.begin() + 1;
			auto* pBone = bones.begin();
			auto* bone = pBone + 1;
			for (int boneIdx = 1; boneIdx < itBones.get_size(); ++boneIdx, ++iBone, ++pBone, ++bone)
			{
				if (applyVelocity)
				{
					Vector3 velocityMS = (iBone->locationMS - bone->placementMS.get_translation()) * invDeltaTime;
					bone->velocityMS = velocityMS;// blend_to_using_time(bone->velocityMS, velocityMS, 0.1f, deltaTime);
				}
				bone->placementMS = look_matrix(iBone->locationMS, iBone->forwardMS, pBone->placementMS.get_axis(Axis::Up)).to_transform();
			}
			bones.get_last().placementMS.set_orientation(look_matrix33(targetPlacementMS.get_axis(Axis::Y), bones.get_last().placementMS.get_axis(Axis::Up)).to_quat());
			debug_draw_arrow(true, Colour::c64Blue, targetPlacementMS.get_translation(), targetPlacementMS.get_translation() + targetPlacementMS.get_axis(Axis::Forward) * 0.05f);
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_DEBUG_RENDERER
	for_count(int, i, bones.get_size() - 1)
	{
		Vector3 a = bones[i].placementMS.get_translation();
		Vector3 b = bones[i + 1].placementMS.get_translation();
		Vector3 d = (b - a).normal();

		Vector3 c = a + d * bones[i].lengthToNext;
		debug_draw_line(true, Colour::green, a, c);
		debug_draw_line(true, Colour::red, c, b);
		debug_draw_arrow(true, Colour::c64LightBlue, a, a + bones[i].placementMS.get_axis(Axis::Forward) * 0.05f);
	}
#endif
#endif

	// ms to ls
	{
		Transform prevBoneMS = parentBoneMS;
		for_every(bone, bones)
		{
			poseLS->set_bone(bone->bone.get_index(), prevBoneMS.to_local(bone->placementMS));
			prevBoneMS = bone->placementMS;
		}
	}

	// and rotate foot bone properly to match target placement
	if (footBone.is_valid())
	{
		Transform footBoneLS = poseLS->get_bone(footBone.get_index());
		Transform footBoneMS = targetPlacementMS.to_world(footBoneLS);
		footBoneLS = bones.get_last().placementMS.to_local(footBoneMS);
		poseLS->set_bone(footBone.get_index(), footBoneLS);
	}

	if (relativeToStartBone)
	{
		for (int i = 1; i < bones.get_size() - 1; ++i)
		{
			auto& b = bones[i];
			b.placementMS = startBone.placementMS.to_local(b.placementMS);
			b.velocityMS = startBone.placementMS.vector_to_local(b.velocityMS);
		}
	}

#ifndef AN_CLANG
	an_assert(poseLS->is_ok());
#endif

	debug_pop_transform();
	debug_no_context();
	debug_no_filter();
}

void Tentacle::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
	OUT_ ArrayStack<Name> & dependsOnVariables, OUT_ ArrayStack<Name>& providesVariables, OUT_ ArrayStack<Name>& dependsOnControllers, OUT_ ArrayStack<Name>& providesControllers) const
{
	base::get_info_for_auto_processing_order(OUT_ dependsOnBones, OUT_ dependsOnParentBones, OUT_ usesBones, OUT_ providesBones,
		OUT_ dependsOnVariables, OUT_ providesVariables, OUT_ dependsOnControllers, OUT_ providesControllers);

	dependsOnBones.push_back(parentBone.get_index());
	for_every(bone, bones)
	{
		//dependsOnBones.push_back(bone->bone.get_index());
		providesBones.push_back(bone->bone.get_index());
	}
	if (footBone.is_valid())
	{
		providesBones.push_back(footBone.get_index());
	}
	if (targetPlacementMSVar.is_valid())
	{
		dependsOnVariables.push_back(targetPlacementMSVar.get_name());
	}
	if (upDirMSVar.is_valid())
	{
		dependsOnVariables.push_back(upDirMSVar.get_name());
	}	
}

//

REGISTER_FOR_FAST_CAST(TentacleData);

AppearanceControllerData* TentacleData::create_data()
{
	return new TentacleData();
}

void TentacleData::register_itself()
{
	AppearanceControllers::register_appearance_controller(NAME(tentacle), create_data);
}

TentacleData::TentacleData()
{
}

bool TentacleData::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= startBone.load_from_xml(_node, TXT("startBone"));
	result &= endBone.load_from_xml(_node, TXT("endBone"));
	result &= footBone.load_from_xml(_node, TXT("footBone"));

	result &= targetPlacementMSVar.load_from_xml(_node, TXT("targetPlacementMSVarID"));
	result &= upDirMSVar.load_from_xml(_node, TXT("upDirMSVarID"));
	result &= upDirMS.load_from_xml(_node, TXT("upDirMS"));

	result &= angleLimit.load_from_xml(_node, TXT("angleLimit"));
	result &= alignTowardsNextSegment.load_from_xml(_node, TXT("alignTowardsNextSegment"));
	
	result &= randomiseInSphere.load_from_xml(_node, TXT("randomiseInSphere"));
	
	result &= relativeToStartBone.load_from_xml(_node, TXT("relativeToStartBone"));
	result &= applyVelocity.load_from_xml(_node, TXT("applyVelocity"));
	result &= applyAccelerationWS.load_from_xml(_node, TXT("applyAccelerationWS"));

	return result;
}

bool TentacleData::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

AppearanceControllerData* TentacleData::create_copy() const
{
	TentacleData* copy = new TentacleData();
	*copy = *this;
	return copy;
}

AppearanceController* TentacleData::create_controller()
{
	return new Tentacle(this);
}

void TentacleData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	startBone.if_value_set([this, _rename](){ startBone = _rename(startBone.get(), NP); });
	endBone.if_value_set([this, _rename](){ endBone = _rename(endBone.get(), NP); });
	footBone.if_value_set([this, _rename](){ footBone = _rename(footBone.get(), NP); });
	targetPlacementMSVar.if_value_set([this, _rename](){ targetPlacementMSVar = _rename(targetPlacementMSVar.get(), NP); });
	upDirMSVar.if_value_set([this, _rename]() { upDirMSVar = _rename(upDirMSVar.get(), NP); });
}

void TentacleData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);

	upDirMS.if_value_set([this, _transform]() { upDirMS.access() = _transform.vector_to_world(upDirMS.get()); });
	randomiseInSphere.if_value_set([this, _transform]() { randomiseInSphere.access().apply_transform(_transform); });
}

void TentacleData::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	startBone.fill_value_with(_context);
	endBone.fill_value_with(_context);
	footBone.fill_value_with(_context);
	targetPlacementMSVar.fill_value_with(_context);
	upDirMSVar.fill_value_with(_context);
	upDirMS.fill_value_with(_context);
	angleLimit.fill_value_with(_context);
	alignTowardsNextSegment.fill_value_with(_context);
	randomiseInSphere.fill_value_with(_context);
	relativeToStartBone.fill_value_with(_context);
	applyVelocity.fill_value_with(_context);
	applyAccelerationWS.fill_value_with(_context);
}

