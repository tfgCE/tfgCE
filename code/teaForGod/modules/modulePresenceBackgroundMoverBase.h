#pragma once

#include "..\..\framework\module\modulePresence.h"

namespace TeaForGodEmperor
{
	/*
	 *	This is to make it easier/possible to tell whether we're moving.
	 *	As background mover keeps base/platform in the same spot, code in pilgrim module won't be able to tell if we're on a moving platform or not.
	 */
	class ModulePresenceBackgroundMoverBase
	: public Framework::ModulePresence
	{
		FAST_CAST_DECLARE(ModulePresenceBackgroundMoverBase);
		FAST_CAST_BASE(Framework::ModulePresence);
		FAST_CAST_END();

		typedef Framework::ModulePresence base;
	public:
		static Framework::RegisteredModule<Framework::ModulePresence> & register_itself();

		ModulePresenceBackgroundMoverBase(Framework::IModulesOwner* _owner);

		bool is_moving_for_background_mover() const { return moving; }
		void set_moving_for_background_mover(bool _moving) { moving = _moving; }

	private:
		bool moving = false;
	};
};

