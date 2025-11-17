#include "modulePilgrim.h"

#include "moduleEquipment.h"
#include "moduleEXM.h"
#include "modulePilgrimData.h"
#include "modulePilgrimHand.h"
#include "equipment\me_gun.h"

#include "..\moduleMovementPilgrimKeeper.h"
#include "..\modulePresenceBackgroundMoverBase.h"
#include "..\modules.h"

#include "..\custom\mc_grabable.h"
#include "..\custom\mc_itemHolder.h"
#include "..\custom\mc_pickup.h"
#include "..\custom\health\mc_health.h"

#include "..\..\teaForGodTest.h"

#include "..\..\game\damage.h"
#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"
#include "..\..\game\gameLog.h"
#include "..\..\game\gameSettings.h"
#include "..\..\game\gameState.h"
#include "..\..\game\gameStats.h"
#include "..\..\game\playerSetup.h"
#include "..\..\game\screenShotContext.h"
#include "..\..\library\library.h"

#include "..\..\sound\subtitleSystem.h"

#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\vr\vrOffsets.h"

#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\game\gameUtils.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\item.h"
#include "..\..\..\framework\presence\presencePath.h"
#include "..\..\..\framework\presence\relativeToPresencePlacement.h"
#include "..\..\..\framework\world\presenceLink.h"
#include "..\..\..\framework\world\roomRegionVariables.inl"

#ifdef AN_ALLOW_BULLSHOTS
#include "..\..\..\framework\game\bullshotSystem.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifndef INVESTIGATE_SOUND_SYSTEM
	#ifdef AN_DEVELOPMENT_OR_PROFILER
		#ifdef AN_DEBUG_RENDERER
			#define DEBUG_DRAW_GRAB
		#endif
		//#define DEBUG_GRABBING
	#endif

	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		#define INSPECT_DESTROYING_EXMS
		#define INSPECT_DESTROYING_PILGRIM
		#define INSPECT_EQUIPMENT
		#define INSPECT_GEAR_CREATION
	#endif

	REMOVE_AS_SOON_AS_POSSIBLE_
	#ifndef INSPECT_EQUIPMENT
		#define INSPECT_EQUIPMENT
	#endif

	REMOVE_AS_SOON_AS_POSSIBLE_
	#ifndef INSPECT_GEAR_CREATION
		#define INSPECT_GEAR_CREATION
	#endif

	REMOVE_AS_SOON_AS_POSSIBLE_
	#ifndef INSPECT_EXMS
		#define INSPECT_EXMS
	#endif
#endif

//#define INSPECT_SHOWING_GUN_STATS_CALL_STACK

#define EQUIPMENT_RELEASED_WITHOUT_HOLDING__COUNT_TO_PROMPT 5

#define NON_PICKUPS_CAN_BE_PUT_INTO_POCKET false

//

using namespace TeaForGodEmperor;

//

// presence action info
DEFINE_STATIC_NAME(pilgrimNotUsing);

// module params
DEFINE_STATIC_NAME(handEnergyStarting);
DEFINE_STATIC_NAME(handEnergyMax);
DEFINE_STATIC_NAME(collisionFlagsForUsedEquipment);
DEFINE_STATIC_NAME(collisionFlagsForUsedEquipmentNonInteractive);
DEFINE_STATIC_NAME(collisionFlagsForUsedEquipmentToBeKept);
DEFINE_STATIC_NAME(alwaysAppearToHoldGun);
DEFINE_STATIC_NAME(handHoldGunVarID);
DEFINE_STATIC_NAME(handPressTriggerVarID);
DEFINE_STATIC_NAME(handAwayTriggerVarID);
DEFINE_STATIC_NAME(handHoldObjectVarID);
DEFINE_STATIC_NAME(handHoldObjectSizeVarID);
DEFINE_STATIC_NAME(objectsHeldObjectSizeVarID);
DEFINE_STATIC_NAME(handFistVarID);
DEFINE_STATIC_NAME(handGrabVarID);
DEFINE_STATIC_NAME(handGrabDialVarID);
DEFINE_STATIC_NAME(handStraightVarID);
DEFINE_STATIC_NAME(handPortActiveVarID);
DEFINE_STATIC_NAME(handDisplayActiveNormalVarID);
DEFINE_STATIC_NAME(handDisplayActiveExtraVarID);
DEFINE_STATIC_NAME(restPointHandObjectVarID);
DEFINE_STATIC_NAME(restPointEquipmentObjectVarID);
DEFINE_STATIC_NAME(restPointTVarID);
DEFINE_STATIC_NAME(restPointEmptyVarID);
DEFINE_STATIC_NAME(allowHandTrackingPoseVarID);
DEFINE_STATIC_NAME(relaxedThumbVarID);
DEFINE_STATIC_NAME(relaxedPointerVarID);
DEFINE_STATIC_NAME(relaxedMiddleVarID);
DEFINE_STATIC_NAME(relaxedRingVarID);
DEFINE_STATIC_NAME(relaxedPinkyVarID);
DEFINE_STATIC_NAME(straightThumbVarID);
DEFINE_STATIC_NAME(straightPointerVarID);
DEFINE_STATIC_NAME(straightMiddleVarID);
DEFINE_STATIC_NAME(straightRingVarID);
DEFINE_STATIC_NAME(straightPinkyVarID);
//
DEFINE_STATIC_NAME(handPoseBlendTime);
DEFINE_STATIC_NAME(fingerBlendTime);
DEFINE_STATIC_NAME(portFoldTime);
DEFINE_STATIC_NAME(displayFoldTime);
DEFINE_STATIC_NAME(restPointFoldTime);
//
DEFINE_STATIC_NAME(handPoseBlendTimeParalysed);
DEFINE_STATIC_NAME(fingerBlendTimeParalysed);
DEFINE_STATIC_NAME(portFoldTimeParalysed);
DEFINE_STATIC_NAME(displayFoldTimeParalysed);
DEFINE_STATIC_NAME(restPointFoldTimeParalysed);
//
DEFINE_STATIC_NAME(minPhysicalViolenceHandSpeed);
DEFINE_STATIC_NAME(minPhysicalViolenceHandSpeedStrong);
DEFINE_STATIC_NAME(physicalViolenceDamage);
DEFINE_STATIC_NAME(physicalViolenceDamageStrong);

DEFINE_STATIC_NAME(pilgrimEmpty);

// variables
DEFINE_STATIC_NAME(pocketsVerticalAdjustment);
DEFINE_STATIC_NAME(hadDisplayBoxMaterialIndex);
DEFINE_STATIC_NAME(pressedTrigger);
DEFINE_STATIC_NAME(lostRightHand);
DEFINE_STATIC_NAME(lostLeftHand);

// input
DEFINE_STATIC_NAME(grabEquipment);
DEFINE_STATIC_NAME(grabEquipmentLeft);
DEFINE_STATIC_NAME(grabEquipmentRight);
DEFINE_STATIC_NAME(pullEnergy);
DEFINE_STATIC_NAME(pullEnergyLeft);
DEFINE_STATIC_NAME(pullEnergyRight);
DEFINE_STATIC_NAME(pullEnergyAlt);
DEFINE_STATIC_NAME(pullEnergyAltLeft);
DEFINE_STATIC_NAME(pullEnergyAltRight);
DEFINE_STATIC_NAME(releaseEquipment);
DEFINE_STATIC_NAME(releaseEquipmentLeft);
DEFINE_STATIC_NAME(releaseEquipmentRight);
DEFINE_STATIC_NAME(sharedReleaseGrabDeploy);
DEFINE_STATIC_NAME(useEquipmentAway);
DEFINE_STATIC_NAME(useEquipmentAwayLeft);
DEFINE_STATIC_NAME(useEquipmentAwayRight);
DEFINE_STATIC_NAME(useEquipment);
DEFINE_STATIC_NAME(useEquipmentLeft);
DEFINE_STATIC_NAME(useEquipmentRight);
DEFINE_STATIC_NAME(useAsUsableEquipment);
DEFINE_STATIC_NAME(useAsUsableEquipmentLeft);
DEFINE_STATIC_NAME(useAsUsableEquipmentRight);
DEFINE_STATIC_NAME(useEXM);
DEFINE_STATIC_NAME(useEXMLeft);
DEFINE_STATIC_NAME(useEXMRight);
DEFINE_STATIC_NAME(makeFist);
DEFINE_STATIC_NAME(makeFistLeft);
DEFINE_STATIC_NAME(makeFistRight);
DEFINE_STATIC_NAME(deployMainEquipment);
DEFINE_STATIC_NAME(deployMainEquipmentLeft);
DEFINE_STATIC_NAME(deployMainEquipmentRight);
DEFINE_STATIC_NAME(autoHoldEquipment);
DEFINE_STATIC_NAME(autoMainEquipment);
DEFINE_STATIC_NAME(autoPointing);
DEFINE_STATIC_NAME(separateFingers);
DEFINE_STATIC_NAME(allowHandTrackingPose);
DEFINE_STATIC_NAME(relaxedThumb);
DEFINE_STATIC_NAME(relaxedPointer);
DEFINE_STATIC_NAME(relaxedMiddle);
DEFINE_STATIC_NAME(relaxedRing);
DEFINE_STATIC_NAME(relaxedPinky);
DEFINE_STATIC_NAME(straightThumb);
DEFINE_STATIC_NAME(straightPointer);
DEFINE_STATIC_NAME(straightMiddle);
DEFINE_STATIC_NAME(straightRing);
DEFINE_STATIC_NAME(straightPinky);
DEFINE_STATIC_NAME(easyReleaseEquipment);
DEFINE_STATIC_NAME(easyReleaseEquipmentLeft);
DEFINE_STATIC_NAME(easyReleaseEquipmentRight);
DEFINE_STATIC_NAME(requestInGameMenu);
DEFINE_STATIC_NAME(requestInGameMenuHold);
DEFINE_STATIC_NAME(showOverlayInfo);
DEFINE_STATIC_NAME(overlayInfoMove);

// bullshot input
DEFINE_STATIC_NAME(bullshotControlsMakeFistLeft);
DEFINE_STATIC_NAME(bullshotControlsMakeFistRight);

// collision flags
DEFINE_STATIC_NAME(equipmentInUse);
DEFINE_STATIC_NAME(equipmentInPocket);

// ai messages
//DEFINE_STATIC_NAME(pullEnergy);
DEFINE_STATIC_NAME(pilgrimBlackboardUpdated);
	DEFINE_STATIC_NAME(who);

// global references
DEFINE_STATIC_NAME_STR(grWeaponItemTypeToUseWithWeaponParts, TXT("weapon item type to use with weapon parts"));

// localised strings
DEFINE_STATIC_NAME_STR(lsWeaponsLocked, TXT("pi.ov.in; weapons locked"));

// colours
DEFINE_STATIC_NAME_STR(rcPilgrimageOverlayWarning, TXT("pilgrim_overlay_warning"));

// room tags
DEFINE_STATIC_NAME(itemHolderRoom);

// room/region variables
DEFINE_STATIC_NAME(infiniteHealthRegen);
DEFINE_STATIC_NAME(infiniteAmmoRegen);

// temporary objects
DEFINE_STATIC_NAME_STR(physicalViolence, TXT("physical violence"));
DEFINE_STATIC_NAME_STR(strongPhysicalViolence, TXT("strong physical violence"));
DEFINE_STATIC_NAME_STR(healthFromPhysicalViolence, TXT("health from physical violence"));
DEFINE_STATIC_NAME_STR(ammoFromPhysicalViolence, TXT("ammo from physical violence"));
DEFINE_STATIC_NAME_STR(littleHealthFromPhysicalViolence, TXT("little health from physical violence"));
DEFINE_STATIC_NAME_STR(littleAmmoFromPhysicalViolence, TXT("little ammo from physical violence"));

// tutorial highlights
DEFINE_STATIC_NAME_STR(tutBodyPocket, TXT("body:pocket"));
DEFINE_STATIC_NAME_STR(tutBodyWeapons, TXT("body:weapons"));

// mesh generator parameters / variables
DEFINE_STATIC_NAME_STR(withLeftRestPoint, TXT("with left rest point"));
DEFINE_STATIC_NAME_STR(withRightRestPoint, TXT("with right rest point"));

// spawned object variables
DEFINE_STATIC_NAME(hand);
	// values
	DEFINE_STATIC_NAME(left);
	DEFINE_STATIC_NAME(right);

// pilgrim overlay's reasons
DEFINE_STATIC_NAME(activeEXMs);
DEFINE_STATIC_NAME(itemEquipped);
DEFINE_STATIC_NAME(lookingAtGun);
DEFINE_STATIC_NAME(gunDamagedShortInfo);

// bullstot params
DEFINE_STATIC_NAME(useGearFromGameDefinition);
DEFINE_STATIC_NAME(useGearFromGameDefinitionIdx);

//

void ModulePilgrim::Element::blend_to_target(float _blendTime, float _deltaTime)
{
	state = blend_to_using_speed_based_on_time(state, target, _blendTime, _deltaTime);
}

//--

ModulePilgrim::Pocket::~Pocket()
{
	set_in_pocket(nullptr);
}

ModulePilgrim::Pocket::Pocket(Pocket const & _other)
{
	operator=(_other);
}

ModulePilgrim::Pocket& ModulePilgrim::Pocket::operator=(Pocket const & _other)
{
	isActive = _other.isActive;
	pocketLevel = _other.pocketLevel;
	name = _other.name;
	socket = _other.socket;
	side = _other.side;
	highlight = _other.highlight;
	materialParamIndex = _other.materialParamIndex;
	objectHoldSockets = _other.objectHoldSockets;

	set_in_pocket(_other.inPocket.get());

	return *this;
}

void ModulePilgrim::Pocket::set_in_pocket(Framework::IModulesOwner* _imo)
{
	if (auto* imo = inPocket.get())
	{
		setup_in_pocket_collides_with_flags_for(imo, false);
	}
	inPocket = _imo;
	if (auto* imo = inPocket.get())
	{
		setup_in_pocket_collides_with_flags_for(imo, true);
	}
}

void ModulePilgrim::Pocket::setup_in_pocket_collides_with_flags_for(Framework::IModulesOwner* _imo, bool _inPocket)
{
	if (auto* c = _imo->get_collision())
	{
		if (_inPocket)
		{
			c->push_collides_with_flags(NAME(equipmentInPocket), Collision::Flags::none());
		}
		else
		{
			c->pop_collides_with_flags(NAME(equipmentInPocket));
		}
		c->update_collidable_object();
	}
	if (auto* p = _imo->get_presence())
	{
		Concurrency::ScopedSpinLock lock(p->access_attached_lock());
		for_every_ptr(attached, p->get_attached())
		{
			setup_in_pocket_collides_with_flags_for(attached, _inPocket);
		}
	}
}

//--

GameState* ModulePilgrim::s_hackCreatingForGameState = nullptr;

REGISTER_FOR_FAST_CAST(ModulePilgrim);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModulePilgrim(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModulePilgrimData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModulePilgrim::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("pilgrim")), create_module, create_module_data);
}

ModulePilgrim::ModulePilgrim(Framework::IModulesOwner* _owner)
: base( _owner )
, pendingPilgrimSetup(Persistence::access_current_if_exists())
, pilgrimSetup(Persistence::access_current_if_exists())
{
	for_count(int, h, Hand::MAX)
	{
		lostHand[h] = false;
	}

	equipmentControls.use[0] = 0.0f;
	equipmentControls.use[1] = 0.0f;
	equipmentControls.useAsUsable[0] = 0.0f;
	equipmentControls.useAsUsable[1] = 0.0f;
	equipmentControls.change[0] = 0.0f;
	equipmentControls.change[1] = 0.0f;
	equipmentControls.dPad[0] = VectorInt2::zero;
	equipmentControls.dPad[1] = VectorInt2::zero;
	equipmentControlsPrev = equipmentControls;
	exmControls.use[0] = 0.0f;
	exmControls.use[1] = 0.0f;
	exmControlsPrev = exmControls;

	normalBlendTimes.handPoseBlendTime = 0.05f;
	normalBlendTimes.fingerBlendTime = 0.05f;
	normalBlendTimes.portFoldTime = 0.2f;
	normalBlendTimes.displayFoldTime = 0.5f;
	normalBlendTimes.restPointFoldTime = 0.2f;

	paralysedBlendTimes.handPoseBlendTime = 0.2f;
	paralysedBlendTimes.fingerBlendTime = 0.2f;
	paralysedBlendTimes.portFoldTime = 0.2f;
	paralysedBlendTimes.displayFoldTime = 0.2f;
	paralysedBlendTimes.restPointFoldTime = 0.2f;

	set_update_attached(true); // we should be updating attached objects
}

ModulePilgrim::~ModulePilgrim()
{
#ifdef INSPECT_DESTROYING_PILGRIM
	output(TXT("destroying pilgrim %S"), get_owner() ? get_owner()->ai_get_name().to_char() : TXT("??"));
#endif
	if (auto* g = Game::get_as<Game>())
	{
		if (g->access_player().get_actor() == get_owner())
		{
			if (auto* persistence = Persistence::access_current_if_exists())
			{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
				output(TXT("[persistence] store last setup on destruction of module pilgrim"));
#endif
				persistence->store_last_setup(this);
			}
		}
	}
	set_object_to_grab_proposition(Hand::Left, nullptr);
	set_object_to_grab_proposition(Hand::Right, nullptr);
	leftEXMs.cease_to_exists_and_clear();
	rightEXMs.cease_to_exists_and_clear();
	if (pilgrimKeeper.is_set())
	{
		pilgrimKeeper->be_autonomous();
		pilgrimKeeper->cease_to_exist(true);
	}
	if (leftMainEquipment.is_set())
	{
		leftMainEquipment->be_autonomous();
		leftMainEquipment->cease_to_exist(true);
	}
	if (rightMainEquipment.is_set())
	{
		rightMainEquipment->be_autonomous();
		rightMainEquipment->cease_to_exist(true);
	}
	if (leftHandEquipment.is_set())
	{
		leftHandEquipment->be_autonomous();
		leftHandEquipment->cease_to_exist(true);
	}
	if (rightHandEquipment.is_set())
	{
		rightHandEquipment->be_autonomous();
		rightHandEquipment->cease_to_exist(true);
	}
	if (leftHand.is_set())
	{
		leftHand->be_autonomous();
		leftHand->cease_to_exist(true);
	}
	if (rightHand.is_set())
	{
		rightHand->be_autonomous();
		rightHand->cease_to_exist(true);
	}
	if (leftHandForearmDisplay.is_set())
	{
		leftHandForearmDisplay->be_autonomous();
		leftHandForearmDisplay->cease_to_exist(true);
	}
	if (rightHandForearmDisplay.is_set())
	{
		rightHandForearmDisplay->be_autonomous();
		rightHandForearmDisplay->cease_to_exist(true);
	}
	if (leftRestPoint.is_set())
	{
		leftRestPoint->be_autonomous();
		leftRestPoint->cease_to_exist(true);
	}
	if (rightRestPoint.is_set())
	{
		rightRestPoint->be_autonomous();
		rightRestPoint->cease_to_exist(true);
	}
#ifdef INSPECT_DESTROYING_PILGRIM
	output(TXT("destroyed pilgrim %S"), get_owner() ? get_owner()->ai_get_name().to_char() : TXT("??"));
#endif
}

PhysicalViolence ModulePilgrim::read_physical_violence_from(Framework::ModuleData const* _moduleData)
{
	PhysicalViolence pv;
	pv.minSpeed = _moduleData->get_parameter<float>(nullptr, NAME(minPhysicalViolenceHandSpeed), 0.0f);
	pv.minSpeedStrong = _moduleData->get_parameter<float>(nullptr, NAME(minPhysicalViolenceHandSpeedStrong), 0.0f);
	pv.damage = Energy::get_from_module_data(nullptr, _moduleData, NAME(physicalViolenceDamage), Energy::zero());
	pv.damageStrong = Energy::get_from_module_data(nullptr, _moduleData, NAME(physicalViolenceDamageStrong), Energy::zero());
	return pv;
}

Energy ModulePilgrim::read_hand_energy_base_max_from(Framework::ModuleData const* _moduleData)
{
	Energy baseMaxEnergy = Energy::zero();
	baseMaxEnergy = Energy::get_from_module_data(nullptr, _moduleData, NAME(handEnergyMax), baseMaxEnergy);
	return baseMaxEnergy;
}

void ModulePilgrim::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);
	modulePilgrimData = fast_cast<ModulePilgrimData>(_moduleData);
	if (_moduleData)
	{
		pilgrimEmpty = _moduleData->get_parameter<bool>(this, NAME(pilgrimEmpty), pilgrimEmpty);

		leftHandEnergyStorage.startingEnergy = Energy::get_from_module_data(nullptr, _moduleData, NAME(handEnergyStarting), leftHandEnergyStorage.startingEnergy);
		leftHandEnergyStorage.baseMaxEnergy = Energy::get_from_module_data(nullptr, _moduleData, NAME(handEnergyMax), leftHandEnergyStorage.baseMaxEnergy);
		rightHandEnergyStorage.startingEnergy = leftHandEnergyStorage.startingEnergy;
		rightHandEnergyStorage.baseMaxEnergy = leftHandEnergyStorage.baseMaxEnergy;

		collisionFlagsForUsedEquipment = _moduleData->get_parameter<String>(this, NAME(collisionFlagsForUsedEquipment), collisionFlagsForUsedEquipment);
		if (! collisionFlagsForUsedEquipment.is_empty())
		{
			useCollisionFlagsForUsedEquipment = Collision::Flags::none();
			useCollisionFlagsForUsedEquipment.access().apply(collisionFlagsForUsedEquipment);
		}
		collisionFlagsForUsedEquipmentNonInteractive = _moduleData->get_parameter<String>(this, NAME(collisionFlagsForUsedEquipmentNonInteractive), collisionFlagsForUsedEquipmentNonInteractive);
		if (!collisionFlagsForUsedEquipmentNonInteractive.is_empty())
		{
			useCollisionFlagsForUsedEquipmentNonInteractive = Collision::Flags::none();
			useCollisionFlagsForUsedEquipmentNonInteractive.access().apply(collisionFlagsForUsedEquipmentNonInteractive);
		}
		collisionFlagsForUsedEquipmentToBeKept = _moduleData->get_parameter<String>(this, NAME(collisionFlagsForUsedEquipmentToBeKept), collisionFlagsForUsedEquipmentToBeKept);
		if (!collisionFlagsForUsedEquipmentToBeKept.is_empty())
		{
			useCollisionFlagsForUsedEquipmentToBeKept = Collision::Flags::none();
			useCollisionFlagsForUsedEquipmentToBeKept.access().apply(collisionFlagsForUsedEquipmentToBeKept);
		}
		alwaysAppearToHoldGun = _moduleData->get_parameter<bool>(this, NAME(alwaysAppearToHoldGun), alwaysAppearToHoldGun);
		handHoldGunVarID = _moduleData->get_parameter<Name>(this, NAME(handHoldGunVarID), handHoldGunVarID);
		handPressTriggerVarID = _moduleData->get_parameter<Name>(this, NAME(handPressTriggerVarID), handPressTriggerVarID);
		handAwayTriggerVarID = _moduleData->get_parameter<Name>(this, NAME(handAwayTriggerVarID), handAwayTriggerVarID);
		handHoldObjectVarID = _moduleData->get_parameter<Name>(this, NAME(handHoldObjectVarID), handHoldObjectVarID);
		handHoldObjectSizeVarID = _moduleData->get_parameter<Name>(this, NAME(handHoldObjectSizeVarID), handHoldObjectSizeVarID);
		objectsHeldObjectSizeVarID = _moduleData->get_parameter<Name>(this, NAME(objectsHeldObjectSizeVarID), objectsHeldObjectSizeVarID);
		handFistVarID = _moduleData->get_parameter<Name>(this, NAME(handFistVarID), handFistVarID);
		handGrabVarID = _moduleData->get_parameter<Name>(this, NAME(handGrabVarID), handGrabVarID);
		handGrabDialVarID = _moduleData->get_parameter<Name>(this, NAME(handGrabDialVarID), handGrabDialVarID);
		handStraightVarID = _moduleData->get_parameter<Name>(this, NAME(handStraightVarID), handStraightVarID);
		handPortActiveVarID = _moduleData->get_parameter<Name>(this, NAME(handPortActiveVarID), handPortActiveVarID);
		handDisplayActiveNormalVarID = _moduleData->get_parameter<Name>(this, NAME(handDisplayActiveNormalVarID), handDisplayActiveNormalVarID);
		handDisplayActiveExtraVarID = _moduleData->get_parameter<Name>(this, NAME(handDisplayActiveExtraVarID), handDisplayActiveExtraVarID);
		restPointHandObjectVarID = _moduleData->get_parameter<Name>(this, NAME(restPointHandObjectVarID), restPointHandObjectVarID);
		restPointEquipmentObjectVarID = _moduleData->get_parameter<Name>(this, NAME(restPointEquipmentObjectVarID), restPointEquipmentObjectVarID);
		restPointTVarID = _moduleData->get_parameter<Name>(this, NAME(restPointTVarID), restPointTVarID);
		restPointEmptyVarID = _moduleData->get_parameter<Name>(this, NAME(restPointEmptyVarID), restPointEmptyVarID);
		allowHandTrackingPoseVarID = _moduleData->get_parameter<Name>(this, NAME(allowHandTrackingPoseVarID), allowHandTrackingPoseVarID);
		relaxedThumbVarID = _moduleData->get_parameter<Name>(this, NAME(relaxedThumbVarID), relaxedThumbVarID);
		relaxedPointerVarID = _moduleData->get_parameter<Name>(this, NAME(relaxedPointerVarID), relaxedPointerVarID);
		relaxedMiddleVarID = _moduleData->get_parameter<Name>(this, NAME(relaxedMiddleVarID), relaxedMiddleVarID);
		relaxedRingVarID = _moduleData->get_parameter<Name>(this, NAME(relaxedRingVarID), relaxedRingVarID);
		relaxedPinkyVarID = _moduleData->get_parameter<Name>(this, NAME(relaxedPinkyVarID), relaxedPinkyVarID);
		straightThumbVarID = _moduleData->get_parameter<Name>(this, NAME(straightThumbVarID), straightThumbVarID);
		straightPointerVarID = _moduleData->get_parameter<Name>(this, NAME(straightPointerVarID), straightPointerVarID);
		straightMiddleVarID = _moduleData->get_parameter<Name>(this, NAME(straightMiddleVarID), straightMiddleVarID);
		straightRingVarID = _moduleData->get_parameter<Name>(this, NAME(straightRingVarID), straightRingVarID);
		straightPinkyVarID = _moduleData->get_parameter<Name>(this, NAME(straightPinkyVarID), straightPinkyVarID);
		//
		normalBlendTimes.handPoseBlendTime = _moduleData->get_parameter<float>(this, NAME(handPoseBlendTime), normalBlendTimes.handPoseBlendTime);
		normalBlendTimes.fingerBlendTime = _moduleData->get_parameter<float>(this, NAME(fingerBlendTime), normalBlendTimes.fingerBlendTime);
		normalBlendTimes.portFoldTime = _moduleData->get_parameter<float>(this, NAME(portFoldTime), normalBlendTimes.portFoldTime);
		normalBlendTimes.displayFoldTime = _moduleData->get_parameter<float>(this, NAME(displayFoldTime), normalBlendTimes.displayFoldTime);
		normalBlendTimes.restPointFoldTime = _moduleData->get_parameter<float>(this, NAME(restPointFoldTime), normalBlendTimes.restPointFoldTime);
		//
		paralysedBlendTimes.handPoseBlendTime = _moduleData->get_parameter<float>(this, NAME(handPoseBlendTimeParalysed), paralysedBlendTimes.handPoseBlendTime);
		paralysedBlendTimes.fingerBlendTime = _moduleData->get_parameter<float>(this, NAME(fingerBlendTimeParalysed), paralysedBlendTimes.fingerBlendTime);
		paralysedBlendTimes.portFoldTime = _moduleData->get_parameter<float>(this, NAME(portFoldTimeParalysed), paralysedBlendTimes.portFoldTime);
		paralysedBlendTimes.displayFoldTime = _moduleData->get_parameter<float>(this, NAME(displayFoldTimeParalysed), paralysedBlendTimes.displayFoldTime);
		paralysedBlendTimes.restPointFoldTime = _moduleData->get_parameter<float>(this, NAME(restPointFoldTimeParalysed), paralysedBlendTimes.restPointFoldTime);
		//
		minPhysicalViolenceHandSpeed = _moduleData->get_parameter<float>(this, NAME(minPhysicalViolenceHandSpeed), minPhysicalViolenceHandSpeed);
		minPhysicalViolenceHandSpeedStrong = _moduleData->get_parameter<float>(this, NAME(minPhysicalViolenceHandSpeedStrong), minPhysicalViolenceHandSpeedStrong);
		physicalViolenceDamage = Energy::get_from_module_data(this, _moduleData, NAME(physicalViolenceDamage), physicalViolenceDamage);
		physicalViolenceDamageStrong = Energy::get_from_module_data(this, _moduleData, NAME(physicalViolenceDamageStrong), physicalViolenceDamageStrong);

		pockets.clear();
		for_every(source, modulePilgrimData->pockets)
		{
			if (pockets.get_size() < MAX_POCKETS)
			{
				Pocket pocket;
				pocket.name = source->name;
				pocket.pocketLevel = source->pocketLevel;
				pocket.isActive = pocket.pocketLevel == 0;
				pocket.socket.set_name(source->socket);
				pocket.side = source->side;
				pocket.materialParamIndex = source->materialParamIndex;
				pocket.objectHoldSockets = source->objectHoldSockets;
				pockets.push_back(pocket);
			}
			else
			{
				error(TXT("increase MAX_POCKETS"));
			}
		}
		pocketsMaterialIndex = modulePilgrimData->pocketsMaterialIndex;
		pocketsMaterialParam = modulePilgrimData->pocketsMaterialParam;

		energyInhaleSocket.set_name(modulePilgrimData->energyInhaleSocket);

		handStates[Hand::Left].physicalViolenceDirSocket = modulePilgrimData->physicalViolenceDirSocket;
		handStates[Hand::Right].physicalViolenceDirSocket = modulePilgrimData->physicalViolenceDirSocket;
		forearms[Hand::Left].forearmStartSocket = modulePilgrimData->leftForearm.forearmStartSocket;
		forearms[Hand::Left].forearmEndSocket = modulePilgrimData->leftForearm.forearmEndSocket;
		forearms[Hand::Left].handOverSocket = modulePilgrimData->leftForearm.handOverSocket;
		forearms[Hand::Right].forearmStartSocket = modulePilgrimData->rightForearm.forearmStartSocket;
		forearms[Hand::Right].forearmEndSocket = modulePilgrimData->rightForearm.forearmEndSocket;
		forearms[Hand::Right].handOverSocket = modulePilgrimData->rightForearm.handOverSocket;
	}
}

