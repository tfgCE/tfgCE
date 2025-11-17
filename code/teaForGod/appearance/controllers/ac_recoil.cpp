#include "ac_recoil.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\modulePilgrimData.h"
#include "..\..\modules\custom\mc_grabable.h"

#include "..\..\..\framework\appearance\appearanceControllerPoseContext.h"
#include "..\..\..\framework\appearance\appearanceControllers.h"
#include "..\..\..\framework\meshGen\meshGenGenerationContext.h"
#include "..\..\..\framework\meshGen\meshGenParamImpl.inl"
#include "..\..\..\framework\meshGen\meshGenValueDefImpl.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#include "..\..\..\core\mesh\pose.h"

#include "..\..\..\core\performance\performanceUtils.h"

#include "..\..\..\core\debug\debugRenderer.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
#define DEBUG_DRAW_GRAB
//#define DEBUG_OUTPUT_RECOIL
#endif

//

using namespace TeaForGodEmperor;
using namespace AppearanceControllersLib;

//

DEFINE_STATIC_NAME(recoil);

//

REGISTER_FOR_FAST_CAST(Recoil);

Recoil::Recoil(RecoilData const * _data)
: base(_data)
, recoilData(_data)
{
	bone = recoilData->bone.get();

	maxRecoilTime = recoilData->maxRecoilTime.get(maxRecoilTime);
	maxAccumulatedRecoilTime = recoilData->maxAccumulatedRecoilTime.get(maxAccumulatedRecoilTime);
	recoilAttackTimePt = recoilData->recoilAttackTimePt.get(recoilAttackTimePt);
	maxRecoilAttackTime = recoilData->maxRecoilAttackTime.get(maxRecoilAttackTime);

	locationRecoil = recoilData->locationRecoil.get(locationRecoil);
	orientationRecoil = recoilData->orientationRecoil.get(orientationRecoil);
	
	orientationRecoilMask = recoilData->orientationRecoilMask.get(orientationRecoilMask);
	
	applyRecoilStrength = recoilData->applyRecoilStrength.get(applyRecoilStrength);
}

Recoil::~Recoil()
{
}

void Recoil::initialise(Framework::ModuleAppearance* _owner)
{
	base::initialise(_owner);

	if (auto* skel = get_owner()->get_skeleton()->get_skeleton())
	{
		bone.look_up(skel);
		parent = Meshes::BoneID(skel, skel->get_parent_of(bone.get_index()));
	}
}

void Recoil::advance_and_adjust_preliminary_pose(REF_ Framework::AppearanceControllerPoseContext & _context)
{
	base::advance_and_adjust_preliminary_pose(_context);
}

