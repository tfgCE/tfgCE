#pragma once

#include "moduleAI.h"
#include "moduleAppearance.h"
#include "moduleCollision.h"
#include "moduleController.h"
#include "moduleCustom.h"
#include "moduleGameplay.h"
#include "moduleMovement.h"
#include "moduleNavElement.h"
#include "modulePresence.h"
#include "moduleSound.h"
#include "moduleTemporaryObjects.h"
#include "registeredModule.h"

namespace Framework
{
	/**
	 *	Modules should advance separetly and provide just info for other modules.
	 *	One that breaks from it, is Movement module, which advances Appearance (to have forced velocity present) and modifies Presence.
	 */

	class Modules
	{
	public:
		//public static RegisteredModuleSet<ModuleActionHandler> actionHandler;
		static RegisteredModuleSet<ModuleAI, ModuleAIData> ai;
		static RegisteredModuleSet<ModuleAppearance, ModuleAppearanceData> appearance;
		static RegisteredModuleSet<ModuleCollision, ModuleCollisionData> collision;
		static RegisteredModuleSet<ModuleController, ModuleData> controller;
		static RegisteredModuleSet<ModuleCustom, ModuleData> custom;
		static RegisteredModuleSet<ModuleGameplay, ModuleData> gameplay;
		static RegisteredModuleSet<ModuleMovement, ModuleMovementData> movement;
		static RegisteredModuleSet<ModuleNavElement, ModuleData> navElement;
		static RegisteredModuleSet<ModulePresence, ModulePresenceData> presence;
		static RegisteredModuleSet<ModuleSound, ModuleSoundData> sound;
		static RegisteredModuleSet<ModuleTemporaryObjects, ModuleTemporaryObjectsData> temporaryObjects;

		static void initialise_static();
		static void close_static();
	};
};