void ModulePilgrim::look_up_sockets()
{
	if (auto * mesh = get_owner()->get_appearance()->get_mesh())
	{
		energyInhaleSocket.look_up(mesh);
		for_every(pocket, pockets)
		{
			pocket->socket.look_up(mesh);
		}
		for_count(int, i, Hand::MAX)
		{
			forearms[i].forearmStartSocket.look_up(mesh, AllowToFail);
			forearms[i].forearmEndSocket.look_up(mesh, AllowToFail);
		}
	}
}

void ModulePilgrim::store_for_game_state(PilgrimGear* _gear, OUT_ REF_ bool& _atLeastHalfHealth) const
{
	_gear->clear();

	// keep safe pointers to objects, should not get destroyed as destruction is suspended at this point
	SafePtr<Framework::IModulesOwner> inHand[Hand::MAX];
	ArrayStatic<SafePtr<Framework::IModulesOwner>, MAX_POCKETS> inPockets;

	// lock for as quickly as possible
	{
		MODULE_OWNER_LOCK(TXT("ModulePilgrim::store_for_game_state"));
#ifdef BUILD_PREVIEW
		::System::ScopedTimeStampOutput stso(TXT("ModulePilgrim::store_for_game_state"));
#endif

		_atLeastHalfHealth = false;
		// store initial energy levels
		{
			if (auto* h = get_owner()->get_custom<CustomModules::Health>())
			{
				_gear->initialEnergyLevels.health = h->get_total_health();
				_atLeastHalfHealth = h->get_total_health() >= (h->get_max_total_health() / 2);
			}
			_gear->initialEnergyLevels.hand[Hand::Left] = leftHandEnergyStorage.energy;
			_gear->initialEnergyLevels.hand[Hand::Right] = rightHandEnergyStorage.energy;
		}

		{
			Concurrency::ScopedMRSWLockRead lock(pilgrimInventory.exmsLock);
			for_every(exm, pilgrimInventory.availableEXMs)
			{
				_gear->unlockedEXMs.push_back_unique(*exm);
			}
			for_every(exm, pilgrimInventory.permanentEXMs)
			{
				_gear->permanentEXMs.push_back(*exm);
			}
		}

		for_count(int, hIdx, Hand::MAX)
		{
			Hand::Type hand = (Hand::Type)hIdx;
			auto& handSrc = pendingPilgrimSetup.get_hand(hand);
			auto& handDst = _gear->hands[hIdx];
			handDst.clear();
			//
			for_every(exm, handSrc.passiveEXMs)
			{
				an_assert_log_always(handDst.passiveEXMs.has_place_left());
				handDst.passiveEXMs.push_back(*exm);
			}
			handDst.activeEXM = handSrc.activeEXM;
			if (!handSrc.weaponSetup.is_empty())
			{
				handDst.weaponSetup.copy_for_different_persistence(handSrc.weaponSetup, true);
			}
		}

		//

		// in hand
		for_count(int, hIdx, Hand::MAX)
		{
			Hand::Type hand = (Hand::Type)hIdx;
			if (auto* eq = get_hand_equipment(hand))
			{
				_gear->hands[hIdx].inHand.mainWeapon = eq == get_main_equipment(hand);
				if (!_gear->hands[hIdx].inHand.mainWeapon)
				{
					inHand[hIdx] = eq;
				}
			}
		}

		// pockets
		inPockets.set_size(pockets.get_size());
		for_every(pocket, pockets)
		{
			int i = for_everys_index(pocket);
			if (i < MAX_POCKETS)
			{
				inPockets[i] = pocket->get_in_pocket();
			}
		}
	}

	// now store the objects as we unlocked pilgrim
	{
#ifdef BUILD_PREVIEW
		::System::ScopedTimeStampOutput stso(TXT("store objects in game state"));
#endif

		for_count(int, hIdx, Hand::MAX)
		{
			if (auto* eq = inHand[hIdx].get())
			{
				_gear->hands[hIdx].inHand.store_object(eq);
			}
		}

		for_every(inPocket, inPockets)
		{
			int i = for_everys_index(inPocket);
			if (auto* eq = inPocket->get())
			{
				_gear->pockets[i].store_object(eq);
			}
		}
	}
}

void ModulePilgrim::initialise()
{
	base::initialise();

	trace(TXT("initialise pilgrim"));

	overlayInfo.set_owner(get_owner());

	blockControlsTimeLeft = 0.5f; // in case we started with pulling trigger

	RefCountObjectPtr<PilgrimGear> startingGear;

	struct CopyWeaponSetup
	{
		static void copy(PlayerGameSlot* pgs, Random::Generator& weaponPartRG,
			WeaponSetup const & srcWeaponSetup, WeaponSetupTemplate const & srcWeaponSetupTemplate,
			WeaponSetup & dstWeaponSetup)
		{
			dstWeaponSetup.clear();
			if (!srcWeaponSetup.is_empty())
			{
				dstWeaponSetup.copy_for_different_persistence(srcWeaponSetup);
			}
			else if (!srcWeaponSetupTemplate.is_empty())
			{
				for_every(wp, srcWeaponSetupTemplate.get_parts())
				{
					auto* wpi = wp->create_weapon_part(&pgs->persistence, weaponPartRG.get_int());
					dstWeaponSetup.add_part(wp->at, wpi, wp->nonRandomised, wp->damaged, wp->defaultMeshParams);
				}
			}
		}
	};

	{
		bool pilgrimSet = false;

		bool allowPilgrimageToModifyStartingSetup = false;

		if (pilgrimEmpty)
		{
			pilgrimSet = true;
		}

		if (!pilgrimSet)
		{
			if (auto* ps = PlayerSetup::access_current_if_exists())
			{
				if (auto* pgs = ps->access_active_game_slot())
				{
#ifdef AN_ALLOW_BULLSHOTS
					if (Framework::BullshotSystem::is_active())
					{
						Concurrency::ScopedSpinLock lock(Framework::BullshotSystem::access_lock());
						auto& vars = Framework::BullshotSystem::access_variables();
						if (auto* gdName = vars.get_existing<Framework::LibraryName>(NAME(useGearFromGameDefinition)))
						{
							if (auto* lib = Library::get_current_as<Library>())
							{
								if (auto* gd = lib->get_game_definitions().find(*gdName))
								{
									startingGear = gd->get_starting_gear(vars.get_value<int>(NAME(useGearFromGameDefinitionIdx), 0));
								}
							}
						}
					}
#endif
					if (!startingGear.is_set())
					{
						if (auto* gs = s_hackCreatingForGameState)
						{
							startingGear = gs->gear.get();
						}
					}
					if (!startingGear.is_set())
					{
						if (auto* gd = pgs->get_game_definition())
						{
							startingGear = gd->get_default_starting_gear();
							allowPilgrimageToModifyStartingSetup = true;
						}
					}
					if (startingGear.is_set())
					{
						// copy initial energy levels
						{
							pilgrimSetup.set_initial_energy_levels(startingGear->initialEnergyLevels);
							pendingPilgrimSetup.set_initial_energy_levels(startingGear->initialEnergyLevels);
						}
						{
							Concurrency::ScopedMRSWLockWrite lock(pilgrimInventory.exmsLock);
							for_every(exm, startingGear->unlockedEXMs)
							{
								pilgrimInventory.availableEXMs.push_back_unique(*exm);
							}
							for_every(exm, startingGear->permanentEXMs)
							{
								pilgrimInventory.permanentEXMs.push_back(*exm);
							}
						}
						Random::Generator weaponPartRG = get_owner()->get_individual_random_generator();
						weaponPartRG.advance_seed(34957, 29508);
						for_count(int, hIdx, Hand::MAX)
						{
							Hand::Type hand = (Hand::Type)hIdx;
							auto& handSrc = startingGear->hands[hIdx];
							auto& handDst = pendingPilgrimSetup.access_hand(hand);
							//
							{
								Concurrency::ScopedSpinLock lock(handDst.exmsLock, true);
								handDst.copy_exms_from(handSrc.passiveEXMs, handSrc.activeEXM);
							}
							CopyWeaponSetup::copy(pgs, weaponPartRG, handSrc.weaponSetup, handSrc.weaponSetupTemplate, handDst.weaponSetup);
						}
						pilgrimSet = true;
					}
				}
			}
		}

		if (!pilgrimSet)
		{
			if (auto* ps = PlayerSetup::access_current_if_exists())
			{
				pendingPilgrimSetup.copy_from(ps->get_pilgrim_setup());
				pilgrimSet = true;
			}
		}

		if (allowPilgrimageToModifyStartingSetup)
		{
			if (auto* pi = PilgrimageInstance::get())
			{
				pi->adjust_starting_pilgrim_setup(pendingPilgrimSetup);
			}
		}

		update_exms_in_inventory();

		{
			upToDateWithPilgrimSetup = false;
		}
	}

	look_up_sockets();

	if (modulePilgrimData)
	{
		if (auto * ownerAsObject = get_owner_as_object())
		{
			Game::get()->push_activation_group(ownerAsObject->get_activation_group());
			Random::Generator rg = ownerAsObject->get_individual_random_generator();
			// hands
			for_count(int, i, Hand::MAX)
			{
				Hand::Type iAsHand = (Hand::Type)i;
				if (! lostHand[i])
				{
					SafePtr<Framework::IModulesOwner> & hand = i == Hand::Left ? leftHand : rightHand;
					if (auto * handType = modulePilgrimData->get_hand(iAsHand).actorType.get())
					{
						handType->load_on_demand_if_required();

						Game::get_as<Game>()->perform_sync_world_job(TXT("create hand"), [&hand, iAsHand, handType, ownerAsObject]()
						{
							hand = handType->create(String(iAsHand == Hand::Left ? TXT("left hand") : TXT("right hand")));
							hand->init(ownerAsObject->get_in_sub_world());
						});
						hand->access_individual_random_generator() = rg.spawn();
						hand->initialise_modules();
						hand->set_instigator(ownerAsObject);
						hand->be_non_autonomous();
						if (auto* mesh = hand->get_appearance()->get_mesh())
						{
							forearms[i].handOverSocket.look_up(mesh);
							handStates[i].physicalViolenceDirSocket.look_up(mesh);
						}
						if (modulePilgrimData->get_hand(iAsHand).attachToSocket.is_valid())
						{
							hand->get_presence()->attach_to_socket(ownerAsObject, modulePilgrimData->get_hand(iAsHand).attachToSocket, true);
						}
						else
						{
							hand->get_presence()->attach_to(ownerAsObject);
						}
						if (auto* pilgrimHand = hand->get_gameplay_as<ModulePilgrimHand>())
						{
							pilgrimHand->set_pilgrim(get_owner(), iAsHand);
							pilgrimHand->set_health_energy_transfer(get_owner());
						}
					}
				}
				if (!lostHand[i])
				{
					SafePtr<Framework::IModulesOwner> & forearmDisplay = i == Hand::Left ? leftHandForearmDisplay : rightHandForearmDisplay;
					bool attachDisplayToHand = false;
					auto const & forearmDisplayAttach = modulePilgrimData->get_hand_forearm_display(iAsHand, OUT_ attachDisplayToHand);
					if (!attachDisplayToHand || get_hand(iAsHand))
					{
						if (auto * forearmDisplayType = forearmDisplayAttach.actorType.get())
						{
							forearmDisplayType->load_on_demand_if_required();

							Game::get_as<Game>()->perform_sync_world_job(TXT("create forearm display"), [&forearmDisplay, iAsHand, forearmDisplayType, ownerAsObject]()
							{
								forearmDisplay = forearmDisplayType->create(String(iAsHand == Hand::Left ? TXT("left fad") : TXT("right fad")));
								forearmDisplay->init(ownerAsObject->get_in_sub_world());
							});
							forearmDisplay->access_individual_random_generator() = rg.spawn();
							forearmDisplay->access_variables().access<Name>(NAME(hand)) = iAsHand == Hand::Left? NAME(left) : NAME(right);
							forearmDisplay->initialise_modules();
							forearmDisplay->set_instigator(ownerAsObject);
							forearmDisplay->be_non_autonomous();
							Framework::IModulesOwner* attachTo = ownerAsObject;
							if (attachDisplayToHand)
							{
								attachTo = get_hand(iAsHand);
							}
							if (forearmDisplayAttach.attachToSocket.is_valid())
							{
								forearmDisplay->get_presence()->attach_to_socket(attachTo, forearmDisplayAttach.attachToSocket, true);
							}
							else
							{
								forearmDisplay->get_presence()->attach_to(attachTo);
							}
						}
					}
				}
				if (!lostHand[i])
				{
					SafePtr<Framework::IModulesOwner> & restPoint = i == Hand::Left ? leftRestPoint : rightRestPoint;
					// rest points
					// IMPORTANT NOTE
					// they are created and attached after hands which will make them updated in this order (hands, rest points)
					// it is important as rest points use hands' sockets 
					if (auto * restPointType = modulePilgrimData->get_rest_point(iAsHand).actorType.get())
					{
						restPointType->load_on_demand_if_required();

						Game::get_as<Game>()->perform_sync_world_job(TXT("create left rest point"), [&restPoint, iAsHand, restPointType, ownerAsObject]()
						{
							restPoint = restPointType->create(String(iAsHand == Hand::Left ? TXT("left rest point") : TXT("right rest point")));
							restPoint->init(ownerAsObject->get_in_sub_world());
						});
						restPoint->access_individual_random_generator() = rg.spawn();
						restPoint->access_variables().access<Name>(NAME(hand)) = iAsHand == Hand::Left ? NAME(left) : NAME(right);
						restPoint->initialise_modules();
						restPoint->set_instigator(ownerAsObject);
						restPoint->be_non_autonomous();
						if (modulePilgrimData->get_rest_point(iAsHand).attachToSocket.is_valid())
						{
							restPoint->get_presence()->attach_to_socket(ownerAsObject, modulePilgrimData->get_rest_point(iAsHand).attachToSocket, false);
						}
						else
						{
							restPoint->get_presence()->attach_to(ownerAsObject);
						}
					}
				}
			}

			// variables
			for_count(int, i, Hand::MAX)
			{
				Hand::Type iAsHand = (Hand::Type)i;

				auto & handState = handStates[i];

				if (auto* hand = get_hand(iAsHand))
				{
					if (handHoldGunVarID.is_valid())
					{
						handState.handHoldGunVarID = hand->access_variables().find<float>(handHoldGunVarID);
					}
					if (handPressTriggerVarID.is_valid())
					{
						handState.handPressTriggerVarID = hand->access_variables().find<float>(handPressTriggerVarID);
					}
					if (handAwayTriggerVarID.is_valid())
					{
						handState.handAwayTriggerVarID = hand->access_variables().find<float>(handAwayTriggerVarID);
					}
					if (handHoldObjectVarID.is_valid())
					{
						handState.handHoldObjectVarID = hand->access_variables().find<float>(handHoldObjectVarID);
					}
					if (handHoldObjectSizeVarID.is_valid())
					{
						handState.handHoldObjectSizeVarID = hand->access_variables().find<float>(handHoldObjectSizeVarID);
					}
					if (handFistVarID.is_valid())
					{
						handState.handFistVarID = hand->access_variables().find<float>(handFistVarID);
					}
					if (handGrabVarID.is_valid())
					{
						handState.handGrabVarID = hand->access_variables().find<float>(handGrabVarID);
					}
					if (handGrabDialVarID.is_valid())
					{
						handState.handGrabDialVarID = hand->access_variables().find<float>(handGrabDialVarID);
					}
					if (handStraightVarID.is_valid())
					{
						handState.handStraightVarID = hand->access_variables().find<float>(handStraightVarID);
					}
					if (handPortActiveVarID.is_valid())
					{
						handState.handPortActiveVarID = hand->access_variables().find<float>(handPortActiveVarID);
					}
					if (handDisplayActiveNormalVarID.is_valid())
					{
						handState.handDisplayActiveNormalVarID = hand->access_variables().find<float>(handDisplayActiveNormalVarID);
					}
					if (handDisplayActiveExtraVarID.is_valid())
					{
						handState.handDisplayActiveExtraVarID = hand->access_variables().find<float>(handDisplayActiveExtraVarID);
					}
					if (allowHandTrackingPoseVarID.is_valid())
					{
						handState.allowHandTrackingPoseVarID = hand->access_variables().find<float>(allowHandTrackingPoseVarID);
					}
					if (relaxedThumbVarID.is_valid())
					{
						handState.relaxedThumbVarID = hand->access_variables().find<float>(relaxedThumbVarID);
					}
					if (relaxedPointerVarID.is_valid())
					{
						handState.relaxedPointerVarID = hand->access_variables().find<float>(relaxedPointerVarID);
					}
					if (relaxedMiddleVarID.is_valid())
					{
						handState.relaxedMiddleVarID = hand->access_variables().find<float>(relaxedMiddleVarID);
					}
					if (relaxedRingVarID.is_valid())
					{
						handState.relaxedRingVarID = hand->access_variables().find<float>(relaxedRingVarID);
					}
					if (relaxedPinkyVarID.is_valid())
					{
						handState.relaxedPinkyVarID = hand->access_variables().find<float>(relaxedPinkyVarID);
					}
					if (straightThumbVarID.is_valid())
					{
						handState.straightThumbVarID = hand->access_variables().find<float>(straightThumbVarID);
					}
					if (straightPointerVarID.is_valid())
					{
						handState.straightPointerVarID = hand->access_variables().find<float>(straightPointerVarID);
					}
					if (straightMiddleVarID.is_valid())
					{
						handState.straightMiddleVarID = hand->access_variables().find<float>(straightMiddleVarID);
					}
					if (straightRingVarID.is_valid())
					{
						handState.straightRingVarID = hand->access_variables().find<float>(straightRingVarID);
					}
					if (straightPinkyVarID.is_valid())
					{
						handState.straightPinkyVarID = hand->access_variables().find<float>(straightPinkyVarID);
					}
				}

				if (auto* restPoint = get_rest_point(iAsHand))
				{
					if (restPointHandObjectVarID.is_valid())
					{
						handState.restPointHandObjectVarID = restPoint->access_variables().find<SafePtr<Framework::IModulesOwner>>(restPointHandObjectVarID);
					}
					if (restPointEquipmentObjectVarID.is_valid())
					{
						handState.restPointEquipmentObjectVarID = restPoint->access_variables().find<SafePtr<Framework::IModulesOwner>>(restPointEquipmentObjectVarID);
					}
					if (restPointTVarID.is_valid())
					{
						handState.restPointTVarID = restPoint->access_variables().find<float>(restPointTVarID);
					}
					if (restPointEmptyVarID.is_valid())
					{
						handState.restPointEmptyVarID = restPoint->access_variables().find<float>(restPointEmptyVarID);
					}
				}
			}

			// pilgrim keeper (only if we use vr)
#ifndef AN_DEVELOPMENT_OR_PROFILER
			if (VR::IVR::get())
#endif
			{
				if (auto* pilgrimKeeperActorType = modulePilgrimData->pilgrimKeeperActorType.get())
				{
					pilgrimKeeperActorType->load_on_demand_if_required();

					Framework::IModulesOwner* pk = nullptr;
					Game::get_as<Game>()->perform_sync_world_job(TXT("create pilgrim keeper"), [&pk, pilgrimKeeperActorType, ownerAsObject]()
						{
							pk = pilgrimKeeperActorType->create(String(TXT("pilgrim keeper")));
							pk->init(ownerAsObject->get_in_sub_world());
						});
					pk->access_individual_random_generator() = Random::Generator(1, 2);
					pk->initialise_modules();
					pk->set_instigator(ownerAsObject);
					pk->be_non_autonomous();
					if (auto* pkm = fast_cast<ModuleMovementPilgrimKeeper>(pk->get_movement()))
					{
						pkm->set_pilgrim(get_owner());
					}
					pilgrimKeeper = pk;
				}
			}

			Game::get()->pop_activation_group(ownerAsObject->get_activation_group());
		}
	}

	advance_hands(1.0f);

	recreate_mesh_accordingly_to_pending_setup(true);
	create_things_accordingly_to_pending_pilgrim_setup(true);

	{
		// create items and weapons for pockets/in hands
		// do it post create_things_accordingly_to_pending_pilgrim_setup as we'll be putting things into hands
		// and create_things_accordingly_to_pending_pilgrim_setup puts into hand and takes it back
		if (startingGear.is_set())
		{
			if (auto* ownerAsObject = get_owner_as_object())
			{
				scoped_call_stack_info(TXT("create starting gear"));

				Game::get()->push_activation_group(ownerAsObject->get_activation_group());

				for_count(int, hIdx, Hand::MAX)
				{
					if (lostHand[hIdx])
					{
						continue;
					}
					Hand::Type hand = (Hand::Type)hIdx;
					auto& inHand = startingGear->hands[hIdx].inHand;
					if (inHand.mainWeapon)
					{
						scoped_call_stack_info(TXT("using main equipment"));
						use(hand, get_main_equipment(hand));
					}
					else if (inHand.is_used())
					{
						scoped_call_stack_info_str_printf(TXT("create starting gear for inHand %i"), hIdx);
						if (auto* gearObject = inHand.async_create_object(ownerAsObject))
						{
							use(hand, gearObject);
						}
					}
				}

				for_count(int, i, MAX_POCKETS)
				{
					scoped_call_stack_info_str_printf(TXT("create starting gear for pocket %i"), i);
					auto& inPocket = startingGear->pockets[i];
					if (inPocket.is_used() && pockets.is_index_valid(i))
					{
						if (auto* gearObject = inPocket.async_create_object(ownerAsObject))
						{
							put_into_pocket(&pockets[i], gearObject);
						}
					}
				}

				Game::get()->pop_activation_group(ownerAsObject->get_activation_group());
			}
		}
	}
}

void ModulePilgrim::activate()
{
	base::activate();

	// reput into pocket on activation (useful for restoring from a game state to fix collision flights on buttons)
	for_every(pocket, pockets)
	{
		if (auto* inPocket = pocket->get_in_pocket())
		{
			put_into_pocket(pocket, inPocket);
		}
	}
}

void ModulePilgrim::clear_extra_exms_in_inventory()
{
	ASSERT_SYNC_OR_ASYNC;

	Array<Name> exmsInUse;

	// get exms in use (active + passive)
	{
		for_count(int, iHand, Hand::MAX)
		{
			auto& handSetup = pilgrimSetup.access_hand((Hand::Type)iHand);
			Concurrency::ScopedSpinLock lock(handSetup.exmsLock);
			if (handSetup.activeEXM.is_set())
			{
				exmsInUse.push_back_unique(handSetup.activeEXM.exm);
			}
			for_every(exm, handSetup.passiveEXMs)
			{
				if (exm->is_set())
				{
					exmsInUse.push_back_unique(exm->exm);
				}
			}
		}
	}

	// go through available exms (active and passive) and remove ones that we don't have
	{
		Concurrency::ScopedMRSWLockWrite lockPI(pilgrimInventory.exmsLock);

		for (int i = 0; i < pilgrimInventory.availableEXMs.get_size(); ++i)
		{
			bool removeThisOne = false;
			auto exmName = pilgrimInventory.availableEXMs[i];
			if (auto* exm = EXMType::find(exmName))
			{
				if (!exm->is_permanent() &&
					!exm->is_integral())
				{
					if (!exmsInUse.does_contain(exmName))
					{
						removeThisOne = true;
					}
				}
			}
			if (removeThisOne)
			{
				pilgrimInventory.availableEXMs.remove_at(i);
				--i;
			}
		}
	}
}

void ModulePilgrim::update_exms_in_inventory()
{
	ASSERT_SYNC_OR_ASYNC;

	if (auto* persistence = Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lockP(persistence->access_lock());
		Concurrency::ScopedMRSWLockWrite lockPI(pilgrimInventory.exmsLock);

		persistence->cache_exms();

		for_every(exm, persistence->get_unlocked_exms())
		{
			pilgrimInventory.availableEXMs.push_back_unique(*exm);
		}

		// for permanent we have to check whether a given EXM has already been added
		for_every(exm, persistence->get_permanent_exms())
		{
			// get idx of the same type of permanent exms, as unlocked
			int permIdx = 0;
			for_count(int, i, for_everys_index(exm))
			{
				if (persistence->get_permanent_exms()[i] == *exm)
				{
					++permIdx;
				}
			}
			// check how many of such exms are already unlocked in the inventory
			int unlocked = 0;
			for_every(piexm, pilgrimInventory.permanentEXMs)
			{
				if (*piexm == *exm)
				{
					++unlocked;
				}
			}
			if (unlocked <= permIdx)
			{
				pilgrimInventory.permanentEXMs.push_back(*exm);
			}
		}
	}
}

void ModulePilgrim::reset_energy_state()
{
	redistribute_energy(true);
}

void ModulePilgrim::redistribute_energy_to_health(Energy const& _minHealthEnergy, bool _fillUpHealthFromRegen, bool _makeUpEnergyIfNotEnough)
{
	if (auto* h = get_owner()->get_custom<CustomModules::Health>())
	{
		Energy newEnergyLevel = _minHealthEnergy;
		h->sanitise_energy_levels();
		h->update_super_health_storage();
		if (h->get_total_health() < newEnergyLevel)
		{
			// get some health form hand storages to health
			Energy availableEnergy = leftHandEnergyStorage.energy + rightHandEnergyStorage.energy;
			Energy missingHealthEnergy = newEnergyLevel - h->get_total_health();
			if (missingHealthEnergy >= availableEnergy)
			{
				if (!_makeUpEnergyIfNotEnough)
				{
					newEnergyLevel = h->get_total_health() + leftHandEnergyStorage.energy + rightHandEnergyStorage.energy;
				}
				// leave no energy in storages
				leftHandEnergyStorage.energy = Energy::zero();
				rightHandEnergyStorage.energy = Energy::zero();
			}
			else
			{
				EnergyCoef drainLeftCoef = missingHealthEnergy.div_to_coef(availableEnergy);
				Energy drainLeft = clamp(leftHandEnergyStorage.energy.adjusted(drainLeftCoef), Energy::zero(), leftHandEnergyStorage.energy);
				Energy drainRight = clamp(missingHealthEnergy - drainLeft, Energy::zero(), rightHandEnergyStorage.energy);
				drainLeft = clamp(missingHealthEnergy - drainRight, Energy::zero(), leftHandEnergyStorage.energy); // if a bit is missing
				an_assert(drainLeft + drainRight == missingHealthEnergy);
				leftHandEnergyStorage.energy = max(Energy::zero(), leftHandEnergyStorage.energy - drainLeft);
				rightHandEnergyStorage.energy = max(Energy::zero(), rightHandEnergyStorage.energy - drainRight);
			}
			h->reset_health(newEnergyLevel, false);
		}

		if (_fillUpHealthFromRegen)
		{
			if (h->get_health() < h->get_max_health())
			{
				h->reset_health(h->get_total_health(), false);
			}
		}
	}
}

void ModulePilgrim::redistribute_energy(bool _resetEnergyState)
{
	bool justResetEnergyState = _resetEnergyState;
	if (! advancedAtLeastOnce || (get_owner_as_object() && !get_owner_as_object()->is_world_active()))
	{
		justResetEnergyState = true;
	}

	if (auto* h = get_owner()->get_custom<CustomModules::Health>())
	{
		h->update_super_health_storage();
	}

	// get "at least" levels
	PilgrimInitialEnergyLevels useEnergyLevels;

	if (justResetEnergyState)
	{
		// DV - default value
		Energy healthDV = Energy::one();
		Energy handDV[Hand::MAX] = { Energy::one(), Energy::one() };
		if (auto* h = get_owner()->get_custom<CustomModules::Health>())
		{
			healthDV = h->get_max_total_health();
		}
		for_count(int, iHand, Hand::MAX)
		{
			handDV[iHand] = get_hand_energy_max_storage(/* as is */(Hand::Type)iHand);
		}

		useEnergyLevels = pilgrimSetup.get_initial_energy_levels().set_defaults_if_missing(healthDV, handDV[Hand::Left], handDV[Hand::Right]);
	}

	int triesLeft = 10;
	while (triesLeft > 0)
	{
		--triesLeft;

		Energy excessEnergy = Energy::zero();

		// reset health, main equipment
		if (auto * h = get_owner()->get_custom<CustomModules::Health>())
		{
			if (justResetEnergyState)
			{
				h->reset_energy_state(useEnergyLevels.health);
			}
			else
			{
				h->redistribute_energy(REF_ excessEnergy);
			}
		}

		for_count(int, iHand, Hand::MAX)
		{
			auto hand = (Hand::Type)iHand;
			auto& handEnergyStorage = iHand == Hand::Left ? leftHandEnergyStorage : rightHandEnergyStorage;
			if (justResetEnergyState)
			{
				handEnergyStorage.maxEnergy = handEnergyStorage.baseMaxEnergy + pilgrimBlackboard.get_hand_energy_storage(get_owner(), hand);
				if (!GameSettings::get().difficulty.commonHandEnergyStorage)
				{
					if (lostHand[iHand])
					{
						handEnergyStorage.maxEnergy = Energy::zero();
					}
				}
				handEnergyStorage.energy = clamp(useEnergyLevels.hand[hand]/*handEnergyStorage.startingEnergy*/, Energy::zero(), handEnergyStorage.maxEnergy);
			}
			if (handEnergyStorage.energy > handEnergyStorage.maxEnergy)
			{
				excessEnergy += handEnergyStorage.energy - handEnergyStorage.maxEnergy;
				handEnergyStorage.energy = handEnergyStorage.maxEnergy;
			}
		}

		if (justResetEnergyState)
		{
			break;
		}
		else if (excessEnergy.is_positive())
		{
			// reset health, main equipment
			if (auto* h = get_owner()->get_custom<CustomModules::Health>())
			{
				h->add_energy(REF_ excessEnergy);
			}

			Energy missingEnergyLeft = leftHandEnergyStorage.maxEnergy - leftHandEnergyStorage.energy;
			Energy missingEnergyRight = rightHandEnergyStorage.maxEnergy - rightHandEnergyStorage.energy;
			if (excessEnergy > missingEnergyLeft + missingEnergyRight)
			{
				leftHandEnergyStorage.energy = leftHandEnergyStorage.maxEnergy;
				rightHandEnergyStorage.energy = rightHandEnergyStorage.maxEnergy;
			}
			else if (missingEnergyLeft + missingEnergyRight > Energy::zero())
			{
				// fill up proportionally
				EnergyCoef fillLeftCoef = missingEnergyLeft.div_to_coef(missingEnergyLeft + missingEnergyRight);
				Energy fillLeft = excessEnergy.adjusted(fillLeftCoef);
				Energy fillRight = max(Energy::zero(), excessEnergy - fillLeft);
				leftHandEnergyStorage.energy += fillLeft;
				rightHandEnergyStorage.energy += fillRight;
			}
		}
		else
		{
			break;
		}
	}
}

