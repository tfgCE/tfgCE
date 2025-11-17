#pragma once

#include "..\..\framework\module\moduleMovement.h"

#include "..\..\core\system\timeStamp.h"

namespace TeaForGodEmperor
{
	/**
	 *	Follows behind a pilgrim to keep it in the valid world.
	 *	Follows either centre or eyes (depending on pilgrim setting).
	 */
	class ModuleMovementInRoomFollower
	: public Framework::ModuleMovement
	{
		FAST_CAST_DECLARE(ModuleMovementInRoomFollower);
		FAST_CAST_BASE(Framework::ModuleMovement);
		FAST_CAST_END();

		typedef ModuleMovement base;
	public:
		static Framework::RegisteredModule<Framework::ModuleMovement> & register_itself();

		ModuleMovementInRoomFollower(Framework::IModulesOwner* _owner);
		virtual ~ModuleMovementInRoomFollower();
	
	public: // Module
		override_ void setup_with(Framework::ModuleData const* _moduleData);

	protected: // ModuleMovement
		override_ void apply_changes_to_velocity_and_displacement(float _deltaTime, VelocityAndDisplacementContext& _context);

	private:
		SafePtr<Framework::IModulesOwner> followObject;

		bool followPlayer = false;
		Name followSocket = Name::invalid();
		bool axisAligned = true;
	};
};

