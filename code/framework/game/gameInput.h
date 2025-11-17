#pragma once

#include "gameInputDefinition.h"

namespace Framework
{
	namespace GameInputIncludeVR
	{
		enum Type
		{
			None = 0,
			Left = 1,
			Right = 2,
			All = Left | Right,
		};
		typedef int Flags;
	}
	
	interface_class IGameInput
	{
	public:
		virtual ~IGameInput() {}

		virtual ::System::Input* get_input() const { return nullptr; } // if based on system input

		virtual bool has_stick(Name const & _name) const = 0;
		virtual Vector2 get_stick(Name const & _name) const = 0;
		
		virtual bool has_mouse(Name const & _name) const = 0;
		virtual Vector2 get_mouse(Name const & _name) const = 0;

		virtual float get_button_state(Name const & _name) const = 0;
		virtual bool has_button(Name const & _name) const = 0;
		virtual bool is_button_pressed(Name const & _name) const = 0;
		virtual bool has_button_been_pressed(Name const & _name) const = 0;
		virtual bool has_button_been_released(Name const & _name) const = 0;
	};

	class GameInput
	: public IGameInput
	{
	public:
		GameInput();

		void use(GameInputDefinition* _gid);
		void use(Name const & _gidName);
		void use(GameInputIncludeVR::Flags _includeVR, bool _useNormalInputWithVRToo = false);

		void disable() { isEnabled = false; }
		void enable() { isEnabled = true; }

	public:
		static bool check_condition(TagCondition const * _condition, Optional<Hand::Type> const& _hand = NP);

	public: // IGameInput
		implement_::System::Input* get_input() const { return ::System::Input::get(); }

		implement_ bool has_stick(Name const & _name) const;
		implement_ Vector2 get_stick(Name const & _name) const;

		implement_ bool has_mouse(Name const & _name) const;
		implement_ Vector2 get_mouse(Name const & _name) const;

		implement_ float get_button_state(Name const & _name) const;
		implement_ bool is_button_available(Name const & _name) const; // checks conditions etc
		implement_ bool has_button(Name const & _name) const;
		implement_ bool is_button_pressed(Name const & _name) const;
		implement_ bool has_button_been_pressed(Name const & _name) const;
		implement_ bool has_button_been_released(Name const & _name) const;

	private:
		bool isEnabled = true;

		bool allGamepads = true;
		bool useNormalInput = true;
		GameInputIncludeVR::Flags includeVR = GameInputIncludeVR::None;

		RefCountObjectPtr<GameInputDefinition> definition;
	};
};