Energy ModulePilgrim::get_hand_energy_storage(Hand::Type _hand) const
{
	if (!GameSettings::get().difficulty.commonHandEnergyStorage && lostHand[_hand])
	{
		return Energy::zero();
	}
	_hand = GameSettings::any_use_hand(_hand);
	return _hand == Hand::Left ? leftHandEnergyStorage.energy : rightHandEnergyStorage.energy;
}

Energy ModulePilgrim::get_hand_energy_max_storage(Hand::Type _hand) const
{
	if (!GameSettings::get().difficulty.commonHandEnergyStorage && lostHand[_hand])
	{
		return Energy::zero();
	}
	_hand = GameSettings::any_use_hand(_hand);
	return _hand == Hand::Left ? leftHandEnergyStorage.maxEnergy : rightHandEnergyStorage.maxEnergy;
}

void ModulePilgrim::set_hand_energy_storage(Hand::Type _hand, Energy const& _energy)
{
	if (!GameSettings::get().difficulty.commonHandEnergyStorage && lostHand[_hand])
	{
		return;
	}
	_hand = GameSettings::any_use_hand(_hand);
	Energy energy = min(_energy, get_hand_energy_max_storage(_hand));
	if (_hand == Hand::Left)
	{
		leftHandEnergyStorage.energy = energy;
	}
	else
	{
		rightHandEnergyStorage.energy = energy;
	}
}

void ModulePilgrim::add_hand_energy_storage(Hand::Type _hand, Energy const& _energy)
{
	if (!GameSettings::get().difficulty.commonHandEnergyStorage && lostHand[_hand])
	{
		return;
	}
	if (auto* gd = GameDirector::get())
	{
		if (gd->are_ammo_storages_unavailable())
		{
			return;
		}
	}
	_hand = GameSettings::any_use_hand(_hand);
	set_hand_energy_storage(_hand, get_hand_energy_storage(_hand) + _energy);
}

bool ModulePilgrim::set_pending_pockets_vertical_adjustment(float _pocketsVerticalAdjustment)
{
	bool result = pendingPocketsVerticalAdjustment != _pocketsVerticalAdjustment;
	pendingPocketsVerticalAdjustment = _pocketsVerticalAdjustment;
	return result;
}

void ModulePilgrim::recreate_mesh_accordingly_to_pending_setup(bool _force)
{
	ASSERT_ASYNC;
	if (_force || pendingPocketsVerticalAdjustment != pocketsVerticalAdjustment)
	{
		pocketsVerticalAdjustment = pendingPocketsVerticalAdjustment;
		if (auto * ap = get_owner()->get_appearance())
		{
			auto& ownerVariables = get_owner()->access_variables();
			ownerVariables.access<float>(NAME(pocketsVerticalAdjustment)) = pocketsVerticalAdjustment;

			ap->recreate_mesh_with_mesh_generator();

			look_up_sockets();
		}
	}
}

void ModulePilgrim::update_pilgrim_setup_for(Framework::IModulesOwner const* _mainEquipment, bool _updatePendingToo)
{
	for_count(int, hIdx, Hand::MAX)
	{
		if (lostHand[hIdx])
		{
			_mainEquipment = nullptr;
		}
		Hand::Type hand = (Hand::Type)hIdx;
		auto* currentMainEquipment = get_main_equipment(hand);
		{
			if (currentMainEquipment == _mainEquipment || !_mainEquipment)
			{
				for_count(int, updatesSetupIdx, _updatePendingToo ? 2 : 1)
				{
					auto& pHand = (updatesSetupIdx == 1? pendingPilgrimSetup : pilgrimSetup).access_hand(hand);
					Concurrency::ScopedSpinLock lock(pHand.weaponSetupLock);
					ModuleEquipments::Gun* gun = currentMainEquipment ? currentMainEquipment->get_gameplay_as<ModuleEquipments::Gun>() : nullptr;
					if (gun)
					{
						pHand.weaponSetup.copy_from(gun->get_weapon_setup());
					}
					else
					{
						pHand.weaponSetup.clear();
					}
				}
			}
		}
	}
}

bool ModulePilgrim::check_pilgrim_setup() const
{
	for_count(int, hIdx, Hand::MAX)
	{
		if (lostHand[hIdx])
		{
			continue;
		}
		Hand::Type hand = (Hand::Type)hIdx;
		auto& pHand = pilgrimSetup.get_hand(hand);
		Concurrency::ScopedSpinLock lock(pHand.weaponSetupLock);
		if (auto* me = get_main_equipment(hand))
		{
			if (pHand.weaponSetup.get_item_type() && me->get_as_item() && pHand.weaponSetup.get_item_type() == me->get_as_item()->get_object_type())
			{
				// assume it's ok if the item type matches
			}
			else if (auto* g = me->get_gameplay_as<ModuleEquipments::Gun>())
			{
				if (pHand.weaponSetup != g->get_weapon_setup())
				{
#ifdef INSPECT_EQUIPMENT
					output(TXT("weapon differs"));
#endif
					return false;
				}
			}
			else
			{
				if (!pHand.weaponSetup.is_empty())
				{
#ifdef INSPECT_EQUIPMENT
					output(TXT("not a weapon?"));
#endif
					return false;
				}
			}
		}
		else
		{
			if (pHand.weaponSetup.is_valid() || pHand.weaponSetup.get_item_type())
			{
#ifdef INSPECT_EQUIPMENT
				output(TXT("no weapon but setup is valid or item type provided"));
#endif
				return false;
			}
		}
	}
	return true;
}

bool ModulePilgrim::compare_permanent_exms() const
{
	if (pilgrimInventory.permanentEXMs.get_size() != leftEXMs.permanentEXMs.get_size())
	{
		return false;
	}
	else
	{
		Concurrency::ScopedMRSWLockRead lock(pilgrimInventory.exmsLock);
		for_every(exm, leftEXMs.permanentEXMs)
		{
			auto& invEXM = pilgrimInventory.permanentEXMs[for_everys_index(exm)];
			if (exm->exm != invEXM)
			{
				return false;
			}
		}
	}
	return true;
}

void ModulePilgrim::create_things_accordingly_to_pending_pilgrim_setup(bool _force)
{
	scoped_call_stack_info(TXT("create_things_accordingly_to_pending_pilgrim_setup"));

	PilgrimSetupComparison comparison;
	// copy weapon setup from guns (if they exist)
	an_assert(check_pilgrim_setup()); // we should have updated setup (always update it!), make sure it is,
	int passiveEXMSlotCount = pilgrimBlackboard.calculate_passive_exm_slot_count(pilgrimSetup, &pilgrimInventory);
	pilgrimSetup.update_passive_exm_slot_count(passiveEXMSlotCount);
	pendingPilgrimSetup.update_passive_exm_slot_count(passiveEXMSlotCount);
	bool same = PilgrimSetup::compare(pilgrimSetup, pendingPilgrimSetup, OUT_ comparison);
	pilgrimSetup.copy_from(pendingPilgrimSetup); // own lock
	pilgrimSetup.update_passive_exm_slot_count(passiveEXMSlotCount);
	if (!compare_permanent_exms())
	{
		same = false;
		comparison.hands[Hand::Left].exms = false;
	}
	if (!same || _force)
	{
		if (!comparison.hands[Hand::Left].exms)
		{
			if (auto* a = fast_cast<Framework::Actor>(get_owner()))
			{
				leftEXMs.create(a, Hand::Left, !lostHand[Hand::Left], pilgrimSetup.get_hand(Hand::Left), &pilgrimInventory); // pilgrimInventory as we want to create permanent but we want to do it just once
				rightEXMs.create(a, Hand::Right, !lostHand[Hand::Right], pilgrimSetup.get_hand(Hand::Right), nullptr);
			}
		}
		create_equipment(!lostHand[Hand::Left] && (_force || !comparison.hands[Hand::Left].equipment),
						 !lostHand[Hand::Right] && (_force || !comparison.hands[Hand::Right].equipment));
	}
	update_blackboard();
	upToDateWithPilgrimSetup = true;
}

bool ModulePilgrim::is_active_exm_equipped(Hand::Type _hand, Framework::IModulesOwner const* _imoEXM) const
{
	if (lostHand[_hand])
	{
		return false;
	}

	auto& exms = _hand == Hand::Left ? leftEXMs : rightEXMs;
	auto& exm = exms.activeEXM;

	return _imoEXM && exm.imo.get() == _imoEXM;
}

ModuleEXM* ModulePilgrim::get_equipped_active_exm(Hand::Type _hand)
{
	if (lostHand[_hand])
	{
		return nullptr;
	}
	auto& exms = _hand == Hand::Left ? leftEXMs : rightEXMs;
	auto& exm = exms.activeEXM;
	
	if (auto* imo = exm.imo.get())
	{
		return imo->get_gameplay_as<ModuleEXM>();
	}

	return nullptr;
}

bool ModulePilgrim::does_equipped_exm_appear_active(Hand::Type _hand, OPTIONAL_ OUT_ Optional<float> * _cooldownLeftPt)
{
	if (lostHand[_hand])
	{
		return false;
	}
	if (auto* exm = get_equipped_active_exm(_hand))
	{
		if (_cooldownLeftPt && ! exm->is_exm_active())
		{
			*_cooldownLeftPt = exm->get_cooldown_left_pt();
		}
		if (exm->does_exm_appear_active())
		{
			return true;
		}
	}
	return false;
}

ModuleEXM* ModulePilgrim::get_equipped_exm(Name const& _exm, Optional<Hand::Type> const& _hand)
{
	for_count(int, iHand, Hand::MAX)
	{
		if (lostHand[iHand])
		{
			continue;
		}
		Hand::Type hand = (Hand::Type)iHand;
		if (!_hand.is_set() || _hand.get() == hand)
		{
			auto& exms = hand == Hand::Left ? leftEXMs : rightEXMs;

			ArrayStatic<EXMs::EXM const *, 10> cachedEXMs; SET_EXTRA_DEBUG_INFO(cachedEXMs, TXT("ModulePilgrim::get_equipped_exm.cachedEXMs"));
			cachedEXMs.push_back(&exms.activeEXM);
			for_every(exm, exms.passiveEXMs)
			{
				cachedEXMs.push_back(exm);
			}
			for_every_ptr(exm, cachedEXMs)
			{
				if (auto* imo = exm->imo.get())
				{
					if (auto* mexm = imo->get_gameplay_as<ModuleEXM>())
					{
						if (auto* exmType = mexm->get_exm_type())
						{
							if (exmType->get_id() == _exm)
							{
								return mexm;
							}
						}
					}
				}
			}
		}
	}

	return nullptr;
}

bool ModulePilgrim::has_exm_equipped(Name const& _exm, Optional<Hand::Type> const& _hand) const
{
	Concurrency::ScopedMRSWLockRead lock(pilgrimSetup.access_lock());

	for_count(int, iHand, Hand::MAX)
	{
		if (lostHand[iHand])
		{
			continue;
		}
		Hand::Type hand = (Hand::Type)iHand;
		if (!_hand.is_set() || _hand.get() == hand)
		{
			auto& psHand = pilgrimSetup.get_hand(hand);
			if (psHand.activeEXM.exm == _exm)
			{
				return true;
			}
			for_every(exm, psHand.passiveEXMs)
			{
				if (exm->exm == _exm)
				{
					return true;
				}
			}
		}
	}

	if (!_hand.is_set())
	{
		Concurrency::ScopedMRSWLockRead lock(pilgrimInventory.exmsLock);
		for_every(exm, pilgrimInventory.permanentEXMs)
		{
			if (*exm == _exm)
			{
				return true;
			}
		}
	}

	return false;
}

void ModulePilgrim::sync_recreate_exms()
{
	ASSERT_SYNC;

	scoped_call_stack_info(TXT("sync_recreate_exms"));

	PilgrimSetupComparison comparison;
	int passiveEXMSlotCount = pilgrimBlackboard.calculate_passive_exm_slot_count(pilgrimSetup, &pilgrimInventory);
	pilgrimSetup.update_passive_exm_slot_count(passiveEXMSlotCount);
	pendingPilgrimSetup.update_passive_exm_slot_count(passiveEXMSlotCount);
	bool same = PilgrimSetup::compare(pilgrimSetup, pendingPilgrimSetup, OUT_ comparison);
	if (!compare_permanent_exms())
	{
		same = false;
		comparison.hands[Hand::Left].exms = false;
	}
	if (!same)
	{
		if (! comparison.hands[Hand::Left].exms ||
			! comparison.hands[Hand::Right].exms)
		{
			pilgrimSetup.copy_exms_from(pendingPilgrimSetup);
			pilgrimSetup.update_passive_exm_slot_count(passiveEXMSlotCount);

			if (auto* a = fast_cast<Framework::Actor>(get_owner()))
			{
				Framework::ScopedAutoActivationGroup saag(a->get_in_world());

				leftEXMs.create(a, Hand::Left, !lostHand[Hand::Left], pilgrimSetup.get_hand(Hand::Left), &pilgrimInventory);
				rightEXMs.create(a, Hand::Right, !lostHand[Hand::Right], pilgrimSetup.get_hand(Hand::Right), nullptr);
			}

			update_blackboard();
		}

		// check again
		PilgrimSetupComparison comparison;
		upToDateWithPilgrimSetup = PilgrimSetup::compare(pilgrimSetup, pendingPilgrimSetup, OUT_ comparison) && compare_permanent_exms();
	}

	if (auto* g = Game::get_as<Game>())
	{
		if (g->access_player().get_actor() == get_owner())
		{
			if (auto* persistence = Persistence::access_current_if_exists())
			{
#ifdef OUTPUT_PERSISTENCE_LAST_SETUP
				output(TXT("[persistence] store last setup on recreate EXMs"));
#endif
				persistence->store_last_setup(this);
			}
		}
	}
}

void ModulePilgrim::update_blackboard()
{
	pilgrimBlackboard.update(pilgrimSetup, &pilgrimInventory);
	leftHandEnergyStorage.maxEnergy = leftHandEnergyStorage.baseMaxEnergy + pilgrimBlackboard.get_hand_energy_storage(get_owner(), Hand::Left);
	rightHandEnergyStorage.maxEnergy = rightHandEnergyStorage.baseMaxEnergy + pilgrimBlackboard.get_hand_energy_storage(get_owner(), Hand::Right);
	if (!GameSettings::get().difficulty.commonHandEnergyStorage)
	{
		if (lostHand[Hand::Left])
		{
			leftHandEnergyStorage.maxEnergy = Energy::zero();
		}
		if (lostHand[Hand::Right])
		{
			rightHandEnergyStorage.maxEnergy = Energy::zero();
		}
	}
	redistribute_energy(); // will call update_super_health_storage as well

	{
		int pocketLevel = PilgrimBlackboard::get_pocket_level(get_owner());
		for_every(pocket, pockets)
		{
			pocket->isActive = pocket->pocketLevel <= pocketLevel;
		}
	}

	overlayInfo.recreate_in_hand_equipment_stats();

	if (auto* w = get_owner()->get_in_world())
	{
		if (auto* aim = w->create_ai_message(NAME(pilgrimBlackboardUpdated)))
		{
			aim->access_param(NAME(who)).access_as<SafePtr<Framework::IModulesOwner>>() = get_owner();
			aim->to_world(w);
		}
	}
}

void ModulePilgrim::set_pending_pilgrim_setup(PilgrimSetup const & _newSetup)
{
	pendingPilgrimSetup.copy_from(_newSetup); // own lock
	upToDateWithPilgrimSetup = false;
}

void ModulePilgrim::create_equipment(bool _left, bool _right)
{
#ifdef INSPECT_EQUIPMENT
	output(TXT("create equipment"));
#endif
	ASSERT_ASYNC;
	if (modulePilgrimData)
	{
		if (auto * ownerAsObject = get_owner_as_object())
		{
			// use owner's activation group only if available, otherwise we will be depending on game's activation group
			auto* ag = ownerAsObject->get_activation_group();
			if (ag)
			{
				Game::get()->push_activation_group(ag);
			}
#ifdef FOR_TSHIRTS
			if (false)
#endif
			{
				for_count(int, idx, Hand::MAX)
				{
					Hand::Type handIdx = (Hand::Type)idx;
					if ((handIdx == Hand::Left && !_left) ||
						(handIdx == Hand::Right && !_right))
					{
#ifdef INSPECT_EQUIPMENT
						output(TXT("not for this hand"));
#endif
						continue;
					}
					
					create_equipment_internal(handIdx, pilgrimSetup.get_hand(handIdx).weaponSetup);
				}
			}

			if (ag)
			{
				Game::get()->pop_activation_group(ag);
			}
		}
	}
	advance_hands(0.0001f); // to put things into hands
}

Framework::IModulesOwner* ModulePilgrim::create_equipment_internal(Hand::Type _hand, Framework::ItemType const* _itemType)
{
	if (lostHand[_hand])
	{
		return nullptr;
	}

	WeaponSetup weaponSetup(nullptr);

	weaponSetup.set_item_type(_itemType);

#ifdef INSPECT_EQUIPMENT
	output(TXT("try to create equipment\"%S\""), _itemType->get_name().to_string().to_char());
#endif

	return create_equipment_internal(_hand, weaponSetup);
}

Framework::IModulesOwner* ModulePilgrim::create_equipment_internal(Hand::Type _hand, WeaponSetup const & _weaponSetup)
{
	if (lostHand[_hand])
	{
		return nullptr;
	}

	ASSERT_ASYNC;
	Framework::IModulesOwner* createdEquipment = nullptr;
	if (auto* ownerAsObject = get_owner_as_object())
	{
#ifdef AN_ASSERT
		//SafePtr<Framework::IModulesOwner>& usedEquipment = _hand == Hand::Left ? leftHandEquipment : rightHandEquipment;
#endif
		SafePtr<Framework::IModulesOwner>& mainEquipment = _hand == Hand::Left ? leftMainEquipment : rightMainEquipment;
		if (mainEquipment.is_set() && !_weaponSetup.is_valid() && !_weaponSetup.get_item_type())
		{
#ifdef INSPECT_EQUIPMENT
			output(TXT("invalid weapon setup, destroy equipment"));
#endif
			mainEquipment->cease_to_exist(true);
		}
		else if (!mainEquipment.is_set())
		{
#ifdef INSPECT_EQUIPMENT
			output(TXT("try to create equipment"));
#endif
			Framework::Object* equipment = nullptr;
			Framework::ItemType* itemType = _weaponSetup.get_item_type();
			bool useWeaponParts = false;
			if (!itemType && _weaponSetup.is_valid())
			{
				useWeaponParts = true;
				itemType = Library::get_current()->get_global_references().get<Framework::ItemType>(NAME(grWeaponItemTypeToUseWithWeaponParts));
			}
			if (itemType)
			{
				itemType->load_on_demand_if_required();

				for_every(part, _weaponSetup.get_parts())
				{
					if (auto* p = part->part.get())
					{
						p->make_sure_schematic_mesh_exists();
					}
				}
				Game::get_as<Game>()->perform_sync_world_job(TXT("create weapon"), [&equipment, itemType, ownerAsObject]()
				{
					equipment = itemType->create(String(TXT("pilgrim weapon")));
					equipment->init(ownerAsObject->get_in_sub_world());
				});
				Random::Generator rg = ownerAsObject->get_individual_random_generator();
				if (_hand == Hand::Left)
				{
					rg.advance_seed(795, 9736);
				}
				else
				{
					rg.advance_seed(9076, 863);
				}
				equipment->access_individual_random_generator() = rg.spawn();

				// rest points are also set via gun module
				//equipment->access_variables().access<bool>(_hand == Hand::Left ? NAME(withLeftRestPoint) : NAME(withRightRestPoint)) = true;
				equipment->access_variables().access<bool>(NAME(withLeftRestPoint)) = true;
				equipment->access_variables().access<bool>(NAME(withRightRestPoint)) = true;
				if (auto* pi = PilgrimageInstance::get())
				{
					if (auto* p = pi->get_pilgrimage())
					{
						equipment->access_variables().set_from(p->get_equipment_parameters());
					}
				}
				if (auto* pd = get_pilgrim_data())
				{
					equipment->access_variables().set_from(pd->get_equipment_parameters());
				}

				equipment->initialise_modules([useWeaponParts, &_weaponSetup](Framework::Module* _module)
				{
					if (auto* g = fast_cast<ModuleEquipments::Gun>(_module))
					{
						// the stats differ then a bit as we have different storages
						//g->be_pilgrim_weapon();

						if (useWeaponParts)
						{
							g->setup_with_weapon_setup(_weaponSetup);
						}
					}
				});

				Game::get_as<Game>()->perform_sync_world_job(TXT("make equipment active"), [this, equipment, _hand]()
				{
					set_main_equipment(_hand, equipment);
					// do not place it anywhere here
					use(_hand, equipment);
					use(_hand, nullptr);
					handStates[_hand].grab.target = 0.0f;
				});
#ifdef INSPECT_EQUIPMENT
				output(TXT("weapon created!"));
#endif
				createdEquipment = equipment;
			}
		}
		else if (_weaponSetup.is_valid() && mainEquipment->get_gameplay_as<ModuleEquipments::Gun>())
		{
			auto* g = mainEquipment->get_gameplay_as<ModuleEquipments::Gun>();
			// will redestribute excess energy
			g->setup_with_weapon_setup(_weaponSetup);
		}
		else
		{
			todo_important(TXT("not a gun? what?"));
		}
	}

	return createdEquipment;
}

void ModulePilgrim::keep_object_attached(Framework::IModulesOwner* _object, bool _vrAttachment)
{
	MODULE_OWNER_LOCK_FOR_IMO(_object, TXT("ModulePilgrim::keep_object_attached"));

	Framework::ModulePresence* presence = get_owner()->get_presence();

	Framework::ModulePresence* objectPresence = _object->get_presence();
	if (!objectPresence->get_in_room())
	{
		objectPresence->issue_reattach();
	}
	else
	{
		float relativeDistance;
		float stringPulledDistance;
		objectPresence->calculate_distances_to_attached_to(OUT_ relativeDistance, OUT_ stringPulledDistance, presence->get_centre_of_presence_os());

		if (stringPulledDistance > 1.5f * relativeDistance)
		{
			objectPresence->issue_reattach();
		}
	}

	if (_vrAttachment)
	{
		objectPresence->set_vr_attachment(presence->is_vr_movement());
	}
}

