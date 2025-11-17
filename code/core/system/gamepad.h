#pragma once

#ifdef AN_SDL
#include "SDL.h"
#endif

#include "..\math\math.h"

namespace System
{
	class Input;

	namespace GamepadAxis
	{
		enum Type
		{
			None = -1,
			//
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
			LeftX = SDL_CONTROLLER_AXIS_LEFTX,
			LeftY = SDL_CONTROLLER_AXIS_LEFTY,
			//
			RightX = SDL_CONTROLLER_AXIS_RIGHTX,
			RightY = SDL_CONTROLLER_AXIS_RIGHTY,
			//
			TriggerLeft = SDL_CONTROLLER_AXIS_TRIGGERLEFT,
			TriggerRight = SDL_CONTROLLER_AXIS_TRIGGERRIGHT,
			//
			Max = SDL_CONTROLLER_AXIS_MAX
#else
			#error implement for non sdl
#endif
#else
			Max = 1
#endif
		};

		Type parse(String const & _value, Type _default);
		tchar const * const to_char(GamepadAxis::Type _axis);
	};

	namespace GamepadStick
	{
		enum Type
		{
			None = -1,
			//
			Left,
			Right,
			DPad,
			//
			Max
		};

		Type parse(String const & _value, Type _default);
		tchar const * const to_char(Type _stick);
	};

	namespace GamepadStickDir
	{
		enum Type
		{
			None,
			Left,
			Right,
			Down,
			Up
		};

		Type parse(String const & _value, Type _default);
		tchar const * const to_char(Type _stick);
	};

	namespace GamepadButton
	{
		enum Type
		{
			None = -1,
			//
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
			A = SDL_CONTROLLER_BUTTON_A,
			B = SDL_CONTROLLER_BUTTON_B,
			X = SDL_CONTROLLER_BUTTON_X,
			Y = SDL_CONTROLLER_BUTTON_Y,
			//
			LeftStick = SDL_CONTROLLER_BUTTON_LEFTSTICK,
			RightStick = SDL_CONTROLLER_BUTTON_RIGHTSTICK,
			LeftBumper = SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
			RightBumper = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
			LeftTrigger = SDL_CONTROLLER_BUTTON_MAX,
			RightTrigger = SDL_CONTROLLER_BUTTON_MAX + 1,
			//
			Back = SDL_CONTROLLER_BUTTON_BACK,
			Guide = SDL_CONTROLLER_BUTTON_GUIDE,
			Start = SDL_CONTROLLER_BUTTON_START,
			//
			DPadUp = SDL_CONTROLLER_BUTTON_DPAD_UP,
			DPadDown = SDL_CONTROLLER_BUTTON_DPAD_DOWN,
			DPadLeft = SDL_CONTROLLER_BUTTON_DPAD_LEFT,
			DPadRight = SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
			//
			LStickUp = SDL_CONTROLLER_BUTTON_MAX + 2,
			LStickDown = SDL_CONTROLLER_BUTTON_MAX + 3,
			LStickLeft = SDL_CONTROLLER_BUTTON_MAX + 4,
			LStickRight = SDL_CONTROLLER_BUTTON_MAX + 5,
			//
			RStickUp = SDL_CONTROLLER_BUTTON_MAX + 6,
			RStickDown = SDL_CONTROLLER_BUTTON_MAX + 7,
			RStickLeft = SDL_CONTROLLER_BUTTON_MAX + 8,
			RStickRight = SDL_CONTROLLER_BUTTON_MAX + 9,
			//
			Max = SDL_CONTROLLER_BUTTON_MAX + 10 // check other MAX to see why it is so, those buttons are emulated
#else
			#error implement for non sdl
#endif
#else
			Max = 1
#endif
		};

		Type parse(String const & _value, Type _default);
		tchar const * const to_char(GamepadButton::Type _button);

		Type from_stick(GamepadStick::Type _stick, GamepadStickDir::Type _dir);
	};

	/**
	 *	Some of the things are emulated:
	 *		what			from		to
	 *		d-pad			buttons		stick
	 *		triggers		axes		buttons
	 *		sticks (l&r)	axes		buttons (just directionals, similar to d-pad)
	 */
	class Gamepad
	{
	public:
		static bool can_create_gamepad(int _systemEnumIdx, Input* _input = nullptr);
		static Gamepad* create(int _systemEnumIdx, Input* _input = nullptr); // will check if it is possible to create gamepad and will create it only then

		~Gamepad();

		void close();

		bool is_ok() const {
#ifdef AN_STANDARD_INPUT
	#ifdef AN_SDL
			return gamepad != nullptr;
	#else
		#error implement for non sdl
	#endif
#else
			return false;
#endif
		}
		int get_system_id() const { return systemID; }
		int get_index() const { return index; }

		void store_button(int _systemButtonID, bool _pressed);
		void store_axis(int _systemAxisID, float _value);

		void advance(float _deltaTime);

		bool is_active() const { return isActive; }
		bool is_any_button_pressed() const;

		Vector2 get_stick(GamepadStick::Type _stick); // with dead zones
		Vector2 get_left_stick(); // with dead zones
		Vector2 get_right_stick(); // with dead zones
		Vector2 get_dpad_stick(); // with dead zones

		Vector2 get_stick_raw(GamepadStick::Type _stick); // with dead zones
		Vector2 get_left_stick_raw();
		Vector2 get_right_stick_raw();
		Vector2 get_dpad_stick_raw();

		float get_button_state(GamepadButton::Type _systemButtonID) const;
		float get_button_state(Array<GamepadButton::Type> const & _systemButtonID) const;
		bool is_button_pressed(GamepadButton::Type _systemButtonID) const;
		bool is_button_pressed(Array<GamepadButton::Type> const & _systemButtonID) const;
		bool has_button_been_pressed(GamepadButton::Type _systemButtonID) const;
		bool has_button_been_pressed(Array<GamepadButton::Type> const & _systemButtonID) const;
		bool has_button_been_released(GamepadButton::Type _systemButtonID) const;
		bool has_button_been_released(Array<GamepadButton::Type> const & _systemButtonID) const;

		static float apply_dead_zone(float _stick, float _deadZone);
		static Vector2 apply_dead_zone(Vector2 const & _stick, Vector2 const & _deadZone);

	private:
#ifdef AN_STANDARD_INPUT
		const float AXIS_TO_BUTTON_TRESHOLD = 0.5f;
#endif

		Input* input;
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
		SDL_GameController* gamepad;
#else
		#error implement for non sdl
#endif
#endif
		int index;
		int systemID;
		bool isActive;

		bool buttons[GamepadButton::Max];
		bool prevButtons[GamepadButton::Max];

		float axis[GamepadAxis::Max];
		float axisSystemMultiplier[GamepadAxis::Max];
		float axisDeadZone[GamepadAxis::Max];
		float axisSensitivity[GamepadAxis::Max];

		Gamepad(int _systemEnumIdx, Input* _input = nullptr);
	};

};
