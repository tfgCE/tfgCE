#include "appearanceControllers.h"

#include "controllers\ac_adjustWalkerPlacements.h"
#include "controllers\ac_alignAxes.h"
#include "controllers\ac_alignmentRoll.h"
#include "controllers\ac_animationGraph.h"
#include "controllers\ac_animTransformToVar.h"
#include "controllers\ac_applyBoneOffset.h"
#include "controllers\ac_applyTransformToVar.h"
#include "controllers\ac_atBone.h"
#include "controllers\ac_atPlacement.h"
#include "controllers\ac_atRotator.h"
#include "controllers\ac_atSocket.h"
#include "controllers\ac_autoBlendBones.h"
#include "controllers\ac_bob.h"
#include "controllers\ac_combineVariables.h"
#include "controllers\ac_continuousRotation.h"
#include "controllers\ac_copyRoomVariables.h"
#include "controllers\ac_dragBoneUp.h"
#include "controllers\ac_footAdjuster.h"
#include "controllers\ac_forthAndBackVar.h"
#include "controllers\ac_gyro.h"
#include "controllers\ac_handToLegs.h"
#include "controllers\ac_head.h"
#include "controllers\ac_lagPlacementMS.h"
#include "controllers\ac_lean.h"
#include "controllers\ac_matchPlacementObject.h"
#include "controllers\ac_offset.h"
#include "controllers\ac_orientAsOtherBone.h"
#include "controllers\ac_previewControllerTransform.h"
#include "controllers\ac_provideFingerTipPlacementMS.h"
#include "controllers\ac_providePlacementFromBoneMS.h"
#include "controllers\ac_providePlacementFromOtherIMOSocketMS.h"
#include "controllers\ac_providePlacementFromSocketMS.h"
#include "controllers\ac_poseInject.h"
#include "controllers\ac_poseManager.h"
#include "controllers\ac_readyHumanoidArm.h"
#include "controllers\ac_readyHumanoidSpine.h"
#include "controllers\ac_reorientate.h"
#include "controllers\ac_rotate.h"
#include "controllers\ac_rotateTowardsObject.h"
#include "controllers\ac_scale.h"
#include "controllers\ac_shake.h"
#include "controllers\ac_springPlacementMS.h"
#include "controllers\ac_storeBoneOffset.h"
#include "controllers\ac_sway.h"
#include "controllers\ac_tentacle.h"
#include "controllers\ac_vr.h"
#include "controllers\ikSolver2.h"
#include "controllers\ikSolver3.h"
#include "controllers\rotateTowards.h"
#include "controllers\walker.h"

#include "controllers\particleModifiers\acpm_intensityToNumber.h"
#include "controllers\particleModifiers\acpm_scaleEach.h"

#include "controllers\particles\acp_crumbs.h"
#include "controllers\particles\acp_decal.h"
#include "controllers\particles\acp_dissolve.h"
#include "controllers\particles\acp_dust.h"
#include "controllers\particles\acp_emitter.h"
#include "controllers\particles\acp_freeRange.h"
#include "controllers\particles\acp_lightning.h"
#include "controllers\particles\acp_localGravity.h"
#include "controllers\particles\acp_orientationOffset.h"
#include "controllers\particles\acp_scale.h"

using namespace Framework;