void ModulePilgrim::advance_post_move(float _deltaTime)
{
	MEASURE_PERFORMANCE(pilgrim_post_move);

	MODULE_OWNER_LOCK(TXT("ModulePilgrim::advance_post_move"));

	MEASURE_PERFORMANCE(pilgrim_post_move_locked);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	beingAdvanced.increase();
#endif

	base::advance_post_move(_deltaTime);

	advancedAtLeastOnce = true;

	if (auto* pk = get_pilgrim_keeper())
	{
		MEASURE_PERFORMANCE(pilgrim_keeper);

		if (!pk->get_presence()->get_in_room())
		{
			if (auto* pkm = fast_cast<ModuleMovementPilgrimKeeper>(pk->get_movement()))
			{
				pkm->request_teleport();
			}
		}
	}

	{
		timeSinceLastPickupInHand += _deltaTime;
		if (timeSinceLastPickupInHand > 1.0f) // no need to check this every frame
		{
			bool holdingPickup = false; // pickup, not gun
			for_count(int, handIdx, Hand::MAX)
			{
				if (auto* he = get_hand_equipment((Hand::Type)handIdx))
				{
					if (he->get_custom<CustomModules::Pickup>() &&
						!he->get_gameplay_as<ModuleEquipments::Gun>())
					{
						holdingPickup = true;
					}
				}
			}
			if (holdingPickup)
			{
				timeSinceLastPickupInHand = 0.0f;
			}
		}
	}

	{
		auto* inRoom = get_owner()->get_presence()->get_in_room();
		if (inRoom != infiniteRegen.checkedForRoom)
		{
			infiniteRegen.checkedForRoom = inRoom;
			infiniteRegen.healthRate = Energy::zero();
			infiniteRegen.ammoRate = Energy::zero();
			if (inRoom)
			{
				if (auto* v = inRoom->get_variable<Energy>(NAME(infiniteHealthRegen)))
				{
					infiniteRegen.healthRate = *v;
				}
				if (auto* v = inRoom->get_variable<Energy>(NAME(infiniteAmmoRegen)))
				{
					infiniteRegen.ammoRate = *v;
				}
			}
		}

		if (!infiniteRegen.healthRate.is_zero())
		{
			Energy giveEnergy = infiniteRegen.healthRate.timed(_deltaTime, infiniteRegen.healthMB);
			if (auto* h = get_owner()->get_custom<CustomModules::Health>())
			{
				EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
				etr.instigator = get_owner();
				etr.energyRequested = giveEnergy;
				h->handle_health_energy_transfer_request(etr);
			}
		}
		else
		{
			infiniteRegen.healthMB = 0.0f;
		}

		if (!infiniteRegen.ammoRate.is_zero())
		{
			Energy giveEnergy = infiniteRegen.ammoRate.timed(_deltaTime, infiniteRegen.ammoMB);
			for_count(int, handIdx, Hand::MAX)
			{
				if (auto* h = get_hand((Hand::Type)handIdx))
				{
					if (auto* mph = h->get_gameplay_as<ModulePilgrimHand>())
					{
						EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
						etr.instigator = get_owner();
						etr.energyRequested = giveEnergy;
						mph->handle_ammo_energy_transfer_request(etr);
					}
				}
			}
		}
		else
		{
			infiniteRegen.ammoMB = 0.0f;
		}
	}

#ifdef TEST_TAKING_FROM_POCKETS
	for_every(p, pockets)
	{
		if (auto* input = ::System::Input::get())
		{
			int takerHand = NONE;
			if (input->has_key_been_pressed(::System::Key::H))
			{
				takerHand = Hand::Left;
			}
			if (input->has_key_been_pressed(::System::Key::J))
			{
				takerHand = Hand::Right;
			}
			if (takerHand != NONE)
			{
				if (get_hand_equipment(Hand::Type(takerHand)) == nullptr)
				{
					if (auto* inPocket = p->get_in_pocket())
					{
						use(Hand::Type(takerHand), inPocket);
					}
				}
			}
		}
	}
#endif

	{
		MEASURE_PERFORMANCE(weapons);

		bool newWeaponsBlocked = GameDirector::get()->are_weapons_blocked();
		if (weaponsBlocked != newWeaponsBlocked)
		{
			for_count(int, hIdx, Hand::MAX)
			{
				if (auto* e = get_main_equipment((Hand::Type)hIdx))
				{
					if (auto* pickup = e->get_custom<CustomModules::Pickup>())
					{
						pickup->set_can_be_picked_up(!newWeaponsBlocked);
					}
					if (auto* u = get_hand_equipment((Hand::Type)hIdx))
					{
						if (e == u)
						{
							ex__force_release((Hand::Type)hIdx);
						}
					}
				}
			}
		}
		weaponsBlocked = newWeaponsBlocked;
	}

	// make sure attached objects are attached, not left behind
	{
		MEASURE_PERFORMANCE(objects_attached);

		for_count(int, h, Hand::MAX)
		{
			if (Framework::IModulesOwner* hand = h == 0 ? leftHand.get() : rightHand.get())
			{
				keep_object_attached(hand, true);
			}
			if (Framework::IModulesOwner* fad = h == 0 ? leftHandForearmDisplay.get() : rightHandForearmDisplay.get())
			{
				keep_object_attached(fad, true);
			}
		}

		for_every(pocket, pockets)
		{
			if (Framework::IModulesOwner* objInPocket = pocket->get_in_pocket())
			{
				keep_object_attached(objInPocket);
			}
		}
	}

	if (GameSettings::get().difficulty.regenerateEnergy)
	{
		leftHandEnergyStorage.energy = leftHandEnergyStorage.maxEnergy;
		rightHandEnergyStorage.energy = rightHandEnergyStorage.maxEnergy;
	}

	advance_hands(_deltaTime);

	// show exms
	{
		bool showsActiveEXMs = false;
		Optional<float> leftCooldownLeftPt;
		Optional<float> rightCooldownLeftPt;
		showsActiveEXMs |= does_equipped_exm_appear_active(Hand::Left, &leftCooldownLeftPt);
		showsActiveEXMs |= does_equipped_exm_appear_active(Hand::Right, &rightCooldownLeftPt);
		showsActiveEXMs |= leftCooldownLeftPt.is_set();
		showsActiveEXMs |= rightCooldownLeftPt.is_set();

		if (showsActiveEXMs ^ overlayShowsActiveEXMs)
		{
			overlayShowsActiveEXMs = showsActiveEXMs;
			if (showsActiveEXMs)
			{
				overlayInfo.show_exms(NAME(activeEXMs), true);
			}
			else
			{
				overlayInfo.hide_exms(NAME(activeEXMs));
			}
		}
	}

	// show gun stats
	if (PlayerPreferences::should_show_in_hand_equipment_stats_at_glance())
	{
		Framework::ModulePresence* presence = get_owner()->get_presence();

		Transform cameraWS = presence->get_placement().to_world(presence->get_eyes_relative_look());

#ifdef INSPECT_SHOWING_GUN_STATS_CALL_STACK
		scoped_call_stack_info(TXT("check if to show gun stats"));
		scoped_call_stack_info(presence->get_in_room() ? presence->get_in_room()->get_name().to_char() : TXT("--"));
#endif

		for_count(int, h, Hand::MAX)
		{
#ifdef INSPECT_SHOWING_GUN_STATS_CALL_STACK
			scoped_call_stack_info(Hand::to_char((Hand::Type)h));
#endif
			if (auto* he = get_hand_equipment((Hand::Type)h))
			{
				if (auto* g = he->get_gameplay_as<ModuleEquipments::Gun>())
				{
					Concurrency::ScopedSpinLock lock(he->get_presence()->access_attached_lock(), TXT("ModulePilgrim::advance_post_move show gun stats"));
#ifdef INSPECT_SHOWING_GUN_STATS_CALL_STACK
					scoped_call_stack_info(he->get_as_object()->get_name().to_char());
#endif
					if (he->get_presence())
					{
						Framework::Room* inRoom = he->get_presence()->get_in_room();
						Transform placementWS = he->get_presence()->get_placement();
						Framework::IModulesOwner* top = nullptr;
#ifdef INSPECT_SHOWING_GUN_STATS_CALL_STACK
						scoped_call_stack_info(inRoom ? inRoom->get_name().to_char() : TXT("--"));
#endif
						he->get_presence()->get_attached_placement_for_top(OUT_ inRoom, OUT_ placementWS, get_owner(), &top);
						if (top == get_owner() &&
							inRoom == presence->get_in_room())
						{
							Vector3 muzzleOS = hardcoded Vector3::yAxis;
							Vector3 rightOS = hardcoded Vector3::xAxis;
							Vector3 fromCameraWS = (placementWS.get_translation() - cameraWS.get_translation()).normal();
							float dotMuzzle = Vector3::dot(placementWS.vector_to_world(muzzleOS).normal(), fromCameraWS);
							float dotRight = Vector3::dot(placementWS.vector_to_world(rightOS).normal(), fromCameraWS);
							if (abs(dotMuzzle) < 0.4f &&
								abs(dotRight) > 0.7f)
							{
								overlayInfo.show_in_hand_equipment_stats((Hand::Type)h, NAME(lookingAtGun), 0.1f);
							}
						}
					}
					if (g->is_damaged())
					{
						overlayInfo.show_in_hand_equipment_stats((Hand::Type)h, NAME(gunDamagedShortInfo), 0.1f);
					}
				}
			}
		}
	}

	overlayInfo.set_may_request_looking_for_pickups(should_pilgrim_overlay_look_for_pickups());

	if (Framework::GameUtils::is_local_player(get_owner()))
	{
		bool isMovingOnBase = false;
		if (auto* imo = get_owner()->get_presence()->get_based_on())
		{
			isMovingOnBase = ! imo->get_presence()->get_velocity_linear().is_zero() || ! imo->get_presence()->get_velocity_rotation().is_zero();
			if (auto* p = fast_cast<ModulePresenceBackgroundMoverBase>(imo->get_presence()))
			{
				isMovingOnBase |= p->is_moving_for_background_mover();
			}
		}
		if (isMovingOnBase)
		{
			timeSinceBaseMoving = 0.0f;
		}
		else
		{
			timeSinceBaseMoving += _deltaTime;
		}
		isMovingOnBase |= timeSinceBaseMoving < 0.1f; // to fix when stops for one frame?
		if (isMovingOnBase && !physSensMoving.is_set())
		{
			PhysicalSensations::OngoingSensation s(PhysicalSensations::OngoingSensation::SlowSpeed);
			physSensMoving = PhysicalSensations::start_sensation(s);
		}
		else if (!isMovingOnBase && physSensMoving.is_set())
		{
			PhysicalSensations::stop_sensation(physSensMoving.get());
			physSensMoving.clear();
		}
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	beingAdvanced.decrease();
#endif
}

#ifdef AN_DEBUG_RENDERER
Vector3 debug_get_pickup_location(Framework::IModulesOwner * imo)
{
	Vector3 loc = imo->get_presence()->get_centre_of_presence_WS();
	if (auto* pickup = imo->get_custom<CustomModules::Pickup>())
	{
		if (pickup->get_pickup_reference_socket().is_valid())
		{
			int	socketIdx = imo->get_appearance()->find_socket_index(pickup->get_pickup_reference_socket());
			if (socketIdx != NONE)
			{
				loc = imo->get_presence()->get_placement().location_to_world(imo->get_appearance()->calculate_socket_os(socketIdx).get_translation());
			}
		}
	}
	return loc;
}
#endif

void ModulePilgrim::advance_hands(float _deltaTime)
{
	MEASURE_PERFORMANCE(advance_hands);

#ifdef AN_DEVELOPMENT_OR_PROFILER
	beingAdvanced.increase();
#endif

	ARRAY_STACK(Vector4, highlightPockets, pockets.get_size());
	highlightPockets.set_size(pockets.get_size());

	bool anyPocketVisible = false;

	for_every(pocket, pockets)
	{
		if (pocket->get_in_pocket())
		{
			bool stillInPocket = false;
			if (auto * imo = pocket->get_in_pocket())
			{
				if (auto* p = imo->get_presence())
				{
					if (p->get_attached_to_socket().is_valid())
					{
						if (p->get_attached_to_socket() == pocket->socket)
						{
							stillInPocket = true;
						}
					}
				}
			}
			if (! stillInPocket)
			{
				if (auto* p = pocket->get_in_pocket()->get_custom<CustomModules::Pickup>())
				{
					p->clear_in_pocket_of(get_owner());
				}
				pocket->set_in_pocket(nullptr);
			}
		}
		bool somethingToPutInside = false;
		bool itemInHand = false;
		for_count(int, i, 2)
		{
			SafePtr<Framework::IModulesOwner>& equipment = i == Hand::Left ? leftHandEquipment : rightHandEquipment;
			SafePtr<Framework::IModulesOwner>& mainEquipment = i == Hand::Left ? leftMainEquipment : rightMainEquipment;
			if (handStates[i].pocketToPutInto == pocket)
			{
				somethingToPutInside = true;
			}
			if (pocket->isActive &&
				equipment.get() && (GameSettings::get().settings.mainEquipmentCanBePutIntoPocket || equipment != mainEquipment))
			{
				bool canBePutIntoPocket = NON_PICKUPS_CAN_BE_PUT_INTO_POCKET;
				if (auto * pickup = equipment->get_custom<CustomModules::Pickup>())
				{
					canBePutIntoPocket = pickup->can_be_put_into_pocket();
				}
				if (canBePutIntoPocket)
				{
					itemInHand = true;
				}
			}
		}
		if (pocket->isActive && ! weaponsBlocked)
		{
			if (pocket->get_in_pocket())
			{
				pocket->highlight.target = 0.0f;
			}
			else if (somethingToPutInside)
			{
				pocket->highlight.target = 1.0f;
			}
			else if (itemInHand)
			{
				pocket->highlight.target = 0.3f;
			}
			else
			{
				pocket->highlight.target = 0.0f;
			}
		}
		else
		{
			pocket->highlight.target = 0.0f;
		}
		pocket->highlight.blend_to_target(0.2f, _deltaTime);
		if (highlightPockets.is_index_valid(pocket->materialParamIndex))
		{
			highlightPockets[pocket->materialParamIndex].x = pocket->highlight.state;
		}

		anyPocketVisible |= pocket->highlight.state > 0.00001f;
	}

	if (pocketsMaterialIndex.is_set() && pocketsMaterialParam.is_valid())
	{
		if (auto* a = get_owner()->get_appearance())
		{
			todo_note(TXT("can't switch appearance invisible as it's not only the pocket indicators but also, break into two separate meshes? is it worth it?"));
			if (anyPocketVisible)
			{
				if (auto* mat = a->access_mesh_instance().access_material_instance(pocketsMaterialIndex.get()))
				{
					mat->set_uniform(pocketsMaterialParam, highlightPockets);
				}
			}
		}
	}

	for_count(int, i, 2)
	{
		if (lostHand[i])
		{
			continue;
		}

		Hand::Type hand = (Hand::Type)i;
		AdvanceHandContext ahContext(this
			,	_deltaTime
			,	i
			,	handStates[i]
			,	i == Hand::Left ? leftHandEquipment : rightHandEquipment
			,	i == Hand::Left ? leftMainEquipment : rightMainEquipment
			);

		if (auto* vr = VR::IVR::get())
		{
			if (auto* himo = get_hand(hand))
			{
				if (auto* c = himo->get_collision())
				{
					c->set_play_collision_sound(vr->is_hand_read((Hand::Type)i));
				}
			}
		}

		ahContext.advance_physical_violence();
		ahContext.advance_late_grab();
		ahContext.advance_hand_state();
		ahContext.update_pocket_to_put_into();
		ahContext.update_equipment_state();
		ahContext.update_gun_equipment();
		ahContext.choose_pose();
		ahContext.update_pose();
		ahContext.set_pose_vars();
		ahContext.update_grabable();
		ahContext.post_advance();
	}

	{
		bool shouldShowPrompt = handStates[0].requiresHoldWeaponPrompt || handStates[1].requiresHoldWeaponPrompt;
		if (shouldShowPrompt != equipmentReleasedWithoutHoldingShowPrompt)
		{
			equipmentReleasedWithoutHoldingShowPrompt = shouldShowPrompt;
			if (equipmentReleasedWithoutHoldingShowPrompt)
			{
				PilgrimOverlayInfo::ShowTipParams showTipParams;
				showTipParams.be_forced().follow_camera_pitch();
				{
					Hand::Type dominantHand = Hand::Right;
					if (auto* vr = VR::IVR::get())
					{
						dominantHand = vr->get_dominant_hand();
					}
					if (handStates[dominantHand].requiresHoldWeaponPrompt)
					{
						showTipParams.for_hand(dominantHand);
					}
					else if (handStates[Hand::other_hand(dominantHand)].requiresHoldWeaponPrompt)
					{
						showTipParams.for_hand(Hand::other_hand(dominantHand));
					}
				}
				overlayInfo.show_tip(PilgrimOverlayInfoTip::InputHoldWeapon, showTipParams);
			}
			else
			{
				overlayInfo.hide_tip(PilgrimOverlayInfoTip::InputHoldWeapon);
			}
		}
	}

	{
		// we set the tip when we grab something but if there's no more reason to show it, hide it
		if (!leftHandEquipment.is_set() || !leftMainEquipment.is_set())
		{
			overlayInfo.hide_tip(PilgrimOverlayInfoTip::InputRemoveDamagedWeaponLeft);
		}
		if (!rightHandEquipment.is_set() || !rightMainEquipment.is_set())
		{
			overlayInfo.hide_tip(PilgrimOverlayInfoTip::InputRemoveDamagedWeaponRight);
		}
		if (highlightDamagedLeftMainEquipment && (!leftHandEquipment.is_set() || !leftMainEquipment.is_set()))
		{
			highlightDamagedLeftMainEquipment = false;
		}
		if (highlightDamagedRightMainEquipment && (!rightHandEquipment.is_set() || !rightMainEquipment.is_set()))
		{
			highlightDamagedRightMainEquipment = false;
		}
	}

	// hands and controls
	for_count(int, i, 2)
	{
		if (auto* equipment = get_hand_equipment((Hand::Type)i))
		{
			if (ModuleEquipment* mse = equipment->get_gameplay_as<ModuleEquipment>())
			{
				MEASURE_PERFORMANCE(advance_user_controls);
				mse->advance_user_controls();
			}
		}
	}

	{
		Vector2 stick = handStates[0].inputOverlayInfoStick + handStates[1].inputOverlayInfoStick;
		stick.x = clamp(stick.x, -1.0f, 1.0f);
		stick.y = clamp(stick.y, -1.0f, 1.0f);
		overlayInfo.advance(_deltaTime, handStates[0].showOverlayInfo || handStates[1].showOverlayInfo, stick);
	}

#ifdef AN_DEVELOPMENT_OR_PROFILER
	beingAdvanced.decrease();
#endif
}

bool ModulePilgrim::should_pilgrim_overlay_look_for_pickups() const
{
	return timeSinceLastPickupInHand > 180.0f; // remind every now and then
}

void ModulePilgrim::show_weapons_blocked_overlay_message()
{
	if (auto* ss = SubtitleSystem::get())
	{
		SubtitleId sid = ss->get(NAME(lsWeaponsLocked));
		if (sid == NONE)
		{
			sid = ss->add(NAME(lsWeaponsLocked), NP, true, RegisteredColours::get_colour(NAME(rcPilgrimageOverlayWarning)), Colour::black.with_alpha(0.25f));
		}
		ss->remove(sid, 2.0f);
	}
}

void ModulePilgrim::lose_hand(Hand::Type _hand)
{
	lostHand[_hand] = true;
	bool destroyWeapons = false;
	if (GameSettings::get().difficulty.aiMean)
	{
		destroyWeapons = true;
	}
	if (auto* gd = GameDefinition::get_chosen())
	{
		if (gd->get_type() == GameDefinition::Type::ComplexRules)
		{
			destroyWeapons = true;
		}
	}
	if (destroyWeapons)
	{
		if (auto* eq = get_hand_equipment(_hand))
		{
			eq->cease_to_exist(false);
		}
		if (auto* eq = get_main_equipment(_hand))
		{
			eq->cease_to_exist(false);
		}
	}
	else
	{
		Array<Framework::IModulesOwner*> damageEq;
		if (auto* eq = get_hand_equipment(_hand))
		{
			damageEq.push_back_unique(eq);
			drop(eq);
		}
		if (auto* eq = get_main_equipment(_hand))
		{
			damageEq.push_back_unique(eq);
			drop(eq);
		}
		for_every_ptr(eq, damageEq)
		{
			if (auto* gun = eq->get_gameplay_as<ModuleEquipments::Gun>())
			{
				for_every(part, gun->access_weapon_setup().get_parts())
				{
					if (auto* p = part->part.get())
					{
						if (auto* wpt = p->get_weapon_part_type())
						{
							if (wpt->get_type() == WeaponPartType::GunCore)
							{
								p->ex__set_damaged(EnergyCoef(0.5f));
							}
						}
					}
				}
				gun->ex__update_stats();
				update_pilgrim_setup_for(eq, true);
			}
		}
	}
	update_pilgrim_setup_for(nullptr, true);
	if (auto* h = get_hand(_hand))
	{
		h->cease_to_exist(false);
	}
	if (auto* imo = (_hand == Hand::Left ? leftHandForearmDisplay : rightHandForearmDisplay).get())
	{
		imo->cease_to_exist(false);
	}
	if (auto* imo = (_hand == Hand::Left ? leftRestPoint : rightRestPoint).get())
	{
		imo->cease_to_exist(false);
	}

	{
		auto& psHand = pilgrimSetup.access_hand(_hand);
		Concurrency::ScopedSpinLock lock(psHand.exmsLock);
		psHand.remove_exms();
	}

	if (_hand == Hand::Left)
	{
		get_owner()->access_variables().access<float>(NAME(lostLeftHand)) = 1.0f;
	}
	if (_hand == Hand::Right)
	{
		get_owner()->access_variables().access<float>(NAME(lostRightHand)) = 1.0f;
	}

	update_pilgrim_setup_for(nullptr, true);

	update_blackboard();
}

//--

void ModulePilgrim::AdvanceHandContext::advance_physical_violence()
{
	ARRAY_STACK(Framework::IModulesOwner*, vObjs, 5);
	if (handState.physicalViolenceDirSocket.is_valid())
	{
		if (auto* hand = get_hand())
		{
			if (handState.physicalViolence.timeSinceLastHit.is_set())
			{
				handState.physicalViolence.timeSinceLastHit = handState.physicalViolence.timeSinceLastHit.get() + deltaTime;
				if (handState.physicalViolence.timeSinceLastHit.get() > 2.0f) // if it stops, it is cleared
				{
					handState.physicalViolence.timeSinceLastHit.clear();
				}
			}
			vObjs.push_back(hand);
			// no hitting with a gun now
			if (equipment.is_set())
			{
				vObjs.push_back(equipment.get());
			}			
			Transform physicalViolenceDirPlacementOS = hand->get_appearance()->calculate_socket_os(handState.physicalViolenceDirSocket.get_index());
			Transform physicalViolenceDirPlacementWS = hand->get_presence()->get_placement().to_world(physicalViolenceDirPlacementOS);
			//Vector3 physicalViolenceDirOS = physicalViolenceDirPlacementOS.get_axis(Axis::Forward);
			Vector3 physicalViolenceDirWS = physicalViolenceDirPlacementWS.get_axis(Axis::Forward);
			Vector3 physicalViolenceWS = physicalViolenceDirPlacementWS.get_translation();
			Vector3 handLocWS = hand->get_presence()->get_placement().get_translation();
			Vector3 velocityWS = hand->get_presence()->get_velocity_linear();
			Vector3 velocityDirWS = velocityWS.normal();
			float speed = velocityWS.length();
			float minSpeed = 1000.0f;
			float minSpeedStrong = 1000.0f;
			modulePilgrim->get_physical_violence_hand_speeds(idxAsHand, OUT_ minSpeed, OUT_ minSpeedStrong);
			debug_filter(pilgrim_physicalViolence);
			debug_context(hand->get_presence()->get_in_room());
			debug_draw_text(true, Colour::green, handLocWS, NP, true, NP, NP, TXT("%.3f"), speed);
			debug_no_context();
			if (Vector3::dot(velocityDirWS, physicalViolenceDirWS) > 0.7f &&
				speed >= minSpeed &&
				!handState.physicalViolence.timeSinceLastHit.is_set())
			{
				bool strongSwing = speed >= minSpeedStrong;

				Framework::IModulesOwner* hitImo = nullptr;
				bool hitImoWithHealth = false;
				Vector3 hitAtWS = Vector3::zero;
				Optional<Vector3> hitAtOS; // other object
				Optional<Vector3> hitDirOS; // other object
				Framework::Room* hitInRoom = nullptr;
				Optional<Vector3> hitRelativeDir; // who was hit
				auto* pilgrimOwner = modulePilgrim->get_owner();
				for_every_ptr(vObj, vObjs)
				{
					if (auto* c = vObj->get_collision())
					{
						for_every(co, c->get_collided_with())
						{
							if (co->collidedWithObject.is_set())
							{
								if (auto* coImo = fast_cast<Framework::IModulesOwner>(co->collidedWithObject.get()))
								{
									if (modulePilgrim->is_in_pocket(coImo)) // ignore items in pockets
									{
										continue;
									}
									if (coImo->get_top_instigator() == pilgrimOwner && // hands and own parts
										!modulePilgrim->is_held_in_hand(coImo)) // but not items held in hand
									{
										continue;
									}
									bool coImoWithHealth = coImo->get_custom<CustomModules::Health>() != nullptr;
									if (!coImoWithHealth && hitImoWithHealth)
									{
										// if we already have one with health and this ones doesn't have health, choose one with health
										continue;
									}
									::Framework::RelativeToPresencePlacement r2pp;
									r2pp.be_temporary_snapshot();
									Optional<Vector3> collisionNormalWS;
									if (co->presenceLink && co->collisionNormal.is_set())
									{
										if (r2pp.find_path(hand, co->presenceLink->get_in_room(), co->presenceLink->get_placement_in_room(), true))
										{
											collisionNormalWS = r2pp.vector_from_target_to_owner(co->collisionNormal.get());
										}
									}
									else if (co->collidedWithShape)
									{
										if (r2pp.find_path(hand, coImo->get_presence()->get_in_room(), coImo->get_presence()->get_placement(), true))
										{
											// object's space
											Vector3 handLocOWS = r2pp.location_from_owner_to_target(handLocWS);
											Vector3 handLocOS = coImo->get_presence()->get_placement().location_to_local(handLocOWS);
											Vector3 objectCentreOS = coImo->get_presence()->get_centre_of_presence_os().get_translation();
											Vector3 closestHitLocOS = objectCentreOS;
											float collisionRadius = 0.0f;
											if (auto* c = coImo->get_collision())
											{
												Vector3 onPrimitiveOS = handLocOS;
												if (c->calc_closest_point_on_primitive_os(REF_ onPrimitiveOS))
												{
													closestHitLocOS = onPrimitiveOS;
													collisionRadius = c->get_collision_primitive_radius();
												}
											}
											Vector3 closestHitLocOWS = coImo->get_presence()->get_placement().location_to_world(closestHitLocOS);
											Vector3 closestHitLocWS = r2pp.location_from_target_to_owner(closestHitLocOWS);
											collisionNormalWS = (handLocWS - closestHitLocWS).normal();
											hitRelativeDir = coImo->get_presence()->get_placement().vector_to_local(r2pp.vector_from_owner_to_target(velocityDirWS));
											physicalViolenceWS = closestHitLocWS + collisionNormalWS.get() * collisionRadius;
											physicalViolenceWS = r2pp.location_from_target_to_owner(physicalViolenceWS);
										}
									}
									if (collisionNormalWS.is_set())
									{
										// we're hitting straight on
										if (Vector3::dot(collisionNormalWS.get(), physicalViolenceDirWS) < -0.3f)
										{
											hitImo = coImo;
											hitImoWithHealth = coImoWithHealth;
											if (co->collisionLocation.is_set() &&
												co->collidedInRoom.is_set())
											{
												hitAtWS = co->collisionLocation.get();
												hitInRoom = co->collidedInRoom.get();
											}
											else
											{
												todo_note(TXT("move through doors <-- ?"));
												hitAtWS = physicalViolenceWS;
												hitInRoom = hand->get_presence()->get_in_room();
											}
											if (r2pp.get_in_final_room() == coImo->get_presence()->get_in_room())
											{
												hitAtOS = coImo->get_presence()->get_placement().location_to_local(r2pp.location_from_owner_to_target(hitAtWS));
												hitDirOS = coImo->get_presence()->get_placement().vector_to_local(r2pp.vector_from_owner_to_target(physicalViolenceDirWS));
											}
										}
									}
								}
							}
							if (hitImo)
							{
								break;
							}
						}
					}
					if (hitImo)
					{
						break;
					}
				}

				if (hitImo)
				{
					handState.physicalViolence.timeSinceLastHit = 0.0f;
					if (hitImoWithHealth)
					{
						bool canBeVampirisedWithMelee = false;
						bool allowAmmoForHit = true;
						bool allowHealthForHit = true;
						if (auto* health = hitImo->get_custom<CustomModules::Health>())
						{
							canBeVampirisedWithMelee = health->can_be_vampirised_with_melee();
							Damage dealDamage;
							DamageInfo damageInfo;
							dealDamage.damage = modulePilgrim->get_physical_violence_damage(idxAsHand, strongSwing);
							dealDamage.damageType = DamageType::Physical;
							dealDamage.meleeDamage = true;
							damageInfo.damager = hand;
							damageInfo.source = hand;
							damageInfo.instigator = modulePilgrim->get_owner();
							damageInfo.hitRelativeDir = hitRelativeDir;
							if (hitAtOS.is_set())
							{
								if (auto* c = hitImo->get_collision())
								{
									Name closestBone = c->find_closest_bone(hitAtOS.get(), AgainstCollision::Precise);
									damageInfo.hitBone = closestBone;
								}
							}
							bool wasAlive = health->is_alive();
							ContinuousDamage dealContinuousDamage;
							dealContinuousDamage.setup_using_weapon_core_modifier_companion_for(dealDamage);
							health->adjust_damage_on_hit_with_extra_effects(REF_ dealDamage);
							health->deal_damage(dealDamage, dealDamage, damageInfo);
							if (dealContinuousDamage.is_valid())
							{
								health->add_continuous_damage(dealContinuousDamage, damageInfo);
							}
							if (wasAlive && !health->is_alive() &&
								canBeVampirisedWithMelee)
							{
								// killing blow, give energy
								{
									Energy giveEnergy = PilgrimBlackboard::get_health_for_melee_kill(pilgrimOwner);
									if (giveEnergy.is_positive())
									{
										if (auto* h = pilgrimOwner->get_custom<CustomModules::Health>())
										{
											EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
											etr.instigator = pilgrimOwner;
											etr.energyRequested = giveEnergy;
											h->handle_health_energy_transfer_request(etr);
											allowHealthForHit = false; // we did it here
											if (auto* hand = modulePilgrim->get_hand(idxAsHand))
											{
												if (auto* to = modulePilgrim->get_owner()->get_temporary_objects())
												{
													to->spawn_attached_to(NAME(healthFromPhysicalViolence), hand, Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(hand->get_presence()->get_centre_of_presence_os()));
												}
												if (auto* sound = hand->get_sound())
												{
													sound->play_sound(NAME(healthFromPhysicalViolence));
												}
											}
										}
									}
								}
								{
									Energy giveEnergy = PilgrimBlackboard::get_ammo_for_melee_kill(pilgrimOwner);
									if (giveEnergy.is_positive())
									{
										auto& energyStorage = idxAsHand == Hand::Left ? modulePilgrim->leftHandEnergyStorage : modulePilgrim->rightHandEnergyStorage;
										energyStorage.energy = min(energyStorage.energy + giveEnergy, energyStorage.maxEnergy);
										allowAmmoForHit = false; // we did it here
										if (auto* hand = modulePilgrim->get_hand(idxAsHand))
										{
											if (auto* to = modulePilgrim->get_owner()->get_temporary_objects())
											{
												to->spawn_attached_to(NAME(ammoFromPhysicalViolence), hand, Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(hand->get_presence()->get_centre_of_presence_os()));
											}
											if (auto* sound = hand->get_sound())
											{
												sound->play_sound(NAME(ammoFromPhysicalViolence));
											}
										}
									}
								}
							}
						}
						if (auto* vr = VR::IVR::get())
						{
							vr->trigger_pulse(Hand::get_vr_hand_index(idxAsHand),
								strongSwing? modulePilgrim->get_pilgrim_data()->get_strong_physical_violence_pulse()
										   : modulePilgrim->get_pilgrim_data()->get_physical_violence_pulse());
						}
						if (auto* to = modulePilgrim->get_owner()->get_temporary_objects())
						{
							if (hitAtOS.is_set() && hitDirOS.is_set())
							{
								to->spawn_attached_to(strongSwing ? NAME(strongPhysicalViolence) : NAME(physicalViolence), hitImo, Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(matrix_from_up(hitAtOS.get(), -hitDirOS.get()).to_transform()));
								debug_context(hitInRoom);
								debug_draw_time_based(4.0f, debug_draw_sphere(true, true, Colour::yellow, 0.2f, Sphere(hitAtWS, 0.02f)));
								debug_draw_time_based(4.0f, debug_draw_text(true, Colour::yellow, handLocWS, NP, true, NP, NP, TXT("%.3f"), speed));
								debug_no_context();
							}
							else
							{
								to->spawn_in_room(strongSwing ? NAME(strongPhysicalViolence) : NAME(physicalViolence), hitInRoom, matrix_from_up(hitAtWS, -physicalViolenceDirWS).to_transform());
								debug_context(hitInRoom);
								debug_draw_time_based(4.0f, debug_draw_sphere(true, true, Colour::red, 0.2f, Sphere(hitAtWS, 0.02f)));
								debug_draw_time_based(4.0f, debug_draw_text(true, Colour::red, handLocWS, NP, true, NP, NP, TXT("%.3f"), speed));
								debug_no_context();
							}
						}
						if (canBeVampirisedWithMelee)
						{
							if (allowHealthForHit)
							{
								if (auto* mexm = modulePilgrim->get_equipped_exm(EXMID::Passive::vampire_fists()))
								{
									Energy addHealth = GameplayBalance::health_for_melee_hit();
									if (addHealth.is_positive())
									{
										if (auto* h = pilgrimOwner->get_custom<CustomModules::Health>())
										{
											EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
											etr.instigator = pilgrimOwner;
											etr.energyRequested = addHealth;
											h->handle_health_energy_transfer_request(etr);
											{
												if (auto* to = modulePilgrim->get_owner()->get_temporary_objects())
												{
													to->spawn_attached_to(NAME(littleAmmoFromPhysicalViolence), hand, Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(hand->get_presence()->get_centre_of_presence_os()));
												}
												if (auto* sound = hand->get_sound())
												{
													sound->play_sound(NAME(littleAmmoFromPhysicalViolence));
												}
											}
										}
									}
								}
							}
							if (allowAmmoForHit)
							{
								if (auto* mexm = modulePilgrim->get_equipped_exm(EXMID::Passive::beggar_fists()))
								{
									Energy addAmmo = GameplayBalance::ammo_for_melee_hit();
									if (addAmmo.is_positive())
									{
										if (auto* hand = modulePilgrim->get_hand(idxAsHand))
										{
											if (auto* h = hand->get_gameplay_as<ModulePilgrimHand>())
											{
												EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
												etr.instigator = pilgrimOwner;
												etr.energyRequested = addAmmo;
												h->handle_ammo_energy_transfer_request(etr);
												{
													if (auto* hand = modulePilgrim->get_hand(idxAsHand))
													{
														if (auto* to = modulePilgrim->get_owner()->get_temporary_objects())
														{
															to->spawn_attached_to(NAME(littleHealthFromPhysicalViolence), hand, Framework::ModuleTemporaryObjects::SpawnParams().at_relative_placement(hand->get_presence()->get_centre_of_presence_os()));
														}
														if (auto* sound = hand->get_sound())
														{
															sound->play_sound(NAME(littleHealthFromPhysicalViolence));
														}
													}
												}
											}
										}
									}
								}
							}
						}
					}
					else
					{
						todo_note(TXT("particles when no health"));
					}
				}
			}
			else if (speed < minSpeed * 0.25f)
			{
				handState.physicalViolence.timeSinceLastHit.clear();
			}
			debug_no_filter();
		}
	}
}

void ModulePilgrim::AdvanceHandContext::advance_late_grab()
{
	if (handState.objectToLateGrabPropositionStillValidFor.is_set())
	{
		handState.objectToLateGrabPropositionStillValidFor = max(0.0f, handState.objectToLateGrabPropositionStillValidFor.get() - deltaTime);
		if (handState.objectToLateGrabPropositionStillValidFor.get() == 0.0f)
		{
			handState.objectToLateGrabProposition.clear();
			handState.objectToLateGrabPropositionStillValidFor.clear();
		}
	}

#ifdef DEBUG_DRAW_GRAB
#ifdef AN_DEBUG_RENDERER
	debug_filter(pilgrimGrab);
	if (handState.objectToLateGrabProposition.is_set())
	{
		debug_context(handState.objectToLateGrabProposition->get_presence()->get_in_room());
		debug_draw_text(true, Colour::blue, debug_get_pickup_location(handState.objectToLateGrabProposition.get()), Vector2::half, true, 0.2f, NP, TXT("L"));
		debug_no_context();
	}
	if (handState.objectToGrabProposition.is_set())
	{
		debug_context(handState.objectToGrabProposition->get_presence()->get_in_room());
		debug_draw_text(true, Colour::green, debug_get_pickup_location(handState.objectToGrabProposition.get()), Vector2::half, true, 0.2f, NP, TXT("G"));
		debug_no_context();
	}
	debug_no_filter();
#endif
#endif
}

void ModulePilgrim::AdvanceHandContext::deploy_main_equipment(HandState& handState, SafePtr<Framework::IModulesOwner>& mainEquipment)
{
	ModuleEquipments::Gun* gunEq = mainEquipment.get() ? mainEquipment->get_gameplay_as<ModuleEquipments::Gun>() : nullptr;
	if (gunEq)
	{
		if (!modulePilgrim->weaponsBlocked)
		{
#ifdef INVESTIGATE_MISSING_INPUT
			REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# ModulePilgrim::AdvanceHandContext::deploy_main_equipment"));
#endif

			// use main equipment
			handState.blockUsingEquipment = true; // allow using when we release use button
			handState.blockReleasingEquipment = true; // allow releasing only if we actually hold it
			handState.blockReleasingEquipmentTimeLeft = BLOCK_RELEASING_TIME;
			handState.currentState = HandState::GrabObject;
			handState.objectToGrab = mainEquipment;
			handState.objectToGrabPath.reset();
			handState.autoMainEquipmentActive = true;
		}
		else
		{
			handState.autoMainEquipmentActive = false;
			modulePilgrim->show_weapons_blocked_overlay_message();
		}
	}
}

void ModulePilgrim::AdvanceHandContext::advance_hand_state()
{
	handState.prevState = handState.currentState;

	// we should either hold it or not hold it
	if (equipment.is_set())
	{
		if (handState.currentState == HandState::EmptyHand)
		{
			handState.currentState = HandState::HoldingObject;
		}
	}
	else
	{
		if (handState.currentState == HandState::HoldingObject)
		{
			handState.currentState = HandState::EmptyHand;
		}
	}

	// handle state basing on input
	{
		if (handState.shouldBeParalysed)
		{
			if (handState.currentState != HandState::EmptyHand)
			{
				if (!handState.exBlockReleasingEquipment)
				{
					handState.currentState = HandState::ReleaseObject;
				}
			}
		}
		else if (handState.exForceRelease)
		{
			if (handState.currentState != HandState::EmptyHand)
			{
				handState.currentState = HandState::ReleaseObject;
				handState.autoMainEquipmentActive = false;
			}
			else
			{
				handState.exForceRelease = false;
			}
		}
		else
		{
			// maybe there is interim equipment available and we want to fold main equipment?
			Optional<bool> useMainEquipmentDueToAutoInterimEquipment;
			if (handState.inputAutoMainEquipment &&
				!handState.shouldBeParalysed)
			{
				if (handState.autoMainEquipmentActive)
				{
					// only if auto interim equipment
					if ((handState.objectToGrabProposition.is_set() && handState.objectToGrabProposition.get() != mainEquipment.get()) ||
						(handState.objectToLateGrabProposition.is_set() && handState.objectToLateGrabProposition.get() != mainEquipment.get()) ||
						modulePilgrim->weaponsBlocked)
					{
						if (equipment == mainEquipment)
						{
							useMainEquipmentDueToAutoInterimEquipment = false;
						}
					}
					else
					{
						if (!equipment.is_set())
						{
							useMainEquipmentDueToAutoInterimEquipment = true;
						}
					}
				}
			}
			Optional<bool> easyRelease;
			if (handState.inputEasyReleaseEquipment > CONTROLS_THRESHOLD)
			{
				if (equipment.is_set() &&
					equipment != mainEquipment)
				{
					// this is for auto hold, to know whether we should hold "grab" to keep it in hand or drop it
					bool allowEasyReleaseForAutoInterimEquipment = true;
					if (auto* eq = equipment.get())
					{
						if (auto* me = eq->get_gameplay_as<ModuleEquipment>())
						{
							allowEasyReleaseForAutoInterimEquipment = me->should_allow_easy_release_for_auto_interim_equipment();
						}
					}
					if (allowEasyReleaseForAutoInterimEquipment)
					{
						easyRelease = true;
					}
				}
			}

			// release or grab object
			if (handState.currentState != HandState::EmptyHand &&
				handState.currentState != HandState::ReleaseObject)
			{
				// release?
				ModuleEquipments::Gun * gunEq = equipment.get() ? equipment->get_gameplay_as<ModuleEquipments::Gun>() : nullptr;
				if (gunEq &&
					handState.inputUseEquipment > 0.9f)
				{
					// reset timer so the moment we release grab, we drop weapon
					handState.blockReleasingEquipmentTimeLeft = 0.8f;
				}
				bool releaseNow = false;
				bool autoReleaseNow = false;
				//
				if (handState.forceRelease)
				{
					// forced release (grabable, moved too far away)
					releaseNow = true;
				}
				if (!handState.blockReleasingEquipment &&
					((handState.inputSharedReleaseGrabDeploy < CONTROLS_THRESHOLD && handState.inputRelease > CONTROLS_THRESHOLD) ||
					 (handState.inputSharedReleaseGrabDeploy > CONTROLS_THRESHOLD && handState.inputRelease > CONTROLS_THRESHOLD && handState.prevInputRelease <= CONTROLS_THRESHOLD)))
				{
					// release equipment because of input
					handState.autoMainEquipmentActive = false;
					releaseNow = true;
					++modulePilgrim->equipmentReleasedWithoutHoldingCount;
				}
				if (useMainEquipmentDueToAutoInterimEquipment.is_set() && !useMainEquipmentDueToAutoInterimEquipment.get())
				{
					// release main equipment because auto interim equipment management is on and there is auto interim equipment available to grab
					autoReleaseNow = true;
				}
				if (easyRelease.is_set() && easyRelease.get())
				{
					// don't try to grab anymore
					autoReleaseNow = true;
				}
				else if (handState.currentState == HandState::GrabObject &&
					(handState.inputGrab < CONTROLS_THRESHOLD || (!handState.objectToGrabProposition.is_set() && !handState.objectToLateGrabProposition.is_set())) &&
					handState.objectToGrab != mainEquipment)
				{
					// don't try to grab anymore
					autoReleaseNow = true;
				}
				//
				if (releaseNow || autoReleaseNow)
				{
					if (!handState.exBlockReleasingEquipment)
					{
						handState.currentState = HandState::ReleaseObject;
						handState.grab.target = handState.inputSeparateFingers ? 1.0f : (releaseNow ? 1.0f - handState.inputRelease : 0.0f);
					}
				}
			}
			else
			{
				// grab
				if (handState.currentState != HandState::HoldingObject &&
					handState.currentState != HandState::GrabObject)
				{
					bool shouldGrabSomething = false;
					if (handState.inputGrab > CONTROLS_THRESHOLD &&
						handState.prevInputGrab <= CONTROLS_THRESHOLD &&
						(handState.objectToGrabProposition.is_set() || handState.objectToLateGrabProposition.is_set()))
					{
						shouldGrabSomething = true;
					}
					if (shouldGrabSomething &&
						(handState.grab.state < 0.2f || // to require releasing button
						 handState.inputAutoHold)) // for auto-hold we manage grab state separately, so we allow user for greater control with pure input
					{
						handState.currentState = HandState::GrabObject;
						handState.objectToGrab = handState.objectToGrabProposition.is_set() ? handState.objectToGrabProposition : handState.objectToLateGrabProposition;
						handState.objectToGrabPath = handState.objectToGrabProposition.is_set() ? handState.objectToGrabPropositionPath : handState.objectToLateGrabPropositionPath;

#ifdef DEBUG_DRAW_GRAB
#ifdef AN_DEBUG_RENDERER
						debug_filter(pilgrimGrab);
						if (handState.objectToGrab.is_set())
						{
							debug_context(handState.objectToGrab->get_presence()->get_in_room());
							debug_draw_time_based(2.0f, debug_draw_text(true, Colour::red, debug_get_pickup_location(handState.objectToGrab.get()), Vector2::half, true, 0.2f, NP, TXT("!")));
							debug_no_context();
						}
						else
						{
							debug_context(get_hand()->get_presence()->get_in_room());
							debug_draw_time_based(2.0f, debug_draw_text(true, Colour::purple, get_hand()->get_presence()->get_centre_of_presence_WS(), Vector2::half, true, 0.2f, NP, TXT("?")));
							debug_no_context();
						}
						debug_no_filter();
#endif
#endif
					}
					else if (((handState.inputSharedReleaseGrabDeploy < CONTROLS_THRESHOLD && handState.inputUseEquipment > 0.9f && !shouldGrabSomething) ||
							  (handState.inputSharedReleaseGrabDeploy > CONTROLS_THRESHOLD && handState.inputUseEquipment > 0.9f && handState.prevInputUseEquipment <= 0.9f)) ||
							 (useMainEquipmentDueToAutoInterimEquipment.is_set() && useMainEquipmentDueToAutoInterimEquipment.get()))
					{
						// wants to use equipment or main equipment wants to redeploy
						deploy_main_equipment(handState, mainEquipment);
#ifdef DEBUG_DRAW_GRAB
#ifdef AN_DEBUG_RENDERER
						if (handState.inputGrab > CONTROLS_THRESHOLD &&
							handState.prevInputGrab <= CONTROLS_THRESHOLD)
						{
							if (handState.objectToGrab.is_set())
							{
								debug_filter(pilgrimGrab);
								debug_context(handState.objectToGrab->get_presence()->get_in_room());
								debug_draw_time_based(2.0f, debug_draw_text(true, Colour::purple, debug_get_pickup_location(handState.objectToGrab.get()), Vector2::half, true, 0.2f, NP, TXT(".u.")));
								debug_no_context();
								debug_no_filter();
							}
						}
#endif
#endif
					}
#ifdef DEBUG_DRAW_GRAB
#ifdef AN_DEBUG_RENDERER
					else
					{
						if (handState.inputGrab > CONTROLS_THRESHOLD &&
							handState.prevInputGrab <= CONTROLS_THRESHOLD)
						{
							if (handState.objectToGrab.is_set())
							{
								debug_filter(pilgrimGrab);
								debug_context(handState.objectToGrab->get_presence()->get_in_room());
								debug_draw_time_based(2.0f, debug_draw_text(true, Colour::purple, debug_get_pickup_location(handState.objectToGrab.get()), Vector2::half, true, 0.2f, NP, TXT(".g.")));
								debug_no_context();
								debug_no_filter();
							}
						}
					}
#endif
#endif
				}
				// deploy, it has a lower priority
				if (handState.currentState != HandState::HoldingObject &&
					handState.currentState != HandState::GrabObject)
				{
					if (handState.grab.state < 0.2f) // to require releasing button
					{
						if (handState.inputDeployMainEquipment > CONTROLS_THRESHOLD &&
							handState.prevInputDeployMainEquipment <= CONTROLS_THRESHOLD &&
							(! (handState.objectToGrabProposition.is_set() || handState.objectToLateGrabProposition.is_set()) || handState.inputAllowHandTrackingPose))
						{
							deploy_main_equipment(handState, mainEquipment);
						}
					}
				}
			}

			bool pullEnergyNow = false;
			if ((handState.currentState == HandState::EmptyHand ||
				(handState.currentState == HandState::HoldingObject && equipment == mainEquipment)) &&
				handState.inputPullEnergy > CONTROLS_THRESHOLD &&
				handState.prevInputPullEnergy <= CONTROLS_THRESHOLD &&
				(!handState.objectToGrabProposition.is_set() && !handState.objectToLateGrabProposition.is_set()))
			{
				pullEnergyNow = true;
			}
			else  if (handState.inputPullEnergyAlt > CONTROLS_THRESHOLD &&
				handState.prevInputPullEnergyAlt <= CONTROLS_THRESHOLD)
			{
				pullEnergyNow = true;
			}
			if (pullEnergyNow)
			{
				if (get_hand())
				{
					if (Framework::AI::Message* message = hand->get_in_world()->create_ai_message(NAME(pullEnergy)))
					{
						message->to_ai_object(hand);
					}
				}
			}
		}
	}

	// check if is over forearm & view menu
	{
		handState.showOverlayInfo = handState.inputShowOverlayInfo > CONTROLS_THRESHOLD;
		handState.overForearm = 0.0f;

		if (get_hand() &&
			VR::IVR::get() &&
			VR::IVR::get()->is_controls_view_valid())
		{
			float generalScaling = VR::IVR::get()->get_scaling().general;
			if (generalScaling < 1.0f)
			{
				generalScaling = 1.0f + (generalScaling - 1.0f) * 0.3f;
			}
			float smallerGeneralScaling = 1.0f + (generalScaling - 1.0f) * 0.5f;

			if (hand->get_presence())
			{
				if (auto* appearance = modulePilgrim->get_owner()->get_appearance())
				{
					if (auto* path = hand->get_presence()->get_path_to_attached_to())
					{
						if (path->is_active())
						{
							auto const & otherForarm = modulePilgrim->forearms[1 - handIdx];
							Transform actualHandOS = modulePilgrim->get_owner()->get_presence()->get_placement().to_local(path->from_owner_to_target(path->from_owner_to_target(hand->get_presence()->get_placement())));
							// over forearm
							{
								Transform handOS = actualHandOS;
								if (otherForarm.handOverSocket.is_valid())
								{
									if (auto* ap = hand->get_appearance())
									{
										handOS = handOS.to_world(ap->calculate_socket_os(otherForarm.handOverSocket.get_index()));
									}
								}

								if (otherForarm.forearmStartSocket.is_valid() &&
									otherForarm.forearmEndSocket.is_valid() &&
									otherForarm.forearmEndSocket.get_name() != otherForarm.forearmStartSocket.get_name())
								{
									Transform forearmStartOS = appearance->calculate_socket_os(otherForarm.forearmStartSocket.get_index());
									Transform forearmEndOS = appearance->calculate_socket_os(otherForarm.forearmEndSocket.get_index());

									// we assume that there is the same up dir for both ends
									Transform forearmOS = look_at_matrix(forearmStartOS.get_translation(), forearmEndOS.get_translation(), forearmStartOS.get_axis(Axis::Z)).to_transform();
									Transform handFS = forearmOS.to_local(handOS);
									// check if pointing at the hand
									if (handFS.get_axis(Axis::Forward).z < -0.6f)
									{
										//float const fullZ = 0.12f * smallerGeneralScaling;
										float const maxZ = 0.24f * smallerGeneralScaling;
										//float const fullX = 0.05f * smallerGeneralScaling;
										float const maxX = 0.1f * smallerGeneralScaling;
										if (abs(handFS.get_translation().x) < maxX &&
											handFS.get_translation().z < maxZ &&
											handFS.get_translation().y >= 0.0f)
										{
											float distY = (forearmEndOS.get_translation() - forearmStartOS.get_translation()).length();
											if (handFS.get_translation().y <= distY)
											{
												handState.overForearm = 1.0f;
											}
										}
									}
									/*
									debug_context(modulePilgrim->get_owner()->get_presence()->get_in_room());
									debug_push_transform(modulePilgrim->get_owner()->get_presence()->get_placement());
									debug_draw_line(true, Colour::green, forearmStartOS.get_translation(), forearmEndOS.get_translation());
									debug_draw_arrow(true, Colour::green, forearmStartOS.get_translation(), forearmStartOS.get_translation() + forearmStartOS.get_axis(Axis::Z) * 0.2f);
									debug_draw_arrow(true, Colour::green, forearmEndOS.get_translation(), forearmEndOS.get_translation() + forearmEndOS.get_axis(Axis::Z) * 0.2f);
									debug_draw_arrow(true, Colour::yellow, handOS.get_translation(), (forearmStartOS.get_translation() + forearmEndOS.get_translation()) * 0.5f);
									debug_draw_arrow(true, Colour::yellow, handOS.get_translation(), handOS.get_translation() + handOS.get_axis(Axis::Forward) * 0.1f);
									debug_draw_text(true, Colour::yellow, handOS.get_translation(), NP, NP, 0.3f, NP, TXT("f:%.3f x:%.3f,y:%.3f,z:%.3f"), handFS.get_axis(Axis::Forward).z, handFS.get_translation().x, handFS.get_translation().y, handFS.get_translation().z);
									debug_pop_transform();
									debug_no_context();
									*/
								}
							}
						}
					}
				}
			}
		}
	}

	handState.forceRelease = false;

	// update blocks
	if (handState.inputUseEquipment < 0.1f)
	{
		handState.blockUsingEquipment = false;
	}
	handState.blockReleasingEquipmentTimeLeft = max(0.0f, handState.blockReleasingEquipmentTimeLeft - deltaTime);
	if (handState.inputGrab > 0.9f || handState.inputAutoHold > 0.5f /* because autoHold affects holdGun.target */ || handState.blockReleasingEquipmentTimeLeft <= 0.0f)
	{
		handState.blockReleasingEquipment = false;
	}
	if (handState.currentState == HandState::HoldingObject)
	{
		// hide and remember we held an item if we actually hold an object
		if (handState.inputGrab > 0.9f || handState.inputAutoHold > 0.5f /* because autoHold affects holdGun.target */)
		{
			handState.requiresHoldWeaponPrompt = false;
			modulePilgrim->equipmentReleasedWithoutHoldingCount = 0; // we hold it
		}
	}
	else if (handState.currentState == HandState::ReleaseObject ||
			 handState.currentState == HandState::EmptyHand)
	{
		// hide if there's nothing in the hand
		handState.requiresHoldWeaponPrompt = false;
	}
}

CustomModules::ItemHolder* ModulePilgrim::AdvanceHandContext::find_item_holder_to_put_into() const
{
	CustomModules::ItemHolder* putInto = nullptr;
	if (equipment.is_set())
	{
		if (!handState.pocketToPutInto)
		{
			float bestDist = 0.0f;
			Vector3 locWS = equipment->get_presence()->get_centre_of_presence_WS();
			FOR_EVERY_OBJECT_IN_ROOM(obj, equipment->get_presence()->get_in_room())
			{
				if (auto* ih = obj->get_custom<CustomModules::ItemHolder>())
				{
					bool canBeHeld = true;
					if (canBeHeld && ih->is_locked())
					{
						canBeHeld = false;
					}
					if (canBeHeld &&
						!ih->get_hold_only_tagged().is_empty())
					{
						canBeHeld = false;
						if (auto* eqObj = equipment->get_as_object())
						{
							canBeHeld = ih->get_hold_only_tagged().check(eqObj->get_tags());
						}
					}
					if (canBeHeld)
					{
						if (auto* ap = obj->get_appearance())
						{
							Transform socketIHOS = ap->calculate_socket_os(ih->get_hold_socket());
							Transform socketIHWS = obj->get_presence()->get_placement().to_world(socketIHOS);
							float dist = (socketIHWS.get_translation() - locWS).length();
							/*
							debug_context(obj->get_presence()->get_in_room());
							debug_draw_time_based(10.0f, debug_draw_arrow(true, Colour::green, locWS, socketIHWS.get_translation()));
							debug_draw_time_based(10.0f, debug_draw_text(true, Colour::green, socketIHWS.get_translation(), NP, NP, NP, NP, TXT("%.3f"), dist));
							debug_no_context();
							*/
							if (dist < hardcoded magic_number 0.2f)
							{
								if (!putInto || bestDist > dist)
								{
									putInto = ih;
									bestDist = dist;
								}
							}
						}
					}
				}
			}
		}
	}
	return putInto;
}

void ModulePilgrim::AdvanceHandContext::update_pocket_to_put_into()
{
	Pocket* newPocketToPutInto = nullptr;
	if (equipment.is_set())
	{
		if (GameSettings::get().settings.mainEquipmentCanBePutIntoPocket || equipment != mainEquipment)
		{
			bool canBePutIntoPocket = NON_PICKUPS_CAN_BE_PUT_INTO_POCKET;
			if (auto * pickup = equipment->get_custom<CustomModules::Pickup>())
			{
				canBePutIntoPocket = pickup->can_be_put_into_pocket();
			}
			if (canBePutIntoPocket)
			{
				if (get_hand())
				{
					if (auto* path = hand->get_presence()->get_path_to_attached_to())
					{
						if (auto* eq = modulePilgrim->get_hand_equipment(idxAsHand))
						{
							if (auto* eqPath = eq->get_presence()->get_path_to_attached_to())
							{
								if (path->is_active() &&
									eqPath->is_active())
								{
									Transform eqOS = modulePilgrim->get_owner()->get_presence()->get_placement().to_local(path->from_owner_to_target(eqPath->from_owner_to_target(eq->get_presence()->get_centre_of_presence_transform_WS())));
									if (auto* ap = modulePilgrim->get_owner()->get_appearance())
									{
										Pocket* putIntoPocket = nullptr;
										float closestPocketDist = 0.0f;
										for_every(pocket, modulePilgrim->pockets)
										{
											if (pocket->isActive &&
												!pocket->get_in_pocket())
											{
												Transform pocketOS = ap->calculate_socket_os(pocket->socket.get_index());
												float dist = (pocketOS.get_translation() - eqOS.get_translation()).length();
												/*
												debug_context(get_owner()->get_presence()->get_in_room());
												debug_push_transform(get_owner()->get_presence()->get_placement());
												debug_draw_arrow(true, Colour::red, eqOS.get_translation(), pocketOS.get_translation());
												debug_draw_text(true, Colour::red, (eqOS.get_translation() + pocketOS.get_translation()) * 0.5f, NP, NP, NP, NP, TXT("%.3f"), dist);
												debug_pop_transform();
												debug_no_context();
												*/
												if (dist <= GameSettings::get().settings.pocketDistance &&
													(dist <= closestPocketDist || !putIntoPocket))
												{
													putIntoPocket = pocket;
													closestPocketDist = dist;
												}
											}
										}
										newPocketToPutInto = putIntoPocket;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	handState.pocketToPutInto = newPocketToPutInto;

	{
		bool clear = true;
		if (!handState.pocketToPutInto)
		{
			if (equipment.is_set())
			{
				if (auto* r = equipment->get_presence()->get_in_room())
				{
					if (r->get_tags().get_tag(NAME(itemHolderRoom)))
					{
						if (auto* ih = find_item_holder_to_put_into())
						{
							clear = false;
							handState.putIntoItemHolder = ih->get_owner();
						}
					}
				}
			}
		}
		if (clear)
		{
			handState.putIntoItemHolder.clear();
		}
	}
}

void ModulePilgrim::AdvanceHandContext::update_equipment_state()
{
	// update equipment using and state depending on state
	{
		if (handState.currentState == HandState::ReleaseObject)
		{
			if (equipment.is_set())
			{
				CustomModules::ItemHolder* putInto = find_item_holder_to_put_into();

				bool normalRelease = true;
				if (putInto)
				{
					normalRelease = ! putInto->hold(equipment.get());
					if (!normalRelease)
					{
						if (equipment == mainEquipment)
						{
							modulePilgrim->set_main_equipment(idxAsHand, nullptr);
							// remember that we no longer have main equipment!
							modulePilgrim->update_pilgrim_setup_for(nullptr, true);
						}
						modulePilgrim->use(idxAsHand, nullptr);
						handState.currentState = HandState::EmptyHand;
					}
				}

				if (normalRelease)
				{
					if (equipment == mainEquipment && (! GameSettings::get().settings.mainEquipmentCanBePutIntoPocket || !handState.pocketToPutInto))
					{
						restPointUnfoldRequired = true;
						if (/*handState.port.state == 0.0f &&*/ handState.restPoint.state == 1.0f)
						{
							modulePilgrim->use(idxAsHand, nullptr);
							handState.currentState = HandState::EmptyHand;
						}
					}
					else
					{
						// just release
						Framework::IModulesOwner* equipmentHeld = equipment.get();
						modulePilgrim->use(idxAsHand, nullptr);
						handState.currentState = HandState::EmptyHand;
						if (handState.pocketToPutInto && equipmentHeld)
						{
							modulePilgrim->put_into_pocket(handState.pocketToPutInto, equipmentHeld);
						}
					}
				}
			}
			else
			{
				// just mark it is released
				handState.currentState = HandState::EmptyHand;
			}
		}
		else if (handState.currentState == HandState::GrabObject)
		{
			if (auto* objectToGrab = handState.objectToGrab.get())
			{
				if (objectToGrab == mainEquipment.get())
				{
					restPointUnfoldRequired = true;
					if (handState.port.state == 0.0f && handState.restPoint.state == 1.0f)
					{
						modulePilgrim->use(idxAsHand, objectToGrab);
						handState.currentState = HandState::HoldingObject;
						if (modulePilgrim->equipmentReleasedWithoutHoldingCount >= EQUIPMENT_RELEASED_WITHOUT_HOLDING__COUNT_TO_PROMPT)
						{
							handState.requiresHoldWeaponPrompt = true;
						}
					}
				}
				else if (handState.prevState != HandState::GrabObject)
				{
					bool canBeUsed = true;
					if (auto* pickup = objectToGrab->get_custom<CustomModules::Pickup>())
					{
						canBeUsed = pickup->can_be_picked_up();
					}
					if (auto* grabable = objectToGrab->get_custom<CustomModules::Grabable>())
					{
						canBeUsed = grabable->can_be_grabbed();
					}
					if (canBeUsed)
					{
#ifdef DEBUG_DRAW_GRAB
						debug_filter(pilgrimGrab);
						debug_context(get_hand()->get_presence()->get_in_room());
						debug_draw_time_based(2.0f, debug_draw_text(true, Colour::yellow, get_hand()->get_presence()->get_centre_of_presence_WS(), Vector2::half, true, 0.2f, NP, TXT("#")));
						debug_no_context();
						debug_no_filter();
#endif
						bool updatePilgrimSetup = false;
						// do we grab main equipment?
						{
							if (objectToGrab == modulePilgrim->get_main_equipment(Hand::Left))
							{
								modulePilgrim->set_main_equipment(Hand::Left, nullptr);
								updatePilgrimSetup = true;
								modulePilgrim->overlayInfo.done_tip(PilgrimOverlayInfoTip::InputRemoveDamagedWeaponLeft, Hand::Left);
								modulePilgrim->highlightDamagedLeftMainEquipment = false;
							}
							else if (objectToGrab == modulePilgrim->get_main_equipment(Hand::Right))
							{
								modulePilgrim->set_main_equipment(Hand::Right, nullptr);
								updatePilgrimSetup = true;
								modulePilgrim->overlayInfo.done_tip(PilgrimOverlayInfoTip::InputRemoveDamagedWeaponRight, Hand::Right);
								modulePilgrim->highlightDamagedRightMainEquipment = false;
							}
						}
						// make it main equipment
						if (! mainEquipment.get() &&
							objectToGrab->get_gameplay_as<ModuleEquipments::Gun>())
						{
							modulePilgrim->set_main_equipment(idxAsHand, objectToGrab);
							updatePilgrimSetup = true;
						}
						if (updatePilgrimSetup)
						{
							// we own a main equipment now
							modulePilgrim->update_pilgrim_setup_for(nullptr, true);
						}
						// immediate
						modulePilgrim->use(idxAsHand, objectToGrab);
						handState.currentState = HandState::HoldingObject;
						if (GameDefinition::does_allow_extra_tips())
						{
							if (auto* me = mainEquipment.get())
							{
								if (auto* meGun = me->get_gameplay_as<ModuleEquipments::Gun>())
								{
									if (!meGun->get_damaged().is_zero())
									{
										if (auto* gunToGrab = objectToGrab->get_gameplay_as<ModuleEquipments::Gun>())
										{
											if (gunToGrab->get_damaged().is_zero())
											{
												modulePilgrim->overlayInfo.show_tip(idxAsHand == Hand::Left? PilgrimOverlayInfoTip::InputRemoveDamagedWeaponLeft : PilgrimOverlayInfoTip::InputRemoveDamagedWeaponRight,
													PilgrimOverlayInfo::ShowTipParams().be_forced().follow_camera_pitch());
												if (idxAsHand == Hand::Left) modulePilgrim->highlightDamagedLeftMainEquipment = true;
												if (idxAsHand == Hand::Right) modulePilgrim->highlightDamagedRightMainEquipment = true;
											}
										}
									}
								}
							}
						}
					}
#ifdef DEBUG_DRAW_GRAB
					else
					{
						debug_filter(pilgrimGrab);
						debug_context(get_hand()->get_presence()->get_in_room());
						debug_draw_time_based(2.0f, debug_draw_text(true, Colour::black, get_hand()->get_presence()->get_centre_of_presence_WS(), Vector2::half, true, 0.2f, NP, TXT("#")));
						debug_no_context();
						debug_no_filter();
					}
#endif
				}
			}
			else
			{
				handState.currentState = HandState::EmptyHand;
			}
		}
	}

	if (handState.currentState == HandState::EmptyHand ||
		handState.currentState == HandState::HoldingObject)
	{
		handState.objectToGrab.clear();
		handState.objectToGrabPath.reset();
	}
}

void ModulePilgrim::AdvanceHandContext::update_gun_equipment()
{
	if (auto* eq = equipment.get())
	{
		moduleEquipment = eq->get_gameplay_as<ModuleEquipment>();
		if (eq->get_gameplay_as<ModuleEquipments::Gun>())
		{
			gunEquipment = true;
		}
	}
	if (auto* eq = handState.objectToGrab.get())
	{
		if (eq->get_gameplay_as<ModuleEquipments::Gun>())
		{
			gunEquipmentToGrab = true;
		}
	}

	// block if port is not fully unfolded 
	if (gunEquipment && handState.port.state < 1.0f)
	{
#ifdef INVESTIGATE_MISSING_INPUT
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# block if port is not fully unfolded"));
#endif
		handState.blockUsingEquipmentDueToPort = true;
	}
	else
	{
		handState.blockUsingEquipmentDueToPort = false;
	}

	if (gunEquipment && equipment.get() && !mainEquipment.get())
	{
		modulePilgrim->set_main_equipment(idxAsHand, equipment.get());
		modulePilgrim->update_pilgrim_setup_for(equipment.get(), true);
	}
}

void ModulePilgrim::AdvanceHandContext::choose_pose()
{
#ifdef FOR_TSHIRTS
	handState.straight.target = 1.0f;
#endif

	if (handState.shouldBeParalysed)
	{
		choose_pose_paralysed();
	}
	else if (handState.shouldBeIdle)
	{
		choose_pose_idle();
	}
	else
	{
		choose_pose_normal();
	}

	choose_pose_rest_point();
}

void ModulePilgrim::AdvanceHandContext::choose_pose_paralysed()
{
	an_assert(handState.shouldBeParalysed);

	handState.fakePointing.target = 0.0f;
	handState.paralysedPoseTimeTo.update(deltaTime);
	if (handState.paralysedPoseTimeTo.hand <= 0.0f)
	{
		handState.paralysedPoseTimeTo.hand = Random::get_float(0.2f, 0.6f);

		float * poses[] = { &handState.holdGun.target,
			&handState.holdObject.target,
			&handState.fist.target,
			&handState.grab.target,
			&handState.grabIsDial.target,
			&handState.straight.target,
			nullptr };
		int count = 0;
		float** pose = poses;
		while (*pose)
		{
			**pose = 0.0f;
			++count;
			++pose;
		}
		*poses[Random::get_int(count)] = clamp(Random::get_float(0.0f, 0.3f), 0.0f, 1.0f);
		*poses[Random::get_int(count)] = clamp(Random::get_float(0.0f, 0.6f), 0.0f, 1.0f);
		*poses[Random::get_int(count)] = clamp(Random::get_float(0.5f, 1.3f), 0.0f, 1.0f);
	}
	if (handState.paralysedPoseTimeTo.port <= 0.0f)
	{
		if (Random::get_chance(0.4f))
		{
			handState.paralysedPoseTimeTo.port = Random::get_float(0.3f, 4.0f);
		}
		else
		{
			handState.paralysedPoseTimeTo.port = Random::get_float(0.1f, 0.3f);
		}

		handState.port.target = handState.port.target < 0.5f ? 1.0f : 0.0f;
	}
	if (handState.paralysedPoseTimeTo.display <= 0.0f)
	{
		handState.paralysedPoseTimeTo.port = Random::get_float(0.1f, 0.4f);

		handState.displayNormal.target = Random::get_bool() ? 1.0f : 0.0f;
		handState.displayExtra.target = Random::get_bool() ? 1.0f : 0.0f;

		if (handState.displayNormal.target > 0.5f &&
			handState.displayExtra.target > 0.5f)
		{
			if (Random::get_bool())
			{
				handState.displayNormal.target = 1.0f;
				handState.displayExtra.target = 0.0f;
			}
			else
			{
				handState.displayNormal.target = 0.0f;
				handState.displayExtra.target = 1.0f;
			}
		}
	}
	if (handState.paralysedPoseTimeTo.display <= 0.0f)
	{
		handState.paralysedPoseTimeTo.display = Random::get_float(0.1f, 4.0f);

		handState.displayNormal.target = Random::get_bool() ? 1.0f : 0.0f;
		handState.displayExtra.target = clamp(Random::get_float(-0.5f, 1.5f), 0.0f, 1.0f);

		if (handState.displayNormal.target > 0.5f &&
			handState.displayExtra.target > 0.5f)
		{
			if (Random::get_bool())
			{
				handState.displayNormal.target = 1.0f;
				handState.displayExtra.target = 0.0f;
			}
			else
			{
				handState.displayNormal.target = 0.0f;
				handState.displayExtra.target = 1.0f;
			}
		}
	}

	if (handState.restPoint.target > 0.5f)
	{
		handState.displayExtra.target = 0.0f; // fold as they may cross each other
	}

	handState.inputSeparateFingers = 1.0f;
	if (handState.paralysedPoseTimeTo.fingers <= 0.0f)
	{
		handState.paralysedPoseTimeTo.fingers = Random::get_float(0.1f, 2.0f);

		handState.inputRelaxedThumb = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputRelaxedPointer = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputRelaxedMiddle = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputRelaxedRing = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputRelaxedPinky = Random::get_bool() ? 1.0f : 0.0f;

		handState.inputStraightThumb = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputStraightPointer = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputStraightMiddle = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputStraightRing = Random::get_bool() ? 1.0f : 0.0f;
		handState.inputStraightPinky = Random::get_bool() ? 1.0f : 0.0f;
	}
}

void ModulePilgrim::AdvanceHandContext::choose_pose_idle()
{
	an_assert(handState.shouldBeIdle);

	handState.holdGun.target = 0.0f;
	handState.pressTrigger.target = 0.0f;
	handState.awayTrigger.target = 0.0f;
	handState.holdObject.target = 0.0f;
	handState.fist.target = 0.0f;
	handState.grab.target = 0.0f;
	handState.grabIsDial.target = 0.0f;
	handState.straight.target = 0.0f;
	handState.fakePointing.target = 0.0f;
	handState.port.target = 0.0f;
	handState.displayNormal.target = 0.0f;
	handState.displayExtra.target = 0.0f;

	handState.inputSeparateFingers = 1.0f;
	{
		handState.inputRelaxedThumb = 1.0f;
		handState.inputRelaxedPointer = 1.0f;
		handState.inputRelaxedMiddle = 1.0f;
		handState.inputRelaxedRing = 1.0f;
		handState.inputRelaxedPinky = 1.0f;

		handState.inputStraightThumb = 0.0f;
		handState.inputStraightPointer = 0.0f;
		handState.inputStraightMiddle = 0.0f;
		handState.inputStraightRing = 0.0f;
		handState.inputStraightPinky = 0.0f;
	}
}

void ModulePilgrim::AdvanceHandContext::choose_pose_normal()
{
	an_assert(!handState.shouldBeParalysed &&
			  !handState.shouldBeIdle);

	handState.paralysedPoseTimeTo.reset();

	handState.holdGun.target = 0.0f;
	handState.pressTrigger.target = 0.0f;
	handState.awayTrigger.target = 0.0f;
	handState.holdObject.target = 0.0f;
	handState.fist.target = handState.fist.state;
	handState.grab.target = handState.grab.state;
	handState.grabIsDial.target = handState.grabIsDial.state;
	handState.straight.target = handState.straight.state;
	handState.fakePointing.target = 0.0f;
	handState.port.target = handState.port.state;
	handState.displayNormal.target = handState.displayNormal.state;
	handState.displayExtra.target = handState.displayExtra.state;

	if (equipment.get())
	{
		if (gunEquipment)
		{
			// switch to gun holding but allow it to be releasable
			handState.holdGun.target = modulePilgrim->alwaysAppearToHoldGun ? 1.0f : max(handState.inputAutoHold, handState.inputSeparateFingers ? 1.0f : max(handState.inputFist, handState.inputGrab));
			handState.pressTrigger.target = handState.inputUseEquipment;
			handState.awayTrigger.target = handState.inputUseEquipmentAway;
			handState.holdObject.target = 0.0f;
			handState.fist.target = handState.holdGun.target; // because with guns we will blend to and from fist
			handState.grab.target = 0.0f;
		}
		else if (auto * heldObjectSize = equipment->get_variables().get_existing<float>(modulePilgrim->objectsHeldObjectSizeVarID))
		{
			// pretend we're holding an object really tight
			handState.holdGun.target = 0.0f;
			handState.pressTrigger.target = 0.0f;
			handState.awayTrigger.target = 0.0f;
			handState.holdObject.target = 1.0f;
			handState.holdObjectSize = *heldObjectSize;
			handState.grab.target = 1.0f;
			handState.fist.target = 0.0f;
		}
		else
		{
			// pretend we're holding an object really tight
			handState.holdGun.target = 0.0f;
			handState.pressTrigger.target = 0.0f;
			handState.awayTrigger.target = 0.0f;
			handState.holdObject.target = 0.0f;
			handState.holdObjectSize = 0.0f;
			handState.grab.target = 1.0f;
			handState.fist.target = 0.0f;
		}

		handState.grabIsDial.target = 0.0f;
		if (equipment->get_custom<CustomModules::Grabable>())
		{
			handState.grabIsDial.target = 1.0f;
		}
	}
	else
	{
		if (handState.currentState == HandState::EmptyHand &&
			handState.overForearm > 0.0f && handState.inputAutoPointing)
		{
			handState.fakePointing.target = handState.overForearm;
			handState.fist.target = 1.0f;
			handState.grab.target = 0.0f;
		}
		else if (handState.objectToGrab.is_set())
		{
			if (handState.objectToGrab == mainEquipment)
			{
				handState.fist.target = handState.inputSeparateFingers ? 1.0f : max(handState.inputFist, handState.inputGrab);
				handState.grab.target = 0.0f;
			}
			else
			{
				handState.fist.target = 0.0f;
				handState.grab.target = handState.inputSeparateFingers ? 1.0f : max(handState.inputFist, handState.inputGrab);
			}
		}
		else
		{
			handState.fist.target = handState.inputSeparateFingers ? 1.0f : max(max(handState.inputPullEnergy, handState.inputPullEnergyAlt), max(handState.inputFist, handState.inputGrab));
			handState.grab.target = 0.0f;
		}
	}

	// displays and ports
	if (gunEquipment && handState.currentState == HandState::HoldingObject && handState.restPoint.state < 0.7f)
	{
		// only if mounted and rest point is not close
		handState.port.target = 1.0f;
		handState.displayNormal.target = 1.0f;
		handState.displayExtra.target = 0.0f;
	}
	else if (moduleEquipment && moduleEquipment->is_display_rotated())
	{
		handState.displayNormal.target = 0.0f;
		handState.displayExtra.target = 1.0f;
		handState.port.target = 0.0f;
	}
	else
	{
		handState.displayNormal.target = 0.0f;
		handState.displayExtra.target = 0.0f;
		handState.port.target = 0.0f;
	}
	if (handState.blockDisplay)
	{
		handState.displayNormal.target = 0.0f;
		handState.displayExtra.target = 0.0f;
	}
	if (TutorialSystem::check_active())
	{
		Optional<bool> forceState = TutorialSystem::get()->get_forced_hand_displays_state();
		if (forceState.is_set())
		{
			if (forceState.get())
			{
				handState.displayNormal.target = 1.0f;
				handState.displayExtra.target = 0.0f;
			}
			else
			{
				handState.displayNormal.target = 0.0f;
				handState.displayExtra.target = 0.0f;
			}
		}
	}
}

void ModulePilgrim::AdvanceHandContext::choose_pose_rest_point()
{
	// rest points require separate setup as it mixes between
	{
		if (restPointUnfoldRequired &&
			(handState.currentState == HandState::GrabObject || handState.currentState == HandState::ReleaseObject))
		{
			handState.restPoint.target = 1.0f;
		}
		else
		{
			if (handState.shouldBeParalysed)
			{
				if (handState.paralysedPoseTimeTo.restPoint <= 0.0f)
				{
					handState.paralysedPoseTimeTo.restPoint = Random::get_float(0.1f, 3.0f);

					handState.restPoint.target = clamp(Random::get_float(-0.5f, 0.4f), 0.0f, 1.0f);
				}
			}
			else
			{
				handState.restPoint.target = 0.0f;
			}
		}

		if (equipment == mainEquipment)
		{
			// in use, rest point is empty then
			handState.restPointEmpty.target = 1.0f;
		}
		else
		{
			// not in use, then rest point is not empty
			handState.restPointEmpty.target = 0.0f;
		}

		if (handState.restPoint.state == 1.0f)
		{
			handState.restPointEmpty.state = handState.restPointEmpty.target;
		}
	}

}

void ModulePilgrim::AdvanceHandContext::update_pose()
{
	BlendTimes useBlendTimes = handState.shouldBeParalysed ? modulePilgrim->paralysedBlendTimes : modulePilgrim->normalBlendTimes;

	bool fingersHandled = false;

	handState.allowHandTrackingPose.target = handState.inputAllowHandTrackingPose;
	if (handState.shouldBeParalysed ||
		(handState.currentState == HandState::HoldingObject &&
		 modulePilgrim->get_hand_equipment(idxAsHand) != modulePilgrim->get_main_equipment(idxAsHand)))
	{
		handState.allowHandTrackingPose.target = 0.0f;
	}

	// pretend some poses
	if (handState.fakePointing.target > 0.0f && handState.currentState == HandState::EmptyHand)
	{
		handState.inputSeparateFingers = true;
		handState.relaxedThumb.target = 0.0f;
		handState.relaxedPointer.target = 0.0f;
		handState.relaxedMiddle.target = 0.0f;
		handState.relaxedRing.target = 0.0f;
		handState.relaxedPinky.target = 0.0f;
		handState.straightThumb.target = 0.0f;
		handState.straightPointer.target = 1.0f;
		handState.straightMiddle.target = 0.0f;
		handState.straightRing.target = 0.0f;
		handState.straightPinky.target = 0.0f;

		handState.fist.target = 1.0f;

		fingersHandled = true;
	}

	handState.holdGun.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime);
	handState.pressTrigger.blend_to_target(useBlendTimes.handPoseBlendTime * 0.1f, deltaTime); // quicker than a standard blend
	handState.awayTrigger.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime); // quicker than a standard blend
	handState.holdObject.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime);
	handState.fist.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime);
	handState.grab.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime);
	handState.grabIsDial.blend_to_target(useBlendTimes.handPoseBlendTime * 0.1f, deltaTime); // quicker, we choose between two
	handState.straight.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime);
	handState.fakePointing.blend_to_target(useBlendTimes.handPoseBlendTime, deltaTime);
	handState.port.blend_to_target(useBlendTimes.portFoldTime, deltaTime);
	handState.displayNormal.blend_to_target(useBlendTimes.displayFoldTime, deltaTime);
	handState.displayExtra.blend_to_target(useBlendTimes.displayFoldTime, deltaTime);
	handState.restPoint.blend_to_target(useBlendTimes.restPointFoldTime, deltaTime);
	handState.restPointEmpty.blend_to_target(0.1f, deltaTime);

	if (!fingersHandled)
	{
		if (handState.inputSeparateFingers && handState.currentState == HandState::EmptyHand)
		{
			handState.relaxedThumb.target = handState.inputRelaxedThumb;
			handState.relaxedPointer.target = handState.inputRelaxedPointer;
			handState.relaxedMiddle.target = handState.inputRelaxedMiddle;
			handState.relaxedRing.target = handState.inputRelaxedRing;
			handState.relaxedPinky.target = handState.inputRelaxedPinky;
			handState.straightThumb.target = handState.inputStraightThumb;
			handState.straightPointer.target = handState.inputStraightPointer;
			handState.straightMiddle.target = handState.inputStraightMiddle;
			handState.straightRing.target = handState.inputStraightRing;
			handState.straightPinky.target = handState.inputStraightPinky;
		}
		else
		{
			handState.relaxedThumb.target = 0.0f;
			handState.relaxedPointer.target = 0.0f;
			handState.relaxedMiddle.target = 0.0f;
			handState.relaxedRing.target = 0.0f;
			handState.relaxedPinky.target = 0.0f;
			handState.straightThumb.target = 0.0f;
			handState.straightPointer.target = 0.0f;
			handState.straightMiddle.target = 0.0f;
			handState.straightRing.target = 0.0f;
			handState.straightPinky.target = 0.0f;
		}
	}

	handState.allowHandTrackingPose.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.relaxedThumb.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.relaxedPointer.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.relaxedMiddle.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.relaxedRing.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.relaxedPinky.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.straightThumb.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.straightPointer.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.straightMiddle.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.straightRing.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
	handState.straightPinky.blend_to_target(useBlendTimes.fingerBlendTime, deltaTime);
}

void ModulePilgrim::AdvanceHandContext::set_pose_vars()
{
	float pressTrigger = handState.holdGun.state * (1.0f - sqr(1.0f - handState.pressTrigger.state));
	float awayTrigger = handState.holdGun.state * handState.awayTrigger.state;
	if (get_hand())
	{
		float mul = 1.0f;
		if (handState.inputSeparateFingers)
		{
			float maxState = handState.holdGun.state;
			maxState = max(maxState, handState.pressTrigger.state);
			maxState = max(maxState, handState.awayTrigger.state);
			maxState = max(maxState, handState.holdObject.state);
			maxState = max(maxState, handState.fist.state);
			maxState = max(maxState, handState.grab.state);
			maxState = max(maxState, handState.straight.state);
			if (maxState < 1.0f && maxState > 0.0f)
			{
				mul = 1.0f / maxState;
			}
		}
		pressTrigger *= mul;
		awayTrigger *= mul;
		if (handState.handHoldGunVarID.is_valid())
		{
			handState.handHoldGunVarID.access<float>() = handState.holdGun.state * mul;
		}
		if (handState.handPressTriggerVarID.is_valid())
		{
			handState.handPressTriggerVarID.access<float>() = pressTrigger;
		}
		if (handState.handAwayTriggerVarID.is_valid())
		{
			handState.handAwayTriggerVarID.access<float>() = awayTrigger;
		}
		if (handState.handHoldObjectVarID.is_valid())
		{
			handState.handHoldObjectVarID.access<float>() = handState.holdObject.state * mul;
		}
		if (handState.handHoldObjectSizeVarID.is_valid())
		{
			handState.handHoldObjectSizeVarID.access<float>() = handState.holdObjectSize;
		}
		if (handState.handFistVarID.is_valid())
		{
			handState.handFistVarID.access<float>() = handState.fist.state * mul;
		}
		if (handState.handGrabVarID.is_valid())
		{
			handState.handGrabVarID.access<float>() = handState.grab.state * (1.0f - handState.grabIsDial.state) * mul;
		}
		if (handState.handGrabDialVarID.is_valid())
		{
			handState.handGrabDialVarID.access<float>() = handState.grab.state * handState.grabIsDial.state * mul;
		}
		if (handState.handStraightVarID.is_valid())
		{
			handState.handStraightVarID.access<float>() = handState.straight.state * mul;
		}
		if (handState.allowHandTrackingPoseVarID.is_valid())
		{
			handState.allowHandTrackingPoseVarID.access<float>() = handState.allowHandTrackingPose.state;
		}
		if (handState.relaxedThumbVarID.is_valid())
		{
			handState.relaxedThumbVarID.access<float>() = handState.relaxedThumb.state;
		}
		if (handState.relaxedPointerVarID.is_valid())
		{
			handState.relaxedPointerVarID.access<float>() = handState.relaxedPointer.state;
		}
		if (handState.relaxedMiddleVarID.is_valid())
		{
			handState.relaxedMiddleVarID.access<float>() = handState.relaxedMiddle.state;
		}
		if (handState.relaxedRingVarID.is_valid())
		{
			handState.relaxedRingVarID.access<float>() = handState.relaxedRing.state;
		}
		if (handState.relaxedPinkyVarID.is_valid())
		{
			handState.relaxedPinkyVarID.access<float>() = handState.relaxedPinky.state;
		}
		if (handState.straightThumbVarID.is_valid())
		{
			handState.straightThumbVarID.access<float>() = handState.straightThumb.state;
		}
		if (handState.straightPointerVarID.is_valid())
		{
			handState.straightPointerVarID.access<float>() = handState.straightPointer.state;
		}
		if (handState.straightMiddleVarID.is_valid())
		{
			handState.straightMiddleVarID.access<float>() = handState.straightMiddle.state;
		}
		if (handState.straightRingVarID.is_valid())
		{
			handState.straightRingVarID.access<float>() = handState.straightRing.state;
		}
		if (handState.straightPinkyVarID.is_valid())
		{
			handState.straightPinkyVarID.access<float>() = handState.straightPinky.state;
		}
		if (handState.handPortActiveVarID.is_valid())
		{
			handState.handPortActiveVarID.access<float>() = handState.port.state;
		}
		if (handState.handDisplayActiveNormalVarID.is_valid())
		{
			handState.handDisplayActiveNormalVarID.access<float>() = BlendCurve::cubic(handState.displayNormal.state);
		}
		if (handState.handDisplayActiveExtraVarID.is_valid())
		{
			handState.handDisplayActiveExtraVarID.access<float>() = BlendCurve::cubic(clamp(handState.displayExtra.state, 0.0f, 1.0f - handState.displayNormal.state));
		}
	}

	if (auto* restPoint = modulePilgrim->get_rest_point(idxAsHand))
	{
		if (handState.restPointHandObjectVarID.is_valid())
		{
			handState.restPointHandObjectVarID.access<SafePtr<Framework::IModulesOwner>>() = hand;
		}
		if (handState.restPointEquipmentObjectVarID.is_valid())
		{
			handState.restPointEquipmentObjectVarID.access<SafePtr<Framework::IModulesOwner>>() = mainEquipment;
		}
		if (handState.restPointTVarID.is_valid())
		{
			handState.restPointTVarID.access<float>() = handState.restPoint.state;
		}
		if (handState.restPointEmptyVarID.is_valid())
		{
			handState.restPointEmptyVarID.access<float>() = handState.restPointEmpty.state;
		}
	}

	if (equipment.get())
	{
		equipment->access_variables().access<float>(NAME(pressedTrigger)) = pressTrigger;
	}
}

#ifdef AN_DEVELOPMENT_OR_PROFILER
Optional<Transform> handPlacementOnGrabRelative[Hand::MAX];
#endif

void ModulePilgrim::AdvanceHandContext::update_grabable()
{
	if (handState.currentState == HandState::HoldingObject &&
		handState.objectGrabbed.is_set() &&
		get_hand())
	{
		if (auto * grabable = handState.objectGrabbed->get_custom<CustomModules::Grabable>())
		{
			if (auto* presence = modulePilgrim->get_owner()->get_presence())
			{
				// get placement in owner (pilgrim) space
				Transform handOwnerOS = presence->get_requested_relative_hand_for_vr(handIdx);
				handOwnerOS.set_translation(handOwnerOS.get_translation() + Vector3(0.0f, 0.0f, presence->get_vertical_look_offset()));
				Transform ownerWS = AVOID_CALLING_ACK_ presence->calculate_current_placement();
				Transform handOwnerWS = ownerWS.to_world(handOwnerOS);
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!VR::IVR::get())
				{
					handOwnerWS = hand->get_presence()->get_placement();
				}
#endif

				// into hand space
				Transform handHandWS = hand->get_presence()->get_path_to_attached_to()->from_target_to_owner(handOwnerWS);

#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (!VR::IVR::get())
				{
					// won't work properly if we move through doors
					Transform cameraWS = ownerWS.to_world(presence->get_eyes_relative_look());
					if (!handPlacementOnGrabRelative[handIdx].is_set())
					{
						handPlacementOnGrabRelative[handIdx] = cameraWS.to_local(handHandWS);
					}
					handHandWS = cameraWS.to_world(handPlacementOnGrabRelative[handIdx].get());
#ifdef AN_STANDARD_INPUT
					// move up or down
					{
						float dist = 0.0f;
						if (::System::Input::get()->is_key_pressed(handIdx == 0 ? System::Key::Q : System::Key::E))
						{
							if (::System::Input::get()->is_key_pressed(System::Key::LeftAlt))
							{
								dist = 0.2f;
							}
							if (::System::Input::get()->is_key_pressed(System::Key::RightAlt))
							{
								dist = -0.2f;
							}
						}
						if (dist != 0.0f)
						{
							// move up/forward or down/back
							handHandWS.set_translation(handHandWS.get_translation() + cameraWS.vector_to_world(Vector3(0.0f, dist, dist)));
						}
					}
#endif
				}
#endif

				// and into grabbed object's space
				Transform handGrabbedWS = handState.objectGrabbedPath.from_owner_to_target(handHandWS);

				grabable->process_control(handGrabbedWS);

				// we use calculate_current_placement as attachments are updated a bit later but we need the current one
				Transform objectGrabbedPlacementAsIsWS = handState.objectGrabbed->get_presence()->get_placement();
				Transform objectGrabbedPlacementWS = AVOID_CALLING_ACK_ handState.objectGrabbed->get_presence()->calculate_current_placement();

				// get location of axis (it might be not aligned with centre of presence!)
				Vector3 objectGrabbedWS = objectGrabbedPlacementWS.location_to_world(objectGrabbedPlacementAsIsWS.location_to_local(handState.objectGrabbed->get_presence()->get_centre_of_presence_WS()));
				{
					Name grabableSocket = grabable->get_grabable_axis_socket_name();
					if (!grabableSocket.is_valid())
					{
						grabableSocket = grabable->get_grabable_dial_socket_name();
					}
					if (grabableSocket.is_valid())
					{
						Transform grabbedGrabableAxisPlacementOS = handState.objectGrabbed->get_appearance()->calculate_socket_os(grabableSocket);
						objectGrabbedWS = objectGrabbedPlacementWS.location_to_world(grabbedGrabableAxisPlacementOS.get_translation());
					}
				}

				float distance = (handGrabbedWS.get_translation() - objectGrabbedWS).length();
				float maxDistance = max(GameSettings::get().settings.grabRelease, GameSettings::get().settings.grabDistance * 1.05f);

				if (!presence->is_requested_relative_hand_accurate(handIdx))
				{
					// if we're not sure where the hand is, make the distance much larger
					maxDistance = 1.5f;
				}

#ifdef DEBUG_GRABBING
				{
					debug_filter(pilgrimGrab);
					debug_context(presence->get_in_room());
					Vector3 s = handGrabbedWS.get_translation();
					Vector3 d = (objectGrabbedWS - s).normal();
					debug_draw_sphere(true, false, Colour::blue, 0.5f, Sphere(handHandWS.get_translation(), 0.1f));
					if (distance > maxDistance)
					{
						debug_draw_arrow(true, Colour::green, s, s + d * maxDistance);
						debug_draw_arrow(true, Colour::red, s + d * maxDistance, objectGrabbedWS);
					}
					else
					{
						debug_draw_arrow(true, Colour::green, s, objectGrabbedWS);
					}
					debug_no_context();
					debug_no_filter();
				}
#endif
				if (distance > maxDistance)
				{
#ifdef DEBUG_DRAW_GRAB
					debug_filter(pilgrimGrab);
					debug_context(presence->get_in_room());
					Vector3 s = handGrabbedWS.get_translation();
					Vector3 d = (objectGrabbedWS - s).normal();
					debug_draw_time_based(10.0f, debug_draw_sphere(true, false, Colour::blue, 0.5f, Sphere(handHandWS.get_translation(), 0.1f)));
					debug_draw_time_based(10.0f, debug_draw_arrow(true, Colour::green, s, s + d * maxDistance));
					debug_draw_time_based(10.0f, debug_draw_arrow(true, Colour::red, s + d * maxDistance, objectGrabbedWS));
					debug_no_context();
					debug_no_filter();
#endif
					handState.forceRelease = true;
				}
			}
			if (grabable->should_be_forced_released())
			{
				handState.forceRelease = true;
			}
		}
	}
	else
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		handPlacementOnGrabRelative[handIdx].clear();
#endif
	}
}

void ModulePilgrim::AdvanceHandContext::post_advance()
{
	handState.prevInputGrab = handState.inputGrab;
	handState.prevInputRelease = handState.inputRelease;
	handState.prevInputPullEnergy = handState.inputPullEnergy;
	handState.prevInputPullEnergyAlt = handState.inputPullEnergyAlt;
	handState.prevInputDeployMainEquipment = handState.inputDeployMainEquipment;
	handState.prevInputUseEquipment = handState.inputUseEquipment;
	handState.prevInputUseAsUsableEquipment = handState.inputUseAsUsableEquipment;
}

void ModulePilgrim::drop(Framework::IModulesOwner* _item)
{
	if (get_main_equipment(Hand::Left) == _item)
	{
		set_main_equipment(Hand::Left, nullptr);
	}
	if (get_main_equipment(Hand::Right) == _item)
	{
		set_main_equipment(Hand::Right, nullptr);
	}
	if (get_hand_equipment(Hand::Right) == _item)
	{
		use(Hand::Right, nullptr);
		return;
	}
	if (get_hand_equipment(Hand::Left) == _item)
	{
		use(Hand::Right, nullptr);
		return;
	}

	{
		MODULE_OWNER_LOCK_FOR_IMO(_item, TXT("ModulePilgrim::drop _item"));
		bool detach = true;
		if (auto* pickup = _item->get_custom<CustomModules::Pickup>())
		{
			pickup->set_can_be_picked_up(true);
			pickup->be_held_by(nullptr);
		}
		/*
		if (auto* grabable = _item->get_custom<CustomModules::Grabable>())
		{
			detach = false;
			grabable->set_can_be_grabbed(true);
			grabable->set_grabbed(get_hand(_hand), false);
		}
		*/
		if (auto* mse = _item->get_gameplay_as<ModuleEquipment>())
		{
			mse->set_user(nullptr);
		}
		_item->access_variables().access<float>(NAME(pressedTrigger)) = 0.0f;
		if (auto* mc = _item->get_collision())
		{
			if (useCollisionFlagsForUsedEquipment.is_set())
			{
				mc->pop_collision_flags(NAME(equipmentInUse));
				mc->update_collidable_object();
			}
			// don't collide with
			mc->clear_dont_collide_with();
			float keepForTime = 0.2f;
			for_count(int, handIdx, Hand::MAX)
			{
				Hand::Type hand = (Hand::Type)handIdx;
				if (auto* me = get_main_equipment(hand))
				{
					mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(me), keepForTime);
				}
				mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(get_hand(hand)), keepForTime);
				if (auto* ohe = get_hand_equipment(hand))
				{
					mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(ohe), keepForTime);
				}
				mc->dont_collide_with_until_no_collision_up_to_top_instigator(get_hand(hand), keepForTime);
			}
			mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(get_owner()), keepForTime); // never collide with main body
		}
		if (detach)
		{
			if (auto* presence = _item->get_presence())
			{
				// check if we're still attached, maybe someone stole us
				if (presence->is_attached_at_all_to(get_owner()))
				{
					Vector3 velocity = Vector3::zero;
					Rotator3 orientationVelocity = Rotator3::zero;
					presence->set_velocity_linear(velocity);
					presence->set_velocity_rotation(orientationVelocity);
					presence->store_velocities(0);
					presence->detach();
				}
			}
			_item->set_instigator(nullptr);
			_item->be_autonomous();
			if (auto* e = _item->get_gameplay_as<ModuleEquipment>())
			{
				e->set_failsafe_equipment_hand(NP);
			}
		}
	}
}

