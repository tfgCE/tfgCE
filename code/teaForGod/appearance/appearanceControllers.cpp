#include "appearanceControllers.h"

#include "controllers\ac_a_containerJump.h"
#include "controllers\ac_a_tentacle.h"
#include "controllers\ac_blinds.h"
#include "controllers\ac_centipedeBackPlate.h"
#include "controllers\ac_demoRobot.h"
#include "controllers\ac_doorbot.h"
#include "controllers\ac_elevatorCartAttacherArm.h"
#include "controllers\ac_elevatorCartRotatingCore.h"
#include "controllers\ac_heartRoomBeams.h"
#include "controllers\ac_mushroomHeadSide.h"
#include "controllers\ac_pilgrimageEnvironmentRotation.h"
#include "controllers\ac_pilgrimsFakePointing.h"
#include "controllers\ac_pilgrimsGrabbingHand.h"
#include "controllers\ac_readyRestPointArm.h"
#include "controllers\ac_recoil.h"
#include "controllers\ac_restPointPlacement.h"
#include "controllers\ac_rotateRandomly.h"
#include "controllers\ac_weaponMixerWeapon.h"

#include "controllers\particles\acp_airTraffic.h"
#include "controllers\particles\acp_palmersCircles.h"

//

using namespace TeaForGodEmperor;

//

void AppearanceControllers::initialise_static()
{
	// lib
	AppearanceControllersLib::A_ContainerJumpData::register_itself();
	AppearanceControllersLib::A_TentacleData::register_itself();
	AppearanceControllersLib::BlindsData::register_itself();
	AppearanceControllersLib::CentipedeBackPlateData::register_itself();
	AppearanceControllersLib::DemoRobotData::register_itself();
	AppearanceControllersLib::DoorbotData::register_itself();
	AppearanceControllersLib::ElevatorCartAttacherArmData::register_itself();
	AppearanceControllersLib::ElevatorCartRotatingCoreData::register_itself();
	AppearanceControllersLib::HeartRoomBeamsData::register_itself();
	AppearanceControllersLib::MushroomHeadSideData::register_itself();
	AppearanceControllersLib::PilgrimageEnvironmentRotationData::register_itself();
	AppearanceControllersLib::PilgrimsFakePointingData::register_itself();
	AppearanceControllersLib::PilgrimsGrabbingHandData::register_itself();
	AppearanceControllersLib::ReadyRestPointArmData::register_itself();
	AppearanceControllersLib::RecoilData::register_itself();
	AppearanceControllersLib::RestPointPlacementData::register_itself();
	AppearanceControllersLib::RotateRandomlyData::register_itself();
	AppearanceControllersLib::WeaponMixerWeaponData::register_itself();

	// particles
	AppearanceControllersLib::Particles::AirTrafficData::register_itself();
	AppearanceControllersLib::Particles::PalmersCirclesData::register_itself();
}

