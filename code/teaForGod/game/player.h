#pragma once

#include "..\teaEnums.h"

#include "..\..\framework\game\player.h"

namespace TeaForGodEmperor
{
	namespace ControlMode
	{
		enum Type
		{
			None,
			Actor,
			UI
		};
	};

	class Player
	: public Framework::Player
	{
		FAST_CAST_DECLARE(Player);
		FAST_CAST_BASE(Framework::Player);
		FAST_CAST_END();

		typedef Framework::Player base;
	public:
		Player();
		virtual ~Player();

		void set_control_mode(ControlMode::Type _controlMode) { controlMode = _controlMode; }
		ControlMode::Type get_control_mode() const { return controlMode; }

		void set_allow_controlling_both(bool _allowControllingBoth, bool _seeThroughMyEyes) { allowControllingBoth = _allowControllingBoth; seeThroughMyEyes = _seeThroughMyEyes; }
		bool may_control_both() const { return allowControllingBoth; }
		bool should_see_through_my_eyes() const { return get_actor() && (! allowControllingBoth || seeThroughMyEyes); }

	protected:
		override_ void on_actor_change();

	private:
		ControlMode::Type controlMode = ControlMode::Actor;
		
		bool allowControllingBoth = false; // if allowed, may control both at the same time (reset to false on every bind/unbind)
		bool seeThroughMyEyes = false; // if controlling both, we have to explicitly say if we want to see through my (this) eyes
	};
};
