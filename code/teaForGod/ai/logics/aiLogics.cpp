#include "aiLogics.h"

#include "..\..\..\framework\ai\aiLogics.h"

#include "aiLogic_a_blob.h"
#include "aiLogic_a_tentacle.h"
#include "aiLogic_a_tentacledShooter.h"
#include "aiLogic_airfighter.h"
#include "aiLogic_airfighter_v2.h"
#include "aiLogic_armoury.h"
#include "aiLogic_automap.h"
#include "aiLogic_barker.h"
#include "aiLogic_barrel.h"
#include "aiLogic_battery.h"
#include "aiLogic_bigTrainEngineCylinder.h"
#include "aiLogic_blobCapsule.h"
#include "aiLogic_bottle.h"
#include "aiLogic_bridgeShop.h"
#include "aiLogic_catcher.h"
#include "aiLogic_centipede.h"
#include "aiLogic_charger.h"
#include "aiLogic_cleanser.h"
#include "aiLogic_controlPanel.h"
#include "aiLogic_coreShaftDischarges.h"
#include "aiLogic_creeper.h"
#include "aiLogic_discharger.h"
#include "aiLogic_displayMessage.h"
#include "aiLogic_doorDevice.h"
#include "aiLogic_doorbot.h"
#include "aiLogic_doorLock.h"
#include "aiLogic_dropCapsule.h"
#include "aiLogic_dropCapsuleControl.h"
#include "aiLogic_elevatorCart.h"
#include "aiLogic_elevatorCartPoint.h"
#include "aiLogic_energyBalancer.h"
#include "aiLogic_energyCoil.h"
#include "aiLogic_energyCoilBroken.h"
#include "aiLogic_energyKiosk.h"
#include "aiLogic_energyQuantumsOnPath.h"
#include "aiLogic_enforcer.h"
#include "aiLogic_engineGenerator.h"
#include "aiLogic_exmInstallerKiosk.h"
#include "aiLogic_factoryArm.h"
#include "aiLogic_findableBox.h"
#include "aiLogic_fender.h"
#include "aiLogic_follower.h"
#include "aiLogic_gameScriptTrapTrigger.h"
#include "aiLogic_h_craft.h"
#include "aiLogic_h_craft_v2.h"
#include "aiLogic_hackBox.h"
#include "aiLogic_heartRoom.h"
#include "aiLogic_heartRoomBeams.h"
#include "aiLogic_heartRoomProjectiles.h"
#include "aiLogic_inspecter.h"
#include "aiLogic_item.h"
#include "aiLogic_launchChamber.h"
#include "aiLogic_lockerBox.h"
#include "aiLogic_miniGameMachine.h"
#include "aiLogic_missionBriefer.h"
#include "aiLogic_monorailCart.h"
#include "aiLogic_muezzin.h"
#include "aiLogic_none.h"
#include "aiLogic_overseer.h"
#include "aiLogic_pester.h"
#include "aiLogic_permanentUpgradeMachine.h"
#include "aiLogic_pilgrim.h"
#include "aiLogic_pilgrimFake.h"
#include "aiLogic_pilgrimHand.h"
#include "aiLogic_pilgrimHumanForearmDisplay.h"
#include "aiLogic_pilgrimTank.h"
#include "aiLogic_pilgrimTeaBox.h"
#include "aiLogic_plant.h"
#include "aiLogic_portableShield.h"
#include "aiLogic_puzzlePanel.h"
#include "aiLogic_rammer.h"
#include "aiLogic_reactorEnergyOverloaded.h"
#include "aiLogic_reactorEnergyQuantum.h"
#include "aiLogic_reactorEnergyWorking.h"
#include "aiLogic_refillBox.h"
#include "aiLogic_scourer.h"
#include "aiLogic_scriptablePanel.h"
#include "aiLogic_spitter.h"
#include "aiLogic_steamer.h"
#include "aiLogic_stinger.h"
#include "aiLogic_subtitleDevice.h"
#include "aiLogic_switchPanel.h"
#include "aiLogic_throneTree.h"
#include "aiLogic_trashBin.h"
#include "aiLogic_trigger.h"
#include "aiLogic_tunnelLight.h"
#include "aiLogic_turret.h"
#include "aiLogic_turret360.h"
#include "aiLogic_turretScripted.h"
#include "aiLogic_tutorialTarget.h"
#include "aiLogic_unloader.h"
#include "aiLogic_upgradeMachine.h"
#include "aiLogic_waiter.h"
#include "aiLogic_weaponMixer.h"
#include "aiLogic_welder.h"