void Recoil::calculate_final_pose(REF_ Framework::AppearanceControllerPoseContext& _context)
{
	base::calculate_final_pose(_context);

#ifdef MEASURE_PERFORMANCE_APPEARANCE_CONTROLLERS_DETAILED
	MEASURE_PERFORMANCE(recoil_finalPose);
#endif

	auto const* mesh = get_owner()->get_mesh();

	if (!mesh ||
		!bone.is_valid() ||
		!parent.is_valid())
	{
		return;
	}

	int const boneIdx = bone.get_index();
	Meshes::Pose* poseLS = _context.access_pose_LS();

	Transform boneLS = poseLS->get_bone(boneIdx);

	if (!pilgrimOwner.get())
	{
		if (auto* ap = get_owner())
		{
			pilgrimOwner = ap->get_owner();
		}		
	}

	bool clearRecoil = true;

	if (auto* pimo = pilgrimOwner.get())
	{
		if (auto* mp = pimo->get_gameplay_as<ModulePilgrim>())
		{
			auto* curEq = mp->get_hand_equipment(recoilData->hand);
			if (currentEquipment != curEq)
			{
				currentEquipment = curEq;
				timeSinceLastShot = -1.0f;
			}
		}

		if (auto* ce = currentEquipment.get())
		{
			if (auto* g = ce->get_gameplay_as<ModuleEquipments::Gun>())
			{
				clearRecoil = false;

				float gunTimeSinceLastShot = g->get_time_since_last_shot();
				bool shot = gunTimeSinceLastShot < timeSinceLastShot;
				timeSinceLastShot = gunTimeSinceLastShot;

				// we need one frame delay
				// frame
				//		logic -> shoot
				//		animation advance (can't do the recoil here!)
				//		projectile created, placed at the socket
				if (shot)
				{
					recoilDelay.set_if_not_set(0);
				}
				else
				{
					if (recoilDelay.is_set())
					{
						recoilDelay = recoilDelay.get() + 1;
					}
				}

				if (recoilDelay.is_set() && recoilDelay.get() >= 1)
				{
					recoilDelay.clear();
					float chamberLoadingTime = g->calculate_chamber_loading_time();
					recoilTimeLeft = min(maxRecoilTime, chamberLoadingTime);
#ifdef DEBUG_OUTPUT_RECOIL
					output(TXT("start recoil time %.3f"), recoilTimeLeft);
#endif
					recoilAttackTimeLeft = min(recoilTimeLeft * recoilAttackTimePt, maxRecoilAttackTime);
					recoilTimeLeft *= (1.0f + log2f(1.0f + 3.5f * accumulatedRecoil));
#ifdef DEBUG_OUTPUT_RECOIL
					output(TXT("increased %.3f (acc:%.3f)"), recoilTimeLeft, accumulatedRecoil);
#endif
					recoilTimeLeft = min(min(maxAccumulatedRecoilTime, chamberLoadingTime + 1.2f), recoilTimeLeft); // to have automatic fire weapons recoil
					recoilTimeLeft = max(recoilTimeLeft, recoilAttackTimeLeft * 2.0f);
#ifdef DEBUG_OUTPUT_RECOIL
					output(TXT("final %.3f"), recoilTimeLeft);
#endif

					currentRecoilTargetLS = Transform::identity;

					float shotStrength = 1.0f;
					{
						Energy totalDamage;
						g->get_projectile_damage(true, &totalDamage);
						float useDamage = 0.7f;
						shotStrength = (totalDamage.as_float() * useDamage + (1.0f - useDamage) * g->get_chamber_size().as_float());
					}
					float recoilStrength = max(0.5f, 1.0f + (shotStrength - 1.0f) * Random::get(applyRecoilStrength));

					float prevAccRS = accumulatedRecoil;
					{
						float newAccRecoil = recoilStrength * 1.8f;
						accumulatedRecoil = accumulatedRecoil * 0.95f + newAccRecoil;
					}

					// increase more for smaller (as we expect them to be more repetitive)
					float useRel = 0.6f;
					float increaseAmountCoef = 0.6f * (1.0f - useRel) + useRel * max(0.4f, 4.0f / recoilStrength);
#ifdef DEBUG_OUTPUT_RECOIL
					output(TXT("rec strength %.3f -> %.3f (coef:%.3f) (acc:%.3f)"), recoilStrength, recoilStrength + prevAccRS * increaseAmountCoef, increaseAmountCoef, accumulatedRecoil);
#endif
					float recoilStrengthForLoc = recoilStrength;
					recoilStrength += prevAccRS * increaseAmountCoef;

					Framework::Room* inRoom;
					Transform gunWS;
					ce->get_presence()->get_attached_placement_for_top(OUT_ inRoom, OUT_ gunWS, nullptr);
					if (inRoom == pimo->get_presence()->get_in_room())
					{
						Transform boneMS = poseLS->get_bone_ms_from_ls(boneIdx);
						Transform boneWS = pimo->get_appearance()->get_ms_to_ws_transform().to_world(boneMS);

						Transform gunLS = boneWS.to_local(gunWS);

						Transform muzzleGunOS = g->get_muzzle_placement_os();

						Transform muzzleLS = gunLS.to_world(muzzleGunOS);

						{
							Vector3 recoilLinear = -muzzleLS.get_axis(Axis::Forward);
							Rotator3 recoilRotation = Rotator3::zero;
							{
								Vector3 startAt = muzzleLS.get_translation();
								Vector3 endAt = startAt + recoilLinear * startAt.length() * 0.3f;

								Rotator3 rotStart = startAt.to_rotator();
								Rotator3 rotEnd = endAt.to_rotator();

								Rotator3 rotDiff = rotEnd - rotStart;
								recoilRotation = rotDiff;
							}

							if (g->get_core_kind() == WeaponCoreKind::Corrosion)
							{
								recoilLinear *= 1.2f;
								recoilRotation *= 0.8f;
							}

							if (g->get_core_kind() == WeaponCoreKind::Lightning)
							{
								recoilLinear *= 0.4f;
								recoilRotation *= 0.2f;
							}

							currentRecoilTargetLS = Transform(recoilLinear * (recoilStrengthForLoc * Random::get(locationRecoil)), (recoilRotation * orientationRecoilMask * (recoilStrength * Random::get(orientationRecoil))).to_quat());
						}
					}
					currentRecoilTargetLS = currentRecoilLS.to_world(currentRecoilTargetLS);
				}
			}
		}
	}

	if (clearRecoil)
	{
		recoilAttackTimeLeft = 0.0f;
		recoilTimeLeft = min(0.1f, recoilTimeLeft);
	}

	{
		float deltaTime = _context.get_delta_time();
		if (recoilAttackTimeLeft > 0.0f)
		{
			float timeCovered = min(deltaTime, recoilAttackTimeLeft);
			float byPt = timeCovered / recoilAttackTimeLeft;
			currentRecoilLS = Transform::lerp(sqrt(max(0.0f, byPt)), currentRecoilLS, currentRecoilTargetLS);
		}
		else if (recoilTimeLeft > 0.0f)
		{
			float timeCovered = min(deltaTime, recoilTimeLeft);
			float byPt = timeCovered / recoilTimeLeft;
			currentRecoilLS = Transform::lerp(sqrt(max(0.0f,byPt)), currentRecoilLS, Transform::identity);
		}
		else
		{
			currentRecoilLS = Transform::identity;
		}

		recoilTimeLeft = max(0.0f, recoilTimeLeft - deltaTime);
		recoilAttackTimeLeft = max(0.0f, recoilAttackTimeLeft - deltaTime);

		accumulatedRecoil = blend_to_using_time(accumulatedRecoil, 0.0f, 0.5f, deltaTime);
#ifdef DEBUG_OUTPUT_RECOIL
		if (accumulatedRecoil > 0.0f)
		{
			output(TXT("rtl %.3f (acc:%.3f)"), recoilTimeLeft, accumulatedRecoil);
		}
#endif
	}

	boneLS = boneLS.to_world(currentRecoilLS);

	poseLS->access_bones()[boneIdx] = boneLS;
}