void AppearanceControllers::initialise_static()
{
	base::initialise_static();

	// lib
	AppearanceControllersLib::AdjustWalkerPlacementsData::register_itself();
	AppearanceControllersLib::AlignAxesData::register_itself();
	AppearanceControllersLib::AlignmentRollData::register_itself();
	AppearanceControllersLib::AnimationGraphData::register_itself();
	AppearanceControllersLib::AnimTransformToVarData::register_itself();
	AppearanceControllersLib::ApplyBoneOffsetData::register_itself();
	AppearanceControllersLib::ApplyTransformToVarData::register_itself();
	AppearanceControllersLib::AtBoneData::register_itself();
	AppearanceControllersLib::AtPlacementData::register_itself();
	AppearanceControllersLib::AtRotatorData::register_itself();
	AppearanceControllersLib::AtSocketData::register_itself();
	AppearanceControllersLib::AutoBlendBonesData::register_itself();
	AppearanceControllersLib::BobData::register_itself();
	AppearanceControllersLib::CombineVariablesData::register_itself();
	AppearanceControllersLib::ContinuousRotationData::register_itself();
	AppearanceControllersLib::CopyRoomVariablesData::register_itself();
	AppearanceControllersLib::DragBoneUpData::register_itself();
	AppearanceControllersLib::FootAdjusterData::register_itself();
	AppearanceControllersLib::ForthAndBackVarData::register_itself();
	AppearanceControllersLib::GyroData::register_itself();
	AppearanceControllersLib::HandToLegsData::register_itself();
	AppearanceControllersLib::HeadData::register_itself();
	AppearanceControllersLib::IKSolver2Data::register_itself();
	AppearanceControllersLib::IKSolver3Data::register_itself();
	AppearanceControllersLib::LagPlacementMSData::register_itself();
	AppearanceControllersLib::LeanData::register_itself();
	AppearanceControllersLib::MatchPlacementObjectData::register_itself();
	AppearanceControllersLib::OffsetData::register_itself();
	AppearanceControllersLib::OrientAsOtherBoneData::register_itself();
	AppearanceControllersLib::PreviewControllerTransformData::register_itself();
	AppearanceControllersLib::ProvideFingerTipPlacementMSData::register_itself();
	AppearanceControllersLib::ProvidePlacementFromBoneMSData::register_itself();
	AppearanceControllersLib::ProvidePlacementFromOtherIMOSocketMSData::register_itself();
	AppearanceControllersLib::ProvidePlacementFromSocketMSData::register_itself();
	AppearanceControllersLib::PoseInjectData::register_itself();
	AppearanceControllersLib::PoseManagerData::register_itself();
	AppearanceControllersLib::ReadyHumanoidArmData::register_itself();
	AppearanceControllersLib::ReadyHumanoidSpineData::register_itself();
	AppearanceControllersLib::ReorientateData::register_itself();
	AppearanceControllersLib::RotateData::register_itself();
	AppearanceControllersLib::RotateTowardsData::register_itself();
	AppearanceControllersLib::RotateTowardsObjectData::register_itself();
	AppearanceControllersLib::ScaleData::register_itself();
	AppearanceControllersLib::ShakeData::register_itself();
	AppearanceControllersLib::SpringPlacementMSData::register_itself();
	AppearanceControllersLib::StoreBoneOffsetData::register_itself();
	AppearanceControllersLib::SwayData::register_itself();
	AppearanceControllersLib::TentacleData::register_itself();
	AppearanceControllersLib::VRData::register_itself();
	AppearanceControllersLib::WalkerData::register_itself();

	AppearanceControllersLib::ParticleModifiers::IntensityToNumberData::register_itself();
	AppearanceControllersLib::ParticleModifiers::ScaleEachData::register_itself();

	AppearanceControllersLib::Particles::CrumbsData::register_itself();
	AppearanceControllersLib::Particles::DecalData::register_itself();
	AppearanceControllersLib::Particles::DissolveData::register_itself();
	AppearanceControllersLib::Particles::DustData::register_itself();
	AppearanceControllersLib::Particles::EmitterData::register_itself();
	AppearanceControllersLib::Particles::FreeRangeData::register_itself();
	AppearanceControllersLib::Particles::LightningData::register_itself();
	AppearanceControllersLib::Particles::LocalGravityData::register_itself();
	AppearanceControllersLib::Particles::OrientationOffsetData::register_itself();
	AppearanceControllersLib::Particles::ScaleData::register_itself();
}

void AppearanceControllers::close_static()
{
	base::close_static();
}