#include "exms\aiLogic_exmAttractor.h"
#include "exms\aiLogic_exmBlast.h"
#include "exms\aiLogic_exmCorrosionCatalyst.h"
#include "exms\aiLogic_exmDeflector.h"
#include "exms\aiLogic_exmDistractor.h"
#include "exms\aiLogic_exmElevatorMaster.h"
#include "exms\aiLogic_exmEnergyDispatcher.h"
#include "exms\aiLogic_exmEnergyManipulator.h"
#include "exms\aiLogic_exmInvestigator.h"
#include "exms\aiLogic_exmProjectile.h"
#include "exms\aiLogic_exmPush.h"
#include "exms\aiLogic_exmRemoteControl.h"
#include "exms\aiLogic_exmReroll.h"
#include "exms\aiLogic_exmShield.h"
#include "exms\aiLogic_exmSpray.h"

#include "managers\aiManagerLogic_airPursuitManager.h"
#include "managers\aiManagerLogic_airshipsManager.h"
#include "managers\aiManagerLogic_airshipProxiesManager.h"
#include "managers\aiManagerLogic_backgroundMover.h"
#include "managers\aiManagerLogic_bouncingCharge.h"
#include "managers\aiManagerLogic_dischargeZone.h"
#include "managers\aiManagerLogic_doorFalling.h"
#include "managers\aiManagerLogic_infestationManager.h"
#include "managers\aiManagerLogic_monorailManager.h"
#include "managers\aiManagerLogic_regionManager.h"
#include "managers\aiManagerLogic_roomWithEnergySource.h"
#include "managers\aiManagerLogic_shooterWavesManager.h"
#include "managers\aiManagerLogic_spawnManager.h"

#include "tasks\aiLogicTask_attackOrIdle.h"
#include "tasks\aiLogicTask_evadeAiming.h"
#include "tasks\aiLogicTask_grabEnergy.h"
#include "tasks\aiLogicTask_idle.h"
#include "tasks\aiLogicTask_lookAt.h"
#include "tasks\aiLogicTask_perception.h"
#include "tasks\aiLogicTask_projectileHit.h"
#include "tasks\aiLogicTask_scan.h"
#include "tasks\aiLogicTask_scanAndWander.h"
#include "tasks\aiLogicTask_shield.h"
#include "tasks\aiLogicTask_shoot.h"
#include "tasks\aiLogicTask_stayClose.h"
#include "tasks\aiLogicTask_stayCloseInFront.h"
#include "tasks\aiLogicTask_stayCloseOrWander.h"
#include "tasks\aiLogicTask_wander.h"
#include "tasks\aiLogicTask_wander.h"

#ifdef AN_CLANG
#include "..\..\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