void Recoil::get_info_for_auto_processing_order(OUT_ ArrayStack<int> & dependsOnBones, OUT_ ArrayStack<int> & dependsOnParentBones, OUT_ ArrayStack<int> & usesBones, OUT_ ArrayStack<int> & providesBones,
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

REGISTER_FOR_FAST_CAST(RecoilData);

Framework::AppearanceControllerData* RecoilData::create_data()
{
	return new RecoilData();
}

void RecoilData::register_itself()
{
	Framework::AppearanceControllers::register_appearance_controller(NAME(recoil), create_data);
}

RecoilData::RecoilData()
{
}

bool RecoilData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= bone.load_from_xml(_node, TXT("bone"));
	hand = Hand::parse(_node->get_string_attribute_or_from_child(TXT("hand"), String(Hand::to_char(hand))));

	result &= maxRecoilTime.load_from_xml(_node, TXT("maxRecoilTime"));
	result &= maxAccumulatedRecoilTime.load_from_xml(_node, TXT("maxAccumulatedRecoilTime"));
	result &= recoilAttackTimePt.load_from_xml(_node, TXT("recoilAttackTimePt"));
	result &= maxRecoilAttackTime.load_from_xml(_node, TXT("maxRecoilAttackTime"));

	result &= locationRecoil.load_from_xml(_node, TXT("locationRecoil"));
	result &= orientationRecoil.load_from_xml(_node, TXT("orientationRecoil"));
	
	result &= orientationRecoilMask.load_from_xml(_node, TXT("orientationRecoilMask"));
	
	result &= applyRecoilStrength.load_from_xml(_node, TXT("applyRecoilStrength"));

	return result;
}

bool RecoilData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

Framework::AppearanceControllerData* RecoilData::create_copy() const
{
	RecoilData* copy = new RecoilData();
	*copy = *this;
	return copy;
}

Framework::AppearanceController* RecoilData::create_controller()
{
	return new Recoil(this);
}

void RecoilData::apply_mesh_gen_params(Framework::MeshGeneration::GenerationContext const & _context)
{
	base::apply_mesh_gen_params(_context);

	bone.fill_value_with(_context);

	maxRecoilTime.fill_value_with(_context);
	maxAccumulatedRecoilTime.fill_value_with(_context);
	recoilAttackTimePt.fill_value_with(_context);
	maxRecoilAttackTime.fill_value_with(_context);

	locationRecoil.fill_value_with(_context);
	orientationRecoil.fill_value_with(_context);
	
	orientationRecoilMask.fill_value_with(_context);
	
	applyRecoilStrength.fill_value_with(_context);

}

void RecoilData::apply_transform(Matrix44 const & _transform)
{
	base::apply_transform(_transform);
}

void RecoilData::reskin(Meshes::BoneRenameFunc _rename)
{
	base::reskin(_rename);

	bone.if_value_set([this, _rename](){ bone = _rename(bone.get(), NP); });
}