void ModulePilgrim::use(Hand::Type _hand, Framework::IModulesOwner* _item)
{
	Hand::Type otherHand = _hand == Hand::Left ? Hand::Right : Hand::Left;
	SafePtr<Framework::IModulesOwner>& handEquipment = access_hand_equipment(_hand);
	Framework::IModulesOwner* mainHandEquipment = get_main_equipment(_hand);
	if (handEquipment == _item)
	{
		return;
	}
	
	auto & handState = handStates[_hand];

	if (_item && handState.objectToGrab == _item)
	{
#ifdef DEBUG_GRABBING
		output(TXT("[pilgrim-grab] grab \"%S\""), _item->ai_get_name().to_char());
#endif
		handState.objectGrabbed = handState.objectToGrab;
		handState.objectGrabbedPath = handState.objectToGrabPath;
	}
	else
	{
#ifdef DEBUG_GRABBING
		output(TXT("[pilgrim-grab] release"));
#endif
		handState.objectGrabbed.clear();
		handState.objectGrabbedPath.reset();
	}

	if (handEquipment.is_set())
	{
		MODULE_OWNER_LOCK_FOR_IMO(handEquipment.get(), TXT("ModulePilgrim::use handEquipment"));
		bool detach = true;
		if (auto * pickup = handEquipment->get_custom<CustomModules::Pickup>())
		{
			pickup->set_can_be_picked_up(true);
			pickup->be_held_by(nullptr);
		}
		if (auto * grabable = handEquipment->get_custom<CustomModules::Grabable>())
		{
			detach = false;
			grabable->set_can_be_grabbed(true);
			grabable->set_grabbed(get_hand(_hand), false);
		}
		if (auto * mse = handEquipment->get_gameplay_as<ModuleEquipment>())
		{
			mse->set_user(nullptr);
		}
		handEquipment->access_variables().access<float>(NAME(pressedTrigger)) = 0.0f;
		if (auto * mc = handEquipment->get_collision())
		{
			if (useCollisionFlagsForUsedEquipment.is_set())
			{
				mc->pop_collision_flags(NAME(equipmentInUse));
				mc->update_collidable_object();
			}
			// don't collide with
			mc->clear_dont_collide_with();
			float keepForTime = 0.2f;
			mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(mainHandEquipment), keepForTime);
			mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(get_hand(_hand)), keepForTime);
			{	// other hand too! in case we're crossing our hands
				if (auto* ohme = get_main_equipment(otherHand))
				{
					mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(ohme), keepForTime);
				}
				if (auto* ohe = get_hand_equipment(otherHand))
				{
					mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(ohe), keepForTime);
				}
				mc->dont_collide_with_until_no_collision(fast_cast<Collision::ICollidableObject>(get_hand(otherHand)), keepForTime);
			}
			mc->dont_collide_with_until_no_collision_up_to_top_instigator(get_hand(_hand), keepForTime);
			mc->dont_collide_with(fast_cast<Collision::ICollidableObject>(get_owner())); // never collide with main body
			if (Framework::IModulesOwner * goUp = handEquipment.get())
			{
				if (goUp) 
				{
					goUp = goUp->get_presence()->get_attached_to();
				}
				auto* holdingHand = get_hand(_hand);
				while (goUp && goUp != holdingHand)
				{
					if (auto * mc = goUp->get_collision())
					{
						// don't collide with hand
						float keepForTime = 0.2f;
						auto* holdingHandICO = fast_cast<Collision::ICollidableObject>(holdingHand);
						mc->clear_dont_collide_with(holdingHandICO);
						mc->dont_collide_with_until_no_collision(holdingHandICO, keepForTime);
					}
					goUp = goUp->get_presence()->get_attached_to();
				}
			}
		}
		if (detach)
		{
			if (handEquipment == mainHandEquipment)
			{
				todo_note(TXT("make it attach to that little arm instead of body, isn't that already done?"));
				Name handHoldPointSocket = modulePilgrimData->get_rest_hold_point_sockets().get(_hand);
				Name equipmentHoldPointSocket = handHoldPointSocket;
				Name equipmentRestPointSocket = modulePilgrimData->get_rest_hold_point_sockets().get(_hand);
				auto const * heMesh = handEquipment->get_appearance()->get_mesh();
				if (!heMesh->has_socket(equipmentHoldPointSocket))
				{
					equipmentHoldPointSocket = modulePilgrimData->get_rest_hold_point_sockets().get();
				}
				auto * restPoint = get_rest_point(_hand);
				auto const * rpMesh = restPoint->get_appearance()->get_mesh();
				if (!rpMesh->has_socket(equipmentRestPointSocket))
				{
					equipmentRestPointSocket = modulePilgrimData->get_rest_hold_point_sockets().get();
				}
				Transform relativePlacement = heMesh->calculate_socket_using_ms(equipmentHoldPointSocket).inverted();
				handEquipment->get_presence()->attach_to_socket(restPoint, equipmentRestPointSocket, false, relativePlacement);
				handEquipment->set_instigator(get_owner());
				handEquipment->be_non_autonomous();
				if (auto* e = handEquipment->get_gameplay_as<ModuleEquipment>())
				{
					e->set_failsafe_equipment_hand(_hand);
				}
			}
			else
			{
				if (auto* presence = handEquipment->get_presence())
				{
					// check if we're still attached, maybe someone stole us
					if (presence->is_attached_at_all_to(get_owner()))
					{
						Vector3 velocity = presence->calc_average_velocity();
						Rotator3 orientationVelocity = presence->get_velocity_rotation();
						if (auto* pickup = handEquipment->get_custom<CustomModules::Pickup>())
						{
							velocity *= pickup->get_throw_velocity_muliplier();
							orientationVelocity *= pickup->get_throw_rotation_muliplier();
						}
						if (auto* eq = handEquipment->get_gameplay_as<ModuleEquipment>())
						{
							velocity += presence->get_placement().vector_to_world(eq->get_release_velocity() * Vector3(_hand == Hand::Left ? -1.0f : 1.0f, 1.0f, 1.0f));
						}
						/*
						debug_context(presence->get_in_room());
						debug_draw_time_based(10.0f, debug_draw_arrow(true, Colour::green, presence->get_centre_of_presence_WS(), presence->get_centre_of_presence_WS() + velocity.normal() * 0.2f));
						debug_no_context();
						*/
						presence->set_velocity_linear(velocity);
						presence->set_velocity_rotation(orientationVelocity);
						presence->store_velocities(0);
						presence->detach();
					}
				}
				handEquipment->set_instigator(nullptr);
				handEquipment->be_autonomous();
				if (auto* e = handEquipment->get_gameplay_as<ModuleEquipment>())
				{
					e->set_failsafe_equipment_hand(NP);
				}
			}
		}
		handEquipment = nullptr;
		if (Framework::GameUtils::is_local_player(get_owner()))
		{
			PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::ReleasedSomething);
			s.for_hand(_hand);
			PhysicalSensations::start_sensation(s);
		}
	}

	// always clear here
	overlayInfo.clear_in_hand_equipment_stats(_hand);

	if (_item && modulePilgrimData)
	{
		MODULE_OWNER_LOCK_FOR_IMO(_item, TXT("ModulePilgrim::use _item"));
		bool attach = true;
		handEquipment = _item;
		if (Framework::GameUtils::is_local_player(get_owner()))
		{
			PhysicalSensations::SingleSensation s(PhysicalSensations::SingleSensation::GrabbedSomething);
			s.for_hand(_hand);
			PhysicalSensations::start_sensation(s);
		}
		if (auto * pickup = handEquipment->get_custom<CustomModules::Pickup>())
		{
			pickup->set_can_be_picked_up(false);
			pickup->be_held_by(get_owner(), get_hand(_hand));
			if (handEquipment != mainHandEquipment)
			{
				if (Framework::GameUtils::is_local_player(get_owner()))
				{
					GameStats::get().picked_up_item(handEquipment.get());
				}
			}
			if (!handEquipment->get_gameplay_as<ModuleEquipments::Gun>())
			{
				// we're holding a pickup item
				timeSinceLastPickupInHand = 0.0f;
			}
		}
		if (auto * grabable = handEquipment->get_custom<CustomModules::Grabable>())
		{
			attach = false;
			grabable->set_can_be_grabbed(false);
			grabable->set_grabbed(get_hand(_hand), true);
		}
		if (auto * mse = handEquipment->get_gameplay_as<ModuleEquipment>())
		{
			mse->set_user(this, _hand);
		}
		if (auto * mc = handEquipment->get_collision())
		{
			mc->clear_dont_collide_with();
			mc->dont_collide_with(fast_cast<Collision::ICollidableObject>(get_owner())); // never collide with main body
		}
		if (auto * mc = handEquipment->get_collision())
		{
			if (useCollisionFlagsForUsedEquipment.is_set())
			{
				Collision::Flags setFlags = Collision::Flags::none();
				if (useCollisionFlagsForUsedEquipmentToBeKept.is_set())
				{
					setFlags = mc->get_collision_flags() & useCollisionFlagsForUsedEquipmentToBeKept.get();
				}
				bool nonInteractive = false;
				if (auto * mse = handEquipment->get_gameplay_as<ModuleEquipment>())
				{
					nonInteractive = mse->is_non_interactive();
				}
				if (nonInteractive && useCollisionFlagsForUsedEquipmentNonInteractive.is_set())
				{
					setFlags = setFlags | useCollisionFlagsForUsedEquipmentNonInteractive.get();
				}
				else
				{
					setFlags = setFlags | useCollisionFlagsForUsedEquipment.get();
				}
				mc->push_collision_flags(NAME(equipmentInUse), setFlags);
				mc->update_collidable_object();
			}
			if (!attach)
			{
				// don't collide with hand or equipment as we will be holding it and we don't want any extra interaction
				mc->dont_collide_with(fast_cast<Collision::ICollidableObject>(mainHandEquipment));
				mc->dont_collide_with(fast_cast<Collision::ICollidableObject>(get_hand(_hand)));
				if (Framework::IModulesOwner * goUp = handEquipment.get())
				{
					if (goUp)
					{
						goUp = goUp->get_presence()->get_attached_to();
					}
					while (goUp)
					{
						if (auto * mc = goUp->get_collision())
						{
							// don't collide with hand
							mc->dont_collide_with(fast_cast<Collision::ICollidableObject>(get_hand(_hand)));
						}
						goUp = goUp->get_presence()->get_attached_to();
					}
				}
			}
		}
		if (attach && handEquipment.get())
		{
			if (auto* hand = get_hand(_hand))
			{
				auto const * handMesh = hand->get_appearance()->get_mesh();
				auto const * heMesh = handEquipment->get_appearance()->get_mesh();
				bool usingGrabs = false;
				float grabbedObjectSize = 0.0f;
				// try port hold points first
				Name handHoldPointSocket = modulePilgrimData->get_port_hold_point_sockets().get(_hand);
				Name equipmentHoldPointSocket = handHoldPointSocket;
				if (!handMesh->has_socket(handHoldPointSocket))
				{
					handHoldPointSocket = modulePilgrimData->get_port_hold_point_sockets().get();
				}
				if (!heMesh->has_socket(equipmentHoldPointSocket))
				{
					equipmentHoldPointSocket = modulePilgrimData->get_port_hold_point_sockets().get();
				}
				// if equipment does not have port, try hand
				if (!heMesh->has_socket(equipmentHoldPointSocket))
				{
					// try hand hold points
					handHoldPointSocket = modulePilgrimData->get_hand_hold_point_sockets().get(_hand);
					if (!handMesh->has_socket(handHoldPointSocket))
					{
						handHoldPointSocket = modulePilgrimData->get_hand_hold_point_sockets().get();
					}
					equipmentHoldPointSocket = modulePilgrimData->get_hand_hold_point_sockets().get(_hand);
					if (!heMesh->has_socket(equipmentHoldPointSocket))
					{
						equipmentHoldPointSocket = modulePilgrimData->get_hand_hold_point_sockets().get();
					}
				}
				// if equipment does not have hand, try grab
				if (!heMesh->has_socket(equipmentHoldPointSocket))
				{
					usingGrabs = true;
					if (auto* size = handEquipment->get_variables().get_existing<float>(objectsHeldObjectSizeVarID))
					{
						grabbedObjectSize = *size;
					}
					// try hand hold points
					handHoldPointSocket = modulePilgrimData->get_hand_grab_point_sockets().get(_hand);
					if (!handMesh->has_socket(handHoldPointSocket))
					{
						handHoldPointSocket = modulePilgrimData->get_hand_grab_point_sockets().get();
					}
					equipmentHoldPointSocket = modulePilgrimData->get_hand_grab_point_sockets().get(_hand);
					if (!heMesh->has_socket(equipmentHoldPointSocket))
					{
						equipmentHoldPointSocket = modulePilgrimData->get_hand_grab_point_sockets().get();
					}
				}
				Transform relativePlacement = Transform::identity;
				if (usingGrabs) // we choose to use two directionals
				{
					::Framework::PresencePath pp;
					pp.be_temporary_snapshot();
					if (pp.find_path(_item, hand)) // we're attaching item to hand
					{
						auto const * appearance = _item->get_appearance();
						auto const * ourAppearance = hand->get_appearance();
						if (appearance && ourAppearance)
						{
							auto const * mesh = appearance->get_mesh();
							auto const * ourMesh = ourAppearance->get_mesh();
							if (mesh && ourMesh)
							{
								int toSocketIdx = ourMesh->find_socket_index(handHoldPointSocket);
								int viaSocketIdx = mesh->find_socket_index(equipmentHoldPointSocket);
								if (toSocketIdx != NONE && viaSocketIdx != NONE)
								{
									Transform toSocketPlacementWS = hand->get_presence()->get_placement().to_world(ourAppearance->calculate_socket_os(toSocketIdx));
									toSocketPlacementWS = pp.from_target_to_owner(toSocketPlacementWS);
									Transform viaSocketPlacementWS = _item->get_presence()->get_placement().to_world(appearance->calculate_socket_os(viaSocketIdx));
									// tSS socket space "toSocket"'s space (which is hand's socket)
									Transform equipmentPlacement_tSS = toSocketPlacementWS.to_local(viaSocketPlacementWS);

									// equipmentAxis -> equipment's socket axis
									// handAxis -> hand's socket axis
									Axis::Type equipmentAxis = Axis::Forward; // hard coded! same as in AC::PilgrimsGrabbingHand
									Axis::Type equipmentOtherAxis = Axis::Up; // hard coded! up should remain up even if forward would flip
									Axis::Type handAxis = modulePilgrimData->get_hand_grab_point(_hand).axis;
									Axis::Type towardsObjectAxis = modulePilgrimData->get_hand_grab_point(_hand).dirAxis;
									float towardsObjectAxisDir = modulePilgrimData->get_hand_grab_point(_hand).dirSign;
									float sameDirDot = Vector3::dot(equipmentPlacement_tSS.get_axis(equipmentAxis), Vector3::axis(handAxis));
									float sameDir = sign(sameDirDot);

									// we want to calculate preferred location of "toSocket" in our space
									Vector3 locSS = Vector3::axis(towardsObjectAxis) * towardsObjectAxisDir * grabbedObjectSize * 0.5f;

									// we just want to align equipmentAxis with handAxis
									Transform equipmentPrefferedPlacement_tSS;
									equipmentPrefferedPlacement_tSS.build_from_two_axes(equipmentAxis, equipmentOtherAxis, Vector3::axis(handAxis) * sameDir, equipmentPlacement_tSS.get_axis(equipmentOtherAxis), locSS);

									relativePlacement = equipmentPrefferedPlacement_tSS;
									/*
									debug_context(_item->get_presence()->get_in_room());
									debug_draw_time_based(1000.0f, debug_draw_transform_size(true, toSocketPlacementWS, 0.1f));
									debug_draw_time_based(1000.0f, debug_draw_transform_size(true, viaSocketPlacementWS, 0.1f));
									debug_draw_time_based(1000.0f, debug_draw_arrow(true, Colour::purple, viaSocketPlacementWS.get_translation(), toSocketPlacementWS.get_translation()));
									debug_draw_time_based(1000.0f, debug_draw_transform_size(true, toSocketPlacementWS.to_world(equipmentPrefferedPlacement_tSS), 0.2f));
									//debug_draw_time_based(1000.0f, debug_draw_transform_size(true, viaSocketPlacementWS.to_world(relativePlacement), 0.2f));
									debug_no_context();
									*/

								}
							}
						}
					}
				}
				handEquipment->get_presence()->attach_via_socket(hand, equipmentHoldPointSocket, handHoldPointSocket, false, relativePlacement);
				handEquipment->set_instigator(hand);
				handEquipment->be_non_autonomous();
				if (auto* mse = handEquipment->get_gameplay_as<ModuleEquipment>())
				{
					mse->set_failsafe_equipment_hand(_hand);
				}
				if (handEquipment != mainHandEquipment)
				{
					handEquipment->get_presence()->store_velocities(4);
					//overlayInfo.show_in_hand_equipment_stats(_hand, NAME(itemEquipped), 5.0f);
				}
			}
		}
	}
}

