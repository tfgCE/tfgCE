#include "modules.h"

#include "moduleCollisionHeaded.h"
#include "moduleCollisionProjectile.h"
#include "moduleGameplayPreview.h"
#include "moduleGameplayProjectile.h"
#include "moduleMovementDial.h"
#include "moduleMovementFastColliding.h"
#include "moduleMovementFastCollidingProjectile.h"
#include "moduleMovementHover.h"
#include "moduleMovementHoverOld.h"
#include "moduleMovementOnFloor.h"
#include "moduleMovementPath.h"
#include "moduleMovementProjectile.h"
#include "moduleMovementScripted.h"
#include "moduleMovementSwitch.h"
#include "moduleNavElementNode.h"

#include "custom\mc_display.h"
#include "custom\mc_lcdLetters.h"
#include "custom\mc_lcdLights.h"
#include "custom\mc_lightningSpawner.h"
#include "custom\mc_lightSources.h"
#include "custom\mc_temporaryObjectSpawner.h"
#include "custom\mc_vrPlacementsProvider.h"

#include "registeredModule.h"

#include "..\..\core\types\names.h"

using namespace Framework;

RegisteredModuleSet<ModuleAI, ModuleAIData> Modules::ai;
RegisteredModuleSet<ModuleAppearance, ModuleAppearanceData> Modules::appearance;
RegisteredModuleSet<ModuleCollision, ModuleCollisionData> Modules::collision;
RegisteredModuleSet<ModuleController, ModuleData> Modules::controller;
RegisteredModuleSet<ModuleCustom, ModuleData> Modules::custom;
RegisteredModuleSet<ModuleGameplay, ModuleData> Modules::gameplay;
RegisteredModuleSet<ModuleMovement, ModuleMovementData> Modules::movement;
RegisteredModuleSet<ModuleNavElement, ModuleData> Modules::navElement;
RegisteredModuleSet<ModulePresence, ModulePresenceData> Modules::presence;
RegisteredModuleSet<ModuleSound, ModuleSoundData> Modules::sound;
RegisteredModuleSet<ModuleTemporaryObjects, ModuleTemporaryObjectsData> Modules::temporaryObjects;

void Modules::initialise_static()
{
	// ai
	ModuleAI::register_itself().be_default();

	// appearance
	ModuleAppearance::register_itself().be_default();

	// collision
	ModuleCollision::register_itself().be_default();
	ModuleCollisionHeaded::register_itself();
	ModuleCollisionProjectile::register_itself();

	// controller
	ModuleController::register_itself().be_default();

	// custom
	CustomModules::Display::register_itself();
	CustomModules::LCDLetters::register_itself();
	CustomModules::LCDLights::register_itself();
	CustomModules::LightningSpawner::register_itself();
	CustomModules::LightSources::register_itself();
	CustomModules::TemporaryObjectSpawner::register_itself();
	CustomModules::VRPlacementsProvider::register_itself();

	// gameplay
	ModuleGameplayPreview::register_itself();
	ModuleGameplayProjectile::register_itself();

	// movement
	ModuleMovement::register_itself().be_default();
	ModuleMovementDial::register_itself();
	ModuleMovementFastColliding::register_itself();
	ModuleMovementFastCollidingProjectile::register_itself();
	ModuleMovementHover::register_itself();
	ModuleMovementHoverOld::register_itself();
	ModuleMovementOnFloor::register_itself().be_default_for(Names::actor);
	ModuleMovementPath::register_itself();
	ModuleMovementProjectile::register_itself();
	ModuleMovementScripted::register_itself();
	ModuleMovementSwitch::register_itself();

	// nav elements
	ModuleNavElementNode::register_itself();

	// presence
	ModulePresence::register_itself().be_default();
	//ModulePresenceServant.register_itself();

	// sound
	ModuleSound::register_itself().be_default();

	// temporary objects
	ModuleTemporaryObjects::register_itself().be_default();
};

void Modules::close_static()
{
	Modules::ai.clear();
	Modules::appearance.clear();
	Modules::collision.clear();
	Modules::controller.clear();
	Modules::custom.clear();
	Modules::gameplay.clear();
	Modules::movement.clear();
	Modules::navElement.clear();
	Modules::presence.clear();
	Modules::sound.clear();
	Modules::temporaryObjects.clear();
}