void All::register_static()
{
	Framework::AI::Logics::register_ai_logic(TXT("a_blob"), A_Blob::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("a_tentacle"), A_Tentacle::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("a_tentacled shooter"), A_TentacledShooter::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("airfighter"), Airfighter::create_ai_logic, AirfighterData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("airfighter v2"), Airfighter_V2::create_ai_logic, Airfighter_V2Data::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("armoury"), Armoury::create_ai_logic, ArmouryData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("automap"), Automap::create_ai_logic, AutomapData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("barker"), Barker::create_ai_logic, BarkerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("barrel"), Barrel::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("battery"), Battery::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("big train engine cylinder"), BigTrainEngineCylinder::create_ai_logic, BigTrainEngineCylinderData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("blob capsule"), BlobCapsule::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("bottle"), Bottle::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("bridge shop"), BridgeShop::create_ai_logic, BridgeShopData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("catcher"), Catcher::create_ai_logic, CatcherData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("centipede"), Centipede::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("charger"), Charger::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("cleanser"), Cleanser::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("control panel"), ControlPanel::create_ai_logic, ControlPanelData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("core shaft discharges"), CoreShaftDischarges::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("creeper"), Creeper::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("discharger"), Discharger::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("display message"), DisplayMessage::create_ai_logic, DisplayMessageData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("doorbot"), Doorbot::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("door device"), DoorDevice::create_ai_logic, DoorDeviceData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("door lock"), DoorLock::create_ai_logic, DoorLockData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("drop capsule"), DropCapsule::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("drop capsule control"), DropCapsuleControl::create_ai_logic, DropCapsuleControlData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("elevator cart"), ElevatorCart::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("elevator cart point"), ElevatorCartPoint::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("energy balancer"), EnergyBalancer::create_ai_logic, EnergyBalancerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("energy coil"), EnergyCoil::create_ai_logic, EnergyCoilData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("energy coil broken"), EnergyCoilBroken::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("energy kiosk"), EnergyKiosk::create_ai_logic, EnergyKioskData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("energy quantums on path"), EnergyQuantumsOnPath::create_ai_logic, EnergyQuantumsOnPathData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("enforcer"), Enforcer::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("engine generator"), EngineGenerator::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm installer kiosk"), EXMInstallerKiosk::create_ai_logic, EXMInstallerKioskData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("factory arm"), FactoryArm::create_ai_logic, FactoryArmData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("findable box"), FindableBox::create_ai_logic, FindableBoxData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("fender"), Fender::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("follower"), Follower::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("game script trap trigger"), GameScriptTrapTrigger::create_ai_logic, GameScriptTrapTriggerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("h_craft"), H_Craft::create_ai_logic, H_CraftData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("h_craft v2"), H_Craft_V2::create_ai_logic, H_Craft_V2Data::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("hack box"), HackBox::create_ai_logic, HackBoxData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("heart room"), HeartRoom::create_ai_logic, HeartRoomData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("heart room beams"), HeartRoomBeams::create_ai_logic, HeartRoomBeamsData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("heart room projectiles"), HeartRoomProjectiles::create_ai_logic, HeartRoomProjectilesData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("inspecter"), Inspecter::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("item"), Item::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("launch chamber"), LaunchChamber::create_ai_logic, LaunchChamberData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("locker box"), LockerBox::create_ai_logic, LockerBoxData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("monorail cart"), MonorailCart::create_ai_logic);
#ifdef AN_MINIGAME_PLATFORMER
	Framework::AI::Logics::register_ai_logic(TXT("mini game machine"), MiniGameMachine::create_ai_logic, MiniGameMachineData::create_ai_logic_data);
#endif
	Framework::AI::Logics::register_ai_logic(TXT("mission briefer"), MissionBriefer::create_ai_logic, MissionBrieferData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("muezzin"), Muezzin::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("none"), None::create_ai_logic, NoneData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("overseer"), Overseer::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("pester"), Pester::create_ai_logic, PesterData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("permanent upgrade machine"), PermanentUpgradeMachine::create_ai_logic, PermanentUpgradeMachineData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("pilgrim"), Pilgrim::create_ai_logic, PilgrimData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("pilgrim fake"), PilgrimFake::create_ai_logic, PilgrimFakeData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("pilgrim hand"), PilgrimHand::create_ai_logic, PilgrimHandData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("pilgrim human forearm display"), PilgrimHumanForearmDisplay::create_ai_logic, PilgrimHumanForearmDisplayData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("pilgrim tank"), PilgrimTank::create_ai_logic, PilgrimTankData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("pilgrim tea box"), PilgrimTeaBox::create_ai_logic, PilgrimTeaBoxData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("plant"), Plant::create_ai_logic, PlantData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("portable shield"), PortableShield::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("puzzle panel"), PuzzlePanel::create_ai_logic, PuzzlePanelData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("rammer"), Rammer::create_ai_logic, RammerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("reactor energy overloaded"), ReactorEnergyOverloaded::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("reactor energy quantum"), ReactorEnergyQuantum::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("reactor energy working"), ReactorEnergyWorking::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("refill box"), RefillBox::create_ai_logic, RefillBoxData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("scourer"), Scourer::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("scriptable panel"), ScriptablePanel::create_ai_logic, ScriptablePanelData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("spitter"), Spitter::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("steamer"), Steamer::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("stinger"), Stinger::create_ai_logic, StingerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("subtitle device"), SubtitleDevice::create_ai_logic, SubtitleDeviceData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("switch panel"), SwitchPanel::create_ai_logic, SwitchPanelData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("trash bin"), TrashBin::create_ai_logic, TrashBinData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("throne tree"), ThroneTree::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("trigger"), Trigger::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("tunnel light"), TunnelLight::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("turret"), Turret::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("turret 360"), Turret360::create_ai_logic, Turret360Data::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("turret scripted"), TurretScripted::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("tutorial target"), TutorialTarget::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("unloader"), Unloader::create_ai_logic, UnloaderData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("upgrade machine"), UpgradeMachine::create_ai_logic, UpgradeMachineData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("waiter"), Waiter::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("weapon mixer"), WeaponMixer::create_ai_logic, WeaponMixerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("welder"), Welder::create_ai_logic);

	// exms
	Framework::AI::Logics::register_ai_logic(TXT("exm attractor"), EXMAttractor::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm blast"), EXMBlast::create_ai_logic, EXMBlastData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("exm corrosion catalyst"), EXMCorrosionCatalyst::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm deflector"), EXMDeflector::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm distractor"), EXMDistractor::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm elevator master"), EXMElevatorMaster::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm energy dispatcher"), EXMEnergyDispatcher::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm energy manipulator"), EXMEnergyManipulator::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm investigator"), EXMInvestigator::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm projectile"), EXMProjectile::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm push"), EXMPush::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm remote control"), EXMRemoteControl::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm reroll"), EXMReroll::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm shield"), EXMShield::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("exm spray"), EXMSpray::create_ai_logic);

	// managers
	// these are logics as well but are higher level (and are using fake actors/sceneries)
	// can be spawn manager, choreographer, puzzle manager etc
	Framework::AI::Logics::register_ai_logic(TXT("air pursuit manager"), Managers::AirPursuitManager::create_ai_logic, Managers::AirPursuitManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("airships manager"), Managers::AirshipsManager::create_ai_logic, Managers::AirshipsManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("airship proxies manager"), Managers::AirshipProxiesManager::create_ai_logic, Managers::AirshipProxiesManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("background mover"), Managers::BackgroundMover::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("bouncing charge manager"), Managers::BouncingChargeManager::create_ai_logic, Managers::BouncingChargeManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("discharge zone manager"), Managers::DischargeZoneManager::create_ai_logic, Managers::DischargeZoneManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("door falling manager"), Managers::DoorFallingManager::create_ai_logic);
	Framework::AI::Logics::register_ai_logic(TXT("monorail manager"), Managers::MonorailManager::create_ai_logic, Managers::MonorailManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("infestation manager"), Managers::InfestationManager::create_ai_logic, Managers::InfestationManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("region manager"), Managers::RegionManager::create_ai_logic, Managers::RegionManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("room with energy source"), Managers::RoomWithEnergySource::create_ai_logic, Managers::RoomWithEnergySourceData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("shooter waves manager"), Managers::ShooterWavesManager::create_ai_logic, Managers::ShooterWavesManagerData::create_ai_logic_data);
	Framework::AI::Logics::register_ai_logic(TXT("spawn manager"), Managers::SpawnManager::create_ai_logic, Managers::SpawnManagerData::create_ai_logic_data);

	// latent tasks
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("attack or idle"), Tasks::attack_or_idle);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("evade aiming 3d"), Tasks::evade_aiming_3d);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("grab energy"), Tasks::grab_energy);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("idle"), Tasks::idle);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("look at"), Tasks::look_at);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("perception"), Tasks::perception);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("avoid projectile hit 3d"), Tasks::avoid_projectile_hit_3d);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("scan"), Tasks::scan);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("scan and wander"), Tasks::scan_and_wander);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("shield"), Tasks::shield);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("shoot"), Tasks::shoot);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("stay close "), Tasks::stay_close);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("stay close in front"), Tasks::stay_close_in_front);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("stay close or wander"), Tasks::stay_close_or_wander);
	Framework::AI::Logics::register_ai_logic_latent_task(TXT("wander"), Tasks::wander);

	// although it is better to do this through latent logic
	/**
			task name				description
			------------------------------------------------------------------------------------------------------
			attackSequence			attacking enemy, going to enemy and attacking
			idleSequence			nothing to do, idles, wanders etc
			stayCloseSequence		stay close to specific imo
			wanderSequence			wander around
	 */
}

