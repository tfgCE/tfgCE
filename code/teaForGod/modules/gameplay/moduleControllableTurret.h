#pragma once

#include "..\controllable.h"
#include "..\moduleGameplay.h"

#include "..\..\utils\overlayStatus.h"

#include "..\..\teaEnums.h"
#include "..\..\..\framework\appearance\socketID.h"

namespace TeaForGodEmperor
{
	class ModulePortableShieldData;

	class ModuleControllableTurret
	: public ModuleGameplay
	, public IControllable
	{
		FAST_CAST_DECLARE(ModuleControllableTurret);
		FAST_CAST_BASE(ModuleGameplay);
		FAST_CAST_BASE(IControllable);
		FAST_CAST_END();

		typedef ModuleGameplay base;
	public:
		static Framework::RegisteredModule<Framework::ModuleGameplay> & register_itself();

		ModuleControllableTurret(Framework::IModulesOwner* _owner);
		virtual ~ModuleControllableTurret();

	protected: // Module
		override_ void setup_with(Framework::ModuleData const* _moduleData);

		override_ void initialise();

	protected: // ModuleGameplay
		override_ void advance_post_move(float _deltaTime);

	public: // IControllable
		implement_ void pre_process_controls();
		implement_ void process_controls(Hand::Type _hand, Framework::GameInput const& _controls, float _deltaTime);

	protected:
		float rpm = 1.0f; // this is per muzzle, it gives continuous fire 4x for 4 muzzles

		SimpleVariableInfo playerInControlVar; // float var set to 1 or 0

		Name appearAsDeadStoryFact;

		bool canShootAttachedTo = true;

		int shootRequests = 0; // the type of shooting, 1 is continuous, 2 is all at once
		int shootRequestDir = 1; // default/right is 1, otherwise -1

		float timeSinceLastShot = 0.0f;
		int lastShotMuzzleIdx = NONE;

		struct Muzzle
		{
			float toToBeReady = 0.0f;
			Framework::SocketID muzzleSocket;
			bool shootNow = false;
		};
		Array<Muzzle> muzzles; // per muzzle

		OverlayStatus overlayStatus;
	};

	class ModuleControllableTurretData
	: public Framework::ModuleData
	{
		FAST_CAST_DECLARE(ModuleControllableTurretData);
		FAST_CAST_BASE(Framework::ModuleData);
		FAST_CAST_END();

		typedef Framework::ModuleData base;
	public:
		ModuleControllableTurretData(Framework::LibraryStored* _inLibraryStored);
		virtual ~ModuleControllableTurretData();

	private:
	};
};