bool ModulePilgrim::is_aiming_at(Framework::IModulesOwner const * imo, Framework::PresencePath const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const
{
	return is_aiming_at_impl(imo, _usePath, _distance, _maxOffPath, _maxAngleOff);
}

bool ModulePilgrim::is_aiming_at(Framework::IModulesOwner const * imo, Framework::RelativeToPresencePlacement const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const
{
	return is_aiming_at_impl(imo, _usePath, _distance, _maxOffPath, _maxAngleOff);
}

template <class PathClass>
bool ModulePilgrim::is_aiming_at_impl(Framework::IModulesOwner const * imo, PathClass const * _usePath, OUT_ float * _distance, float _maxOffPath, float _maxAngleOff) const
{
	assign_optional_out_param(_distance, 0.0f);
	bool aimingAt = false;
	for_count(int, i, 2)
	{
		if (auto* equipment = get_hand_equipment((Hand::Type)i))
		{
			if (auto * mse = equipment->get_gameplay_as<ModuleEquipment>())
			{
				aimingAt |= mse->is_aiming_at(imo, _usePath, OUT_ _distance, _maxOffPath, _maxAngleOff);
			}
		}
	}
	return aimingAt;
}

void ModulePilgrim::pre_process_controls()
{
	equipmentControlsPrev = equipmentControls;
	exmControlsPrev = exmControls;
}

void ModulePilgrim::process_controls(Hand::Type _hand, Framework::GameInput const & _controls, float _deltaTime)
{
	auto& handState = handStates[_hand];

	// auto hold to not release thing that we had when we entered a menu (or something else)
	if (Game::get()->was_recently_short_paused())
	{
#ifdef INVESTIGATE_MISSING_INPUT
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# was recently short paused, block certain controls"));
#endif
		handState.autoHoldDueToShortPause = true;
		handState.blockUsingEquipmentDueToShortPause = true;
		handState.blockUsingUsableEquipmentDueToShortPause = true;
		handState.blockDeployingMainEquipmentDueToShortPause = true;
		handState.blockUsingEXMDueToShortPause = true;
	}

	bool useVR = VR::IVR::get() != nullptr;
	Name grabEquipment = useVR? NAME(grabEquipment) : (_hand == Hand::Left ? NAME(grabEquipmentLeft) : NAME(grabEquipmentRight));
	Name pullEnergy = useVR ? NAME(pullEnergy) : (_hand == Hand::Left ? NAME(pullEnergyLeft) : NAME(pullEnergyRight));
	Name pullEnergyAlt = useVR ? NAME(pullEnergyAlt) : (_hand == Hand::Left ? NAME(pullEnergyAltLeft) : NAME(pullEnergyAltRight));
	Name sharedReleaseGrabDeploy = NAME(sharedReleaseGrabDeploy);
	Name releaseEquipment = useVR ? NAME(releaseEquipment) : (_hand == Hand::Left ? NAME(releaseEquipmentLeft) : NAME(releaseEquipmentRight));
	Name useEquipment = useVR ? NAME(useEquipment) : (_hand == Hand::Left ? NAME(useEquipmentLeft) : NAME(useEquipmentRight));
	Name useEquipmentAway = useVR ? NAME(useEquipmentAway) : (_hand == Hand::Left ? NAME(useEquipmentAwayLeft) : NAME(useEquipmentAwayRight));
	Name useAsUsableEquipment = useVR ? NAME(useAsUsableEquipment) : (_hand == Hand::Left ? NAME(useAsUsableEquipmentLeft) : NAME(useAsUsableEquipmentRight));
	Name useEXM = useVR ? NAME(useEXM) : (_hand == Hand::Left ? NAME(useEXMLeft) : NAME(useEXMRight));
	Name makeFist = useVR ? NAME(makeFist) : (_hand == Hand::Left ? NAME(makeFistLeft) : NAME(makeFistRight));
	Name deployMainEquipment = useVR ? NAME(deployMainEquipment) : (_hand == Hand::Left ? NAME(deployMainEquipmentLeft) : NAME(deployMainEquipmentRight));
	Name autoHoldEquipment = NAME(autoHoldEquipment);
	Name autoMainEquipment = NAME(autoMainEquipment);
	Name autoPointing = NAME(autoPointing);
	Name easyReleaseEquipment = useVR ? NAME(easyReleaseEquipment) : (_hand == Hand::Left ? NAME(easyReleaseEquipmentLeft) : NAME(easyReleaseEquipmentRight));
	Name separateFingers = NAME(separateFingers);
	Name allowHandTrackingPose = NAME(allowHandTrackingPose);
	Name relaxedThumb = NAME(relaxedThumb);
	Name relaxedPointer = NAME(relaxedPointer);
	Name relaxedMiddle = NAME(relaxedMiddle);
	Name relaxedRing = NAME(relaxedRing);
	Name relaxedPinky = NAME(relaxedPinky);
	Name straightThumb = NAME(straightThumb);
	Name straightPointer = NAME(straightPointer);
	Name straightMiddle = NAME(straightMiddle);
	Name straightRing = NAME(straightRing);
	Name straightPinky = NAME(straightPinky);

	handState.inputShowOverlayInfo = _controls.is_button_pressed(NAME(showOverlayInfo));
	handState.inputOverlayInfoStick = _controls.get_stick(NAME(overlayInfoMove));
	Vector2 newPullEnergyAltStick = TutorialSystem::get_filtered_stick(_controls, pullEnergyAlt);
	if (blockControlsTimeLeft > 0.0f)
	{
#ifdef INVESTIGATE_MISSING_INPUT
		REMOVE_AS_SOON_AS_POSSIBLE_ output(TXT("!@# blockControlsTimeLeft %.3f"), blockControlsTimeLeft);
#endif
		blockControlsTimeLeft = max(0.0f, blockControlsTimeLeft - _deltaTime);
		handState.inputFist = 0.0f;
		handState.inputGrab = 0.0f;
		handState.inputPullEnergy = 0.0f;
		handState.inputPullEnergyAlt = 0.0f;
		handState.inputPullEnergyAltStick = newPullEnergyAltStick;
		handState.inputDeployMainEquipment = 0.0f;
		handState.inputRelease = 0.0f;
		handState.inputEasyReleaseEquipment = 0.0f;
		handState.inputSharedReleaseGrabDeploy = 0.0f;
		handState.inputUseEquipment = 0.0f;
		handState.inputUseEquipmentAway = 0.0f;
		handState.inputUseAsUsableEquipment = 0.0f;
		handState.inputUseEXM = 0.0f;
		handState.inputRelaxedThumb = 1.0f;
		handState.inputRelaxedPointer = 1.0f;
		handState.inputRelaxedMiddle = 1.0f;
		handState.inputRelaxedRing = 1.0f;
		handState.inputRelaxedPinky = 1.0f;
		handState.inputStraightThumb = 1.0f;
		handState.inputStraightPointer = 1.0f;
		handState.inputStraightMiddle = 1.0f;
		handState.inputStraightRing = 1.0f;
		handState.inputStraightPinky = 1.0f;
	}
	else
	{
		handState.inputFist = _controls.get_button_state(makeFist); // not filtered
		handState.inputGrab = TutorialSystem::get_filtered_button_state(_controls, grabEquipment);
		handState.inputPullEnergy = TutorialSystem::get_filtered_button_state(_controls, pullEnergy);
		handState.inputPullEnergyAlt = TutorialSystem::get_filtered_button_state(_controls, pullEnergyAlt);
		handState.inputDeployMainEquipment = TutorialSystem::get_filtered_button_state(_controls, deployMainEquipment);
		handState.inputRelease = _controls.get_button_state(releaseEquipment); // not filtered
		handState.inputEasyReleaseEquipment = _controls.get_button_state(easyReleaseEquipment); // not filtered
		handState.inputSharedReleaseGrabDeploy = TutorialSystem::get_filtered_button_state(_controls, sharedReleaseGrabDeploy);
		handState.inputUseEquipment = TutorialSystem::get_filtered_button_state(_controls, useEquipment);
		handState.inputUseEquipmentAway = TutorialSystem::get_filtered_button_state(_controls, useEquipmentAway);
		handState.inputUseAsUsableEquipment = TutorialSystem::get_filtered_button_state(_controls, useAsUsableEquipment);
		handState.inputUseEXM = TutorialSystem::get_filtered_button_state(_controls, useEXM);
		handState.inputRelaxedThumb = _controls.get_button_state(relaxedThumb); // not filtered
		handState.inputRelaxedPointer = _controls.get_button_state(relaxedPointer); // not filtered
		handState.inputRelaxedMiddle = _controls.get_button_state(relaxedMiddle); // not filtered
		handState.inputRelaxedRing = _controls.get_button_state(relaxedRing); // not filtered
		handState.inputRelaxedPinky = _controls.get_button_state(relaxedPinky); // not filtered
		handState.inputStraightThumb = _controls.get_button_state(straightThumb); // not filtered
		handState.inputStraightPointer = _controls.get_button_state(straightPointer); // not filtered
		handState.inputStraightMiddle = _controls.get_button_state(straightMiddle); // not filtered
		handState.inputStraightRing = _controls.get_button_state(straightRing); // not filtered
		handState.inputStraightPinky = _controls.get_button_state(straightPinky); // not filtered
	}

	struct HandleBlockingDueToShortPause
	{
		static void handle(bool& _block, float& _input)
		{
			if (_block)
			{
				if (_input < 0.1f)
				{
					_block = false;
				}
				else
				{
					_input = 0.0f;
				}
			}
		}
	};
	HandleBlockingDueToShortPause::handle(handState.blockUsingEquipmentDueToShortPause, handState.inputUseEquipment);
	HandleBlockingDueToShortPause::handle(handState.blockUsingUsableEquipmentDueToShortPause, handState.inputUseAsUsableEquipment);
	HandleBlockingDueToShortPause::handle(handState.blockDeployingMainEquipmentDueToShortPause, handState.inputDeployMainEquipment);
	HandleBlockingDueToShortPause::handle(handState.blockUsingEXMDueToShortPause, handState.inputUseEXM);

#ifdef INVESTIGATE_MISSING_INPUT
	REMOVE_AS_SOON_AS_POSSIBLE_
	{
		if (handState.autoHoldDueToShortPause) output(TXT("!@# autoHoldDueToShortPause"));
		if (handState.blockUsingEquipmentDueToShortPause) output(TXT("!@# blockUsingEquipmentDueToShortPause"));
		if (handState.blockUsingUsableEquipmentDueToShortPause) output(TXT("!@# blockUsingUsableEquipmentDueToShortPause"));
		if (handState.blockDeployingMainEquipmentDueToShortPause) output(TXT("!@# blockDeployingMainEquipmentDueToShortPause"));
		if (handState.blockUsingEXMDueToShortPause) output(TXT("!@# blockUsingEXMDueToShortPause"));
	}
#endif

	handState.inputAutoHold = _controls.get_button_state(autoHoldEquipment);
	handState.inputAutoMainEquipment = _controls.get_button_state(autoMainEquipment);
	handState.inputAutoPointing = _controls.get_button_state(autoPointing);
	handState.inputSeparateFingers = _controls.get_button_state(separateFingers);
	handState.inputAllowHandTrackingPose = _controls.get_button_state(allowHandTrackingPose);

#ifdef AN_ALLOW_BULLSHOTS
	if (Framework::BullshotSystem::is_active())
	{
		// force
		handState.inputAutoHold = 1.0;
		auto& ov = get_owner()->get_variables();
		if (auto* v = ov.get_existing<float>(_hand == Hand::Left ? NAME(bullshotControlsMakeFistLeft) : NAME(bullshotControlsMakeFistRight)))
		{
			handState.inputFist = *v;
		}
	}
#endif

	if (auto* ps = PlayerSetup::access_current_if_exists())
	{
		ps->access_preferences().controllerAutoMainEquipment = handState.inputAutoMainEquipment > CONTROLS_THRESHOLD;
		handState.inputAutoMainEquipment = ps->get_preferences().should_auto_main_equipment() ? 1.0f : 0.0f; // even if controller does not want to
	}

	if ((newPullEnergyAltStick - handState.inputPullEnergyAltStick).length() > 0.6f // to only catch fast movement
		//&& (abs(newPullEnergyAltStick.x) > 0.9f)
		) // to only react when we hit the border quickly, and only vertical
	{
		handState.inputPullEnergyAlt = 1.0f - handState.inputPullEnergyAlt; // alternate
		handState.inputPullEnergyAltStick = newPullEnergyAltStick;
	}
	else
	{
		handState.inputPullEnergyAltStick = blend_to_using_time(handState.inputPullEnergyAltStick, newPullEnergyAltStick, 0.4f, _deltaTime); // to avoid when gently moving joystick around
	}

	if (handState.autoHoldDueToShortPause)
	{
		if (handState.inputAutoHold)
		{
			// we're auto holding anyway
			handState.autoHoldDueToShortPause = false;
		}
		else if (handState.currentState != HandState::HoldingObject)
		{
			// we're not holding anything
			handState.autoHoldDueToShortPause = false;
		}
		else if (handState.inputSharedReleaseGrabDeploy > CONTROLS_THRESHOLD)
		{
			// no need to auto hold, it is held automatically
			handState.autoHoldDueToShortPause = false;
		}
		else if (handState.inputGrab >= CONTROLS_THRESHOLD)
		{
			// we grabbed, clear auto hold as we are now holding
			handState.autoHoldDueToShortPause = false;
		}
		else
		{
			handState.inputGrab = 1.0f;
			handState.inputRelease = 0.0f;
		}
	}
	
	//

	SafePtr<Framework::IModulesOwner>& equipment = _hand == Hand::Left ? leftHandEquipment : rightHandEquipment;

	{
		equipmentControls.use[_hand] = weaponsBlocked? 0.0f : ((! equipment.is_set() || handState.blockUsingEquipment || handState.blockUsingEquipmentDueToPort)? 0.0f : handState.inputUseEquipment);
		equipmentControls.useAsUsable[_hand] = weaponsBlocked ? 0.0f : (! equipment.is_set() || handState.blockUsingEquipment)? 0.0f : handState.inputUseAsUsableEquipment;
		exmControls.use[_hand] = weaponsBlocked ? 0.0f : handState.inputUseEXM; // should use it even if hand is paralysed. EXMs are on forearm
	}
}

void ModulePilgrim::set_object_to_grab_proposition(Hand::Type _hand, Framework::IModulesOwner* _objectToGrab, Framework::DoorInRoomArrayPtr const * _throughDoors)
{
	if (handStates[_hand].objectToGrabProposition.is_set() && !handStates[_hand].objectToLateGrabPropositionStillValidFor.is_set() && !_objectToGrab)
	{
		handStates[_hand].objectToLateGrabPropositionStillValidFor = 0.15f; // this is to allow late grab
		handStates[_hand].objectToLateGrabProposition = handStates[_hand].objectToGrabProposition;
		handStates[_hand].objectToLateGrabPropositionPath = handStates[_hand].objectToGrabPropositionPath;
	}

	if (auto* o = handStates[_hand].objectToGrabProposition.get())
	{
		if (auto* p = o->get_custom<CustomModules::Pickup>())
		{
			p->mark_no_longer_interested_picking_up();
		}
	}

	if (auto* h = get_hand(_hand))
	{
		if (auto * hc = h->get_collision())
		{
			if (auto* colObj = fast_cast<Collision::ICollidableObject>(handStates[_hand].objectToGrabProposition.get()))
			{
				hc->clear_dont_collide_with(colObj);
			}
			if (auto* colObj = fast_cast<Collision::ICollidableObject>(_objectToGrab))
			{
				hc->dont_collide_with(colObj);
			}
		}
	}
	handStates[_hand].objectToGrabProposition = _objectToGrab;
	handStates[_hand].objectToGrabPropositionPath.set_owner(get_hand(_hand));
	handStates[_hand].objectToGrabPropositionPath.set_target(_objectToGrab);
	if (_throughDoors)
	{
		handStates[_hand].objectToGrabPropositionPath.set_through_doors(*_throughDoors);
	}
	if (_objectToGrab)
	{
		handStates[_hand].objectToLateGrabPropositionStillValidFor.clear();
		handStates[_hand].objectToLateGrabProposition.clear();
		handStates[_hand].objectToLateGrabPropositionPath.clear_target();
	}

	if (auto* o = handStates[_hand].objectToGrabProposition.get())
	{
		if (auto* p = o->get_custom<CustomModules::Pickup>())
		{
			p->mark_interested_picking_up();
		}
	}
}

bool ModulePilgrim::put_into_pocket(Framework::IModulesOwner* _object)
{
	for_every(pocket, pockets)
	{
		if (pocket->isActive && 
			!pocket->get_in_pocket())
		{
			put_into_pocket(pocket, _object);
			return true;
		}
	}
	return false;
}

void ModulePilgrim::put_into_pocket(Pocket* _pocket, Framework::IModulesOwner* _object)
{
	if (_object == get_main_equipment(Hand::Left))
	{
		// no longer main equipment
		set_main_equipment(Hand::Left, nullptr);
		update_pilgrim_setup_for(nullptr, true);
	}
	else if (_object == get_main_equipment(Hand::Right))
	{
		// no longer main equipment
		set_main_equipment(Hand::Right, nullptr);
		update_pilgrim_setup_for(nullptr, true);
	}

	if (auto* object = _pocket->get_in_pocket())
	{
		MODULE_OWNER_LOCK_FOR_IMO(object, TXT("ModulePilgrim::put_into_pocket  remove"));
		object->get_presence()->detach();
		object->set_instigator(nullptr);
		object->be_autonomous();
		if (auto* e = object->get_gameplay_as<ModuleEquipment>())
		{
			e->set_failsafe_equipment_hand(NP);
		}
	}
	_pocket->set_in_pocket(_object);
	if (auto* object = _pocket->get_in_pocket())
	{
		MODULE_OWNER_LOCK_FOR_IMO(object, TXT("ModulePilgrim::put_into_pocket  put in"));
		auto const * obMesh = object->get_appearance()->get_mesh();
		// try port hold points first
		Name objectHoldSocket;
		for_every(ohs, _pocket->objectHoldSockets)
		{
			if (!obMesh->has_socket(*ohs))
			{
				objectHoldSocket = *ohs;
			}
		}
		if (objectHoldSocket.is_valid())
		{
			object->get_presence()->attach_via_socket(get_owner(), objectHoldSocket, _pocket->socket.get_name(), true);
		}
		else
		{
			object->get_presence()->attach_to_socket(get_owner(), _pocket->socket.get_name(), true, object->get_presence()->get_centre_of_presence_os().inverted());
		}
		object->set_instigator(get_owner()); // to get the proper values
		object->be_autonomous();
		if (auto* e = object->get_gameplay_as<ModuleEquipment>())
		{
			an_assert(_pocket->side.is_set());
			e->set_failsafe_equipment_hand(_pocket->side);
		}
		if (auto* p = object->get_custom<CustomModules::Pickup>())
		{
			p->set_in_pocket_of(get_owner());
		}
	}
}

void ModulePilgrim::set_main_equipment(Hand::Type _type, Framework::IModulesOwner* _equipment)
{
	auto & mainEquipment = _type == Hand::Left ? leftMainEquipment : rightMainEquipment;

	if (auto * eq = mainEquipment.get())
	{
		MODULE_OWNER_LOCK_FOR_IMO(eq, TXT("ModulePilgrim::set_main_equipment  no longer"));
		if (auto * meq = eq->get_gameplay_as<ModuleEquipment>())
		{
			meq->be_main_equipment(false);
		}
	}
	//
	mainEquipment = _equipment;
	//
	if (auto* eq = mainEquipment.get())
	{
		MODULE_OWNER_LOCK_FOR_IMO(eq, TXT("ModulePilgrim::set_main_equipment  be main equipment"));
		if (auto * meq = eq->get_gameplay_as<ModuleEquipment>())
		{
			meq->be_main_equipment(true, _type);
		}
	}
}

bool ModulePilgrim::is_controls_use_equipment_pressed(Hand::Type _hand) const
{
	return equipmentControls.use[_hand] >= CONTROLS_THRESHOLD;
}

bool ModulePilgrim::has_controls_use_equipment_been_pressed(Hand::Type _hand) const
{
	return equipmentControls.use[_hand] >= CONTROLS_THRESHOLD && equipmentControlsPrev.use[_hand] < CONTROLS_THRESHOLD;
}

bool ModulePilgrim::is_controls_use_as_usable_equipment_pressed(Hand::Type _hand) const
{
	return equipmentControls.useAsUsable[_hand] >= CONTROLS_THRESHOLD;
}

bool ModulePilgrim::has_controls_use_as_usable_equipment_been_pressed(Hand::Type _hand) const
{
	return equipmentControls.useAsUsable[_hand] >= CONTROLS_THRESHOLD && equipmentControlsPrev.useAsUsable[_hand] < CONTROLS_THRESHOLD;
}

bool ModulePilgrim::is_controls_use_exm_pressed(Hand::Type _hand) const
{
	return exmControls.use[_hand] >= CONTROLS_THRESHOLD;
}

bool ModulePilgrim::has_controls_use_exm_been_pressed(Hand::Type _hand) const
{
	return exmControls.use[_hand] >= CONTROLS_THRESHOLD && exmControlsPrev.use[_hand] < CONTROLS_THRESHOLD;
}

Energy ModulePilgrim::calculate_items_energy(Optional<EnergyType::Type> const & _ofType) const
{
	Energy result = Energy::zero();
	result += Modules::calculate_total_energy_of(leftMainEquipment.get(), _ofType);
	result += Modules::calculate_total_energy_of(rightMainEquipment.get(), _ofType);
	if (leftHandEquipment != leftMainEquipment)
	{
		result += Modules::calculate_total_energy_of(leftHandEquipment.get(), _ofType);
	}
	if (rightHandEquipment != rightMainEquipment)
	{
		result += Modules::calculate_total_energy_of(rightHandEquipment.get(), _ofType);
	}
	for_every(pocket, pockets)
	{
		result += Modules::calculate_total_energy_of(pocket->get_in_pocket(), _ofType);
	}
	return result;
}

bool ModulePilgrim::can_consume_equipment_energy_for(Hand::Type _hand, Energy const& _amount) const
{
	Energy energyAvailable = get_hand_energy_storage((_hand));
	return energyAvailable >= _amount;
}

bool ModulePilgrim::consume_equipment_energy_for(Hand::Type _hand, Energy const& _amount)
{
	if (auto* h = get_hand(_hand))
	{
		if (auto* mph = h->get_gameplay_as<ModulePilgrimHand>())
		{
			if (get_hand_energy_storage((_hand)) >= _amount)
			{
				EnergyTransferRequest etr(EnergyTransferRequest::Withdraw);
				etr.instigator = get_owner();
				etr.energyRequested = _amount;
				etr.fillOrKill = true;
				mph->handle_ammo_energy_transfer_request(etr);
				if (etr.energyResult == _amount)
				{
					return true;
				}
				an_assert(etr.energyResult.is_zero());
			}
		}
	}
	return false;
}

bool ModulePilgrim::receive_equipment_energy_for(Hand::Type _hand, Energy const& _amount)
{
	Energy amount = _amount;
	if (!amount.is_zero())
	{
		if (auto* h = get_hand(_hand))
		{
			if (auto* mph = h->get_gameplay_as<ModulePilgrimHand>())
			{
				EnergyTransferRequest etr(EnergyTransferRequest::Deposit);
				etr.instigator = get_owner();
				etr.energyRequested = amount;
				mph->handle_ammo_energy_transfer_request(etr);
				amount = etr.energyRequested;
			}
		}
	}
	an_assert(amount.is_zero() || ! get_owner()->get_custom<CustomModules::Health>() || ! get_owner()->get_custom<CustomModules::Health>()->is_alive(), TXT("might be dead"));
	return false;
}

bool ModulePilgrim::does_hand_collide_with(Hand::Type _hand, Framework::ModuleCollision const* _col, Framework::IModulesOwner** _colliderWithHealth) const
{
	bool result = false;
	auto* h = get_hand(_hand);
	if (h && _col->does_collide_with(fast_cast<Collision::ICollidableObject>(h)))
	{
		result = true;
	}
	if (!result || _colliderWithHealth)
	{
		if (h)
		{
			Concurrency::ScopedSpinLock lock(h->get_presence()->access_attached_lock());
			for_every_ptr(attached, h->get_presence()->get_attached())
			{
				if (auto * ico = fast_cast<Collision::ICollidableObject>(attached))
				{
					if (_col->does_collide_with(ico))
					{
						result = true;
						if (_colliderWithHealth)
						{
							if (auto * h = attached->get_custom<CustomModules::Health>())
							{
								*_colliderWithHealth = attached;
								break;
							}
						}
						else
						{
							break;
						}
					}
				}
			}
		}
	}
	if (!result || _colliderWithHealth)
	{
		if (auto * eq = get_hand_equipment(_hand))
		{
			if (_col->does_collide_with(fast_cast<Collision::ICollidableObject>(eq)))
			{
				result = true;
			}
		}
	}
	return result;
}

//

void ModulePilgrim::get_physical_violence_hand_speeds(Hand::Type _hand, OUT_ float& _minSpeed, OUT_ float& _minSpeedStrong) const
{
	float minSpeed = minPhysicalViolenceHandSpeed;
	float minSpeedStrong = minPhysicalViolenceHandSpeedStrong;
	PilgrimBlackboard::update_physical_violence_speeds(get_owner(), _hand, REF_ minSpeed, REF_ minSpeedStrong);
	_minSpeed = minSpeed;
	_minSpeedStrong = minSpeedStrong;
}

Energy ModulePilgrim::get_physical_violence_damage(Hand::Type _hand, bool _strong) const
{
	if (TutorialSystem::check_active() && ! TutorialSystem::get()->is_physical_violence_allowed())
	{
		return Energy::zero();
	}
	Energy damage = _strong ? physicalViolenceDamageStrong : physicalViolenceDamage;
	PilgrimBlackboard::update_physical_violence_damage(get_owner(), _hand, _strong, REF_ damage);
	return damage;
}

void ModulePilgrim::setup_scene_build_context_for_tutorial_highlights(Framework::Render::SceneBuildContext& _context)
{
	if (ScreenShotContext::get_active() || weaponsBlocked)
	{
		// don't render highlights on screenshots
		return;
	}
	if (TutorialSystem::check_highlighted(NAME(tutBodyPocket)))
	{
		todo_important(TXT("we're most likely not going to use it"));
		/*
		int pocketPartsAsBits = 0;
		for_every(pocket, pockets)
		{
			if (pocket->materialIndex.is_set())
			{
				pocketPartsAsBits |= bit(pocket->materialIndex.get());
			}
		}
		if (pocketPartsAsBits)
		{
			if (auto* mesh = get_owner()->get_appearance()->get_mesh())
			{
				_context.set_override_material_render_of(get_owner(), &TutorialSystem::get()->get_highlight_material_instance(mesh->get_mesh()->get_usage()), pocketPartsAsBits);
			}
		}
		*/
	}
	if (TutorialSystem::check_highlighted(NAME(tutBodyWeapons)))
	{
		for_count(int, handIdx, Hand::MAX)
		{
			if (auto* weaponObject = get_main_equipment((Hand::Type)handIdx))
			{
				if (auto* mesh = weaponObject->get_appearance()->get_mesh())
				{
					_context.set_override_material_render_of(weaponObject, &TutorialSystem::get()->get_highlight_material_instance(mesh->get_mesh()->get_usage()));
				}
			}
		}
	}
}

void ModulePilgrim::ex__block_releasing_equipment(Hand::Type _hand, bool _block)
{
	an_assert(_hand == Hand::Left || _hand == Hand::Right);

	handStates[_hand].exBlockReleasingEquipment = _block;
	handStates[_hand].blockReleasingEquipment = true;
	handStates[_hand].blockReleasingEquipmentTimeLeft = BLOCK_RELEASING_TIME;
}

void ModulePilgrim::ex__force_release(Hand::Type _hand)
{
	an_assert(_hand == Hand::Left || _hand == Hand::Right);

	handStates[_hand].exForceRelease = true;
}

void ModulePilgrim::ex__drop_energy_by(Optional<Hand::Type> const& _hand, Energy const& _dropBy)
{
	Hand::Type hand = _hand.get((Hand::Type)Random::get_int(Hand::MAX));
	hand = GameSettings::any_use_hand(hand);
	set_hand_energy_storage(hand, max(Energy::zero(), get_hand_energy_storage(hand) - _dropBy));
}

void ModulePilgrim::ex__set_energy(Optional<Hand::Type> const& _hand, Energy const& _setTo)
{
	Hand::Type hand = _hand.get((Hand::Type)Random::get_int(Hand::MAX));
	if (GameSettings::get().difficulty.commonHandEnergyStorage)
	{
		hand = Hand::Left;
	}
	Energy setTo = _setTo;
	if (auto* gd = GameDirector::get())
	{
		if (gd->are_ammo_storages_unavailable())
		{
			setTo = Energy::zero();
		}
	}
	set_hand_energy_storage(hand, clamp(setTo, Energy::zero(), get_hand_energy_max_storage(hand)));
}

Vector3 ModulePilgrim::get_forearm_location_os(Hand::Type _hand) const
{
	auto* appearance = get_owner()->get_appearance();
	if (!appearance)
	{
		return Vector3::zero;
	}

	auto const& forearm = forearms[_hand];
	Vector3 loc = Vector3::zero;
	float weight = 0.0f;
	if (forearm.forearmStartSocket.is_valid())
	{
		loc += appearance->calculate_socket_os(forearm.forearmStartSocket.get_index()).get_translation();
		weight += 1.0f;
	}
	if (forearm.forearmEndSocket.is_valid())
	{
		loc += appearance->calculate_socket_os(forearm.forearmEndSocket.get_index()).get_translation();
		weight += 1.0f;
	}
	if (weight > 0.0f)
	{
		return loc / weight;
	}
	else
	{
		return Vector3::zero;
	}
}

bool ModulePilgrim::does_have_any_weapon_parts_to_transfer() const
{
	error(TXT("do not use it?"));

	return false;
}

void ModulePilgrim::update_pilgrim_inventory_for_inventory()
{
	{
		Concurrency::ScopedSpinLock lock(pilgrimInventory.weaponPartsLock);

		// scan through weapon parts to see if they belong to that list. if not, remove them.
		// this only affects weapon parts that are based on items
		for (int i = 0; i < pilgrimInventory.weaponParts.get_size(); ++i)
		{
			auto const& piwp = pilgrimInventory.weaponParts[i];
			if (piwp.source == PilgrimInventory::Source::HeldItem)
			{
				pilgrimInventory.weaponParts.remove_at(i);
				--i;
			}
		}
	}
}

void ModulePilgrim::update_pilgrim_inventory_for_transfer()
{
	ARRAY_STACK(ModuleEquipments::Gun*, guns, 32);

	get_all_guns(guns);

	struct WeaponPartFromItem
	{
		WeaponPart* weaponPart;
		Framework::IModulesOwner* item;
		WeaponPartAddress at;
	};
	ARRAY_STACK(WeaponPartFromItem, weaponParts, 256);

	// change that into weapon parts list
	for_every_ptr(gun, guns)
	{
		Framework::IModulesOwner* gunOwner = gun->get_owner();
		for_every(part, gun->get_weapon_setup().get_parts())
		{
			if (auto* wp = part->part.get())
			{
				WeaponPartFromItem wpfi;
				wpfi.item = gunOwner;
				wpfi.at = part->at;
				wpfi.weaponPart = wp;
				weaponParts.push_back(wpfi);
			}
		}
	}

	{
		Concurrency::ScopedSpinLock lock(pilgrimInventory.weaponPartsLock);

		// scan through weapon parts to see if they belong to that list. if not, remove them.
		// this only affects weapon parts that are based on items
		for (int i = 0; i < pilgrimInventory.weaponParts.get_size(); ++ i)
		{
			auto const& piwp = pilgrimInventory.weaponParts[i];
			if (piwp.source == PilgrimInventory::Source::HeldItem)
			{
				bool exists = false;
				for_every(iwp, weaponParts)
				{
					if (piwp.weaponPart == iwp->weaponPart &&
						piwp.item == iwp->item &&
						piwp.address == iwp->at)
					{
						exists = true;
						break;
					}
				}
				if (!exists)
				{
					pilgrimInventory.weaponParts.remove_at(i);
					--i;
				}
			}
		}

		// scan the items and add weapon parts to inventory, mark them as "held items"
		for_every(wp, weaponParts)
		{
			pilgrimInventory.add_weapon_part_from_item(wp->weaponPart, wp->item, wp->at);
		}
	}
}

void ModulePilgrim::transfer_out_weapon_part(PilgrimInventory::ID _weaponPartID)
{
	// this requires pilgrim inventory to be updated with update_pilgrim_inventory_for_transfer()
	// we get info if we're in an item or not, that's enough for us to make it work
	PilgrimInventory::PlacedPart placedPart = pilgrimInventory.get_placed_weapon_part(_weaponPartID);
	if (placedPart.weaponPart.is_set())
	{
		if (placedPart.source == PilgrimInventory::HeldItem)
		{
			ARRAY_STACK(ModuleEquipments::Gun*, guns, 32);

			get_all_guns(guns);

			for_every_ptr(gun, guns)
			{
				if (placedPart.item == gun->get_owner())
				{
					// may result in disassembling the gun
					gun->remove_weapon_part_to_pilgrim_inventory(placedPart.address);
				}
			}
		}

		pilgrimInventory.remove_weapon_part(_weaponPartID);
	}
}

void ModulePilgrim::get_all_items(ArrayStack<Framework::IModulesOwner*>& _into) const
{
	MODULE_OWNER_LOCK_FOR(this, TXT("ModulePilgrim::get_all_items"));

	// get a list of items we can consider them as a part of the inventory
	for_count(int, hIdx, Hand::MAX)
	{
		if (auto* imo = get_hand_equipment((Hand::Type)hIdx))
		{
			if (!is_main_equipment(imo))
			{
				_into.push_back(imo);
			}
		}
		if (auto* imo = get_main_equipment((Hand::Type)hIdx))
		{
			_into.push_back(imo);
		}
	}
	for_every(pocket, pockets)
	{
		if (auto* imo = pocket->get_in_pocket())
		{
			if (!is_main_equipment(imo))
			{
				_into.push_back(imo);
			}
		}
	}
}

void ModulePilgrim::get_all_guns(ArrayStack<ModuleEquipments::Gun*>& _into) const
{
	ARRAY_STACK(Framework::IModulesOwner*, items, 32);
	get_all_items(items);
	for_every_ptr(item, items)
	{
		if (auto* gun = item->get_gameplay_as<ModuleEquipments::Gun>())
		{
			_into.push_back(gun);
		}
	}
}

bool ModulePilgrim::is_in_pocket(Framework::IModulesOwner const* _imo) const
{
	for_every(pocket, pockets)
	{
		if (pocket->get_in_pocket() == _imo)
		{
			return true;
		}
	}
	return false;
}

int ModulePilgrim::get_in_pocket_index(Framework::IModulesOwner const* _imo) const
{
	for_every(pocket, pockets)
	{
		if (pocket->get_in_pocket() == _imo)
		{
			return for_everys_index(pocket);
		}
	}
	return NONE;
}

#ifdef WITH_CRYSTALITE
void ModulePilgrim::add_crystalite(int _amount)
{
	MODULE_OWNER_LOCK(TXT("ModulePilgrim::add_crystalite"));

	crystalite += _amount;
}
#endif

bool ModulePilgrim::is_something_moving() const
{
	for_count(int, i, Hand::MAX)
	{
		auto& handState = handStates[i];
		if ((handState.port.state > 0.0f && handState.port.state < 1.0f) ||
			(handState.displayNormal.state > 0.0f && handState.displayNormal.state < 1.0f) ||
			(handState.displayExtra.state > 0.0f && handState.displayExtra.state < 1.0f) ||
			(handState.restPoint.state > 0.0f && handState.restPoint.state < 1.0f) ||
			(handState.restPointEmpty.state > 0.0f && handState.restPointEmpty.state < 1.0f))
		{
			// if between 0 and 1, most likely moving
			return true;
		}
	}
	return false;
}

bool ModulePilgrim::is_actively_holding_in_hand(Hand::Type _type) const
{
	auto& hs = handStates[_type];
	if (hs.currentState == HandState::HoldingObject &&
		!hs.blockReleasingEquipment)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ModulePilgrim::log(LogInfoContext& _log)
{
	overlayInfo.log(_log);
}

Transform ModulePilgrim::get_hand_relative_placement_os(Hand::Type _type, bool _centre) const
{
	if (auto* h = get_hand(_type))
	{
		Transform handWS = _centre? h->get_presence()->get_centre_of_presence_transform_WS() : h->get_presence()->get_placement();
		handWS = h->get_presence()->get_path_to_attached_to()->from_owner_to_target(handWS);
		return get_owner()->get_presence()->get_placement().to_local(handWS);
	}
	return Transform::identity;
}

void ModulePilgrim::add_gear_to_persistence(bool _allowDuplicates)
{
	ArrayStatic<Framework::IModulesOwner*, 16> gear; SET_EXTRA_DEBUG_INFO(gear, TXT("ModulePilgrim::add_gear_to_persistence.gear"));
	for_count(int, hIdx, Hand::MAX)
	{
		Hand::Type hand = (Hand::Type)hIdx;
		if (auto* imo = get_main_equipment(hand))
		{
			gear.push_back_unique(imo);
		}
		if (auto* imo = get_hand_equipment(hand))
		{
			gear.push_back_unique(imo);
		}
	}

	get_in_pockets(gear, NP, true);

	{
		auto& p = Persistence::access_current();
		Concurrency::ScopedSpinLock lock(p.access_lock());

		for_every_ptr(g, gear)
		{
			if (auto* gun = g->get_gameplay_as< ModuleEquipments::Gun>())
			{
				p.add_weapon_to_armoury(gun->get_weapon_setup(), _allowDuplicates);
			}
		}
	}
}

//--

void ModulePilgrim::EXMs::cease_to_exists_and_clear()
{
#ifdef INSPECT_DESTROYING_EXMS
	output(TXT("destroying exms"));
#endif
	for_every(pEXM, passiveEXMs)
	{
		if (auto* exm = pEXM->imo.get())
		{
			exm->be_autonomous();
			exm->cease_to_exist(true);
		}
	}
	if (auto* exm = activeEXM.imo.get())
	{
		exm->be_autonomous();
		exm->cease_to_exist(true);
	}
	for_every(e, permanentEXMs)
	{
		if (auto* exm = e->imo.get())
		{
			exm->be_autonomous();
			exm->cease_to_exist(true);
		}
	}
	permanentEXMs.clear();
	passiveEXMs.clear();
	activeEXM.clear();
#ifdef INSPECT_DESTROYING_EXMS
	output(TXT("destroyed exms"));
#endif
}

void ModulePilgrim::EXMs::create(Framework::Actor* _owner, Hand::Type _hand, bool _createHandSetup, PilgrimHandSetup const& _handSetup, PilgrimInventory const * _inventoryForPermanent)
{
	ASSERT_SYNC_OR_ASYNC;

	auto* appearance = _owner->get_appearance();
	if (!appearance)
	{
		return;
	}

#ifdef INSPECT_EXMS
	{
		output(TXT("[pilgrim] recreate EXMs for %S hand"), Hand::to_char(_hand));
		{
			output(TXT(" current"));
			output(TXT("    %c active %S"), activeEXM.imo.get()? 'V' : '-', activeEXM.exm.to_char());
			for_every(exm, passiveEXMs)
			{
				output(TXT("  %i %c passive %S"), for_everys_index(exm), exm->imo.get() ? 'V' : '-', exm->exm.to_char());
			}
		}
		{
			output(TXT(" requested"));
			if (_createHandSetup)
			{
				output(TXT("    active %S"), _handSetup.activeEXM.exm.to_char());
				for_every(exm, _handSetup.passiveEXMs)
				{
					output(TXT("  %i passive %S"), for_everys_index(exm), exm->exm.to_char());
				}
			}
		}
	}
#endif

	struct CreateEXM
	{
		EXM* exmPtr = nullptr;
		Name exm;

		CreateEXM(EXM* _exmPtr = nullptr, PilgrimHandEXMSetup const * _exmSetup = nullptr) : exmPtr(_exmPtr), exm(_exmSetup && _exmSetup->is_set()? _exmSetup->exm : Name::invalid()) {}
		CreateEXM(EXM* _exmPtr, Name const & _exm) : exmPtr(_exmPtr), exm(_exm) {}
	};
	ArrayStatic<CreateEXM, 32> createEXMs; SET_EXTRA_DEBUG_INFO(createEXMs, TXT("ModulePilgrim::EXMs::create.createEXMs"));

	PilgrimHandEXMSetup emptyEXM;
	if (_createHandSetup)
	{
		if (passiveEXMs.get_size() < _handSetup.passiveEXMs.get_size())
		{
			passiveEXMs.set_size(_handSetup.passiveEXMs.get_size());
		}
		for_every(pEXM, passiveEXMs)
		{
			int i = for_everys_index(pEXM);
			createEXMs.push_back(CreateEXM(pEXM, _handSetup.passiveEXMs.is_index_valid(i)? &_handSetup.passiveEXMs[i] : &emptyEXM));
		}
		createEXMs.push_back(CreateEXM(&activeEXM, &_handSetup.activeEXM));
	}

	if (_inventoryForPermanent)
	{
		Concurrency::ScopedMRSWLockRead lock(_inventoryForPermanent->exmsLock);
		while (permanentEXMs.get_size() > _inventoryForPermanent->permanentEXMs.get_size())
		{
			auto& lexm = permanentEXMs.get_last();
			if (lexm.imo.is_set())
			{
				lexm.imo->be_autonomous();
				lexm.imo->cease_to_exist(true);
			}
			lexm.clear();
			permanentEXMs.pop_back();
		}
		permanentEXMs.set_size(_inventoryForPermanent->permanentEXMs.get_size());
		for_every(exm, _inventoryForPermanent->permanentEXMs)
		{
			int i = for_everys_index(exm);
			createEXMs.push_back(CreateEXM(&permanentEXMs[i], *exm));
		}
	}

	for_every(c, createEXMs)
	{
		// destroy if differs or should not exist
		if (c->exmPtr->imo.is_set() && // has to exist
			(!c->exm.is_valid() || c->exmPtr->exm != c->exm)) // shouldn't exist or is going to be replaced
		{
			if (auto* imo = c->exmPtr->imo.get())
			{
#ifdef INSPECT_EXMS
				output(TXT("[pilgrim] destroy exm %S"), c->exmPtr->exm.to_char());
#endif
				imo->be_autonomous();
				imo->cease_to_exist(true);
				c->exmPtr->clear();
			}
		}
		// create only if doesn't exist and should - note that if it differs, it should be destroyed above
		if (c->exm.is_valid() && // should exist
			!c->exmPtr->imo.is_set()) // but doesn't right now
		{
			an_assert(!c->exmPtr->imo.is_set());
			if (auto* exmIMO = create_internal(_owner, _hand, c->exm))
			{
#ifdef INSPECT_EXMS
				output(TXT("[pilgrim] create exm %S"), c->exm.to_char());
#endif
				if (auto* mp = _owner->get_gameplay_as<ModulePilgrim>())
				{
					mp->access_pilgrim_inventory().make_exm_available(c->exm);
				}
				c->exmPtr->imo = exmIMO;
				c->exmPtr->exm = c->exm;

				if (c->exmPtr == &activeEXM)
				{
					if (auto* mexm = exmIMO->get_gameplay_as<ModuleEXM>())
					{
						mexm->mark_exm_selected(true);
					}
				}
			}
		}
	}

#ifdef INSPECT_EXMS
	{
		output(TXT("[pilgrim] recreated EXMs for %S hand"), Hand::to_char(_hand));
		{
			output(TXT(" current"));
			output(TXT("    %c active %S"), activeEXM.imo.get() ? 'V' : '-', activeEXM.exm.to_char());
			for_every(exm, passiveEXMs)
			{
				output(TXT("  %i %c passive %S"), for_everys_index(exm), exm->imo.get() ? 'V' : '-', exm->exm.to_char());
			}
		}
	}
#endif
}

Framework::IModulesOwner* ModulePilgrim::EXMs::create_internal(Framework::Actor* _owner, Hand::Type _hand, Name const & _exm)
{
	if (!_owner || !_exm.is_valid())
	{
		return nullptr;
	}

	if (EXMType const* exmType = EXMType::find(_exm))
	{
		if (auto* exm = exmType->create_actor(_owner, _hand))
		{
			auto* exmAppearance = exm->get_appearance();
			bool keepEXM = false;
			if (exmAppearance)
			{
				todo_implement(TXT("we no longer support exms with an appearance"));
			}
			else if (auto* mp = _owner->get_gameplay_as<ModulePilgrim>())
			{
				if (auto* handIMO = mp->get_hand(_hand))
				{
					// attach to hand!
					Game::get_as<Game>()->perform_sync_world_job(TXT("attach exm"), [exm, handIMO]()
					{
						exm->get_presence()->attach_to(handIMO, false, handIMO->get_presence()->get_centre_of_presence_os(), false);
					});

					keepEXM = true;
				}
			}
			if (keepEXM)
			{
				return exm;
			}
			else
			{
				exm->cease_to_exist(true);
				return nullptr;
			}
		}
	}
	return nullptr;
}
