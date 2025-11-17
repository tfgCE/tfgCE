#include "gamepad.h"

#include "input.h"

using namespace System;

GamepadButton::Type GamepadButton::parse(String const & _value, GamepadButton::Type _default)
{
#ifdef AN_STANDARD_INPUT
	if (_value == TXT("a")) return GamepadButton::A;
	if (_value == TXT("b")) return GamepadButton::B;
	if (_value == TXT("x")) return GamepadButton::X;
	if (_value == TXT("y")) return GamepadButton::Y;
	//
	if (_value == TXT("leftStick")) return GamepadButton::LeftStick;
	if (_value == TXT("rightStick")) return GamepadButton::RightStick;
	if (_value == TXT("leftBumper")) return GamepadButton::LeftBumper;
	if (_value == TXT("rightBumper")) return GamepadButton::RightBumper;
	if (_value == TXT("leftTrigger")) return GamepadButton::LeftTrigger;
	if (_value == TXT("rightTrigger")) return GamepadButton::RightTrigger;
	//
	if (_value == TXT("back")) return GamepadButton::Back;
	if (_value == TXT("start")) return GamepadButton::Start;
	//
	if (_value == TXT("d-padUp") ||
		_value == TXT("dPadUp")) return GamepadButton::DPadUp;
	if (_value == TXT("d-padDown") ||
		_value == TXT("dPadDown")) return GamepadButton::DPadDown;
	if (_value == TXT("d-padLeft") ||
		_value == TXT("dPadLeft")) return GamepadButton::DPadLeft;
	if (_value == TXT("d-padRight") ||
		_value == TXT("dPadRight")) return GamepadButton::DPadRight;
	//
	if (_value == TXT("leftStickUp") ||
		_value == TXT("lStickUp")) return GamepadButton::LStickUp;
	if (_value == TXT("leftStickDown") ||
		_value == TXT("lStickDown")) return GamepadButton::LStickDown;
	if (_value == TXT("leftStickLeft") ||
		_value == TXT("lStickLeft")) return GamepadButton::LStickLeft;
	if (_value == TXT("leftStickRight") ||
		_value == TXT("lStickRight")) return GamepadButton::LStickRight;
	//
	if (_value == TXT("rightStickUp") ||
		_value == TXT("rStickUp")) return GamepadButton::RStickUp;
	if (_value == TXT("rightStickDown") ||
		_value == TXT("rStickDown")) return GamepadButton::RStickDown;
	if (_value == TXT("rightStickright") ||
		_value == TXT("rStickRight")) return GamepadButton::RStickRight;
	if (_value == TXT("rightStickRight") ||
		_value == TXT("rStickRight")) return GamepadButton::RStickRight;
	//
	if (_value != TXT(""))
	{
		todo_important(TXT("more buttons!"));
	}
	//
#endif
	return _default;
}

tchar const * const GamepadButton::to_char(GamepadButton::Type _button)
{
#ifdef AN_STANDARD_INPUT
	if (_button == GamepadButton::A) return TXT("a");
	if (_button == GamepadButton::B) return TXT("b");
	if (_button == GamepadButton::X) return TXT("x");
	if (_button == GamepadButton::Y) return TXT("y");
	//
	if (_button == GamepadButton::LeftStick) return TXT("leftStick");
	if (_button == GamepadButton::RightStick) return TXT("rightStick");
	if (_button == GamepadButton::LeftBumper) return TXT("leftBumper");
	if (_button == GamepadButton::RightBumper) return TXT("rightBumper");
	if (_button == GamepadButton::LeftTrigger) return TXT("leftTrigger");
	if (_button == GamepadButton::RightTrigger) return TXT("rightTrigger");
	//
	if (_button == GamepadButton::Back) return TXT("back");
	if (_button == GamepadButton::Start) return TXT("start");
	//
	if (_button == GamepadButton::DPadUp) return TXT("dPadUp");
	if (_button == GamepadButton::DPadDown) return TXT("dPadDown");
	if (_button == GamepadButton::DPadLeft) return TXT("dPadLeft");
	if (_button == GamepadButton::DPadRight) return TXT("dPadRight");
	//
	if (_button == GamepadButton::LStickUp) return TXT("leftStickUp");
	if (_button == GamepadButton::LStickDown) return TXT("leftStickDown");
	if (_button == GamepadButton::LStickLeft) return TXT("leftStickLeft");
	if (_button == GamepadButton::LStickRight) return TXT("leftStickRight");
	//
	if (_button == GamepadButton::RStickUp) return TXT("rightStickUp");
	if (_button == GamepadButton::RStickDown) return TXT("rightStickDown");
	if (_button == GamepadButton::RStickLeft) return TXT("rightStickLeft");
	if (_button == GamepadButton::RStickRight) return TXT("rightStickRight");
	//
#endif
	return TXT("");
}

GamepadButton::Type GamepadButton::from_stick(GamepadStick::Type _stick, GamepadStickDir::Type _dir)
{
#ifdef AN_STANDARD_INPUT
	if (_stick == GamepadStick::Left)
	{
		if (_dir == GamepadStickDir::Left) return GamepadButton::LStickLeft;
		if (_dir == GamepadStickDir::Right) return GamepadButton::LStickRight;
		if (_dir == GamepadStickDir::Down) return GamepadButton::LStickDown;
		if (_dir == GamepadStickDir::Up) return GamepadButton::LStickUp;
	}
	if (_stick == GamepadStick::Right)
	{
		if (_dir == GamepadStickDir::Left) return GamepadButton::RStickLeft;
		if (_dir == GamepadStickDir::Right) return GamepadButton::RStickRight;
		if (_dir == GamepadStickDir::Down) return GamepadButton::RStickDown;
		if (_dir == GamepadStickDir::Up) return GamepadButton::RStickUp;
	}
	if (_stick == GamepadStick::DPad)
	{
		if (_dir == GamepadStickDir::Left) return GamepadButton::DPadLeft;
		if (_dir == GamepadStickDir::Right) return GamepadButton::DPadRight;
		if (_dir == GamepadStickDir::Down) return GamepadButton::DPadDown;
		if (_dir == GamepadStickDir::Up) return GamepadButton::DPadUp;
	}
#endif
	return GamepadButton::None;
}

//

GamepadStick::Type GamepadStick::parse(String const & _value, GamepadStick::Type _default)
{
#ifdef AN_STANDARD_INPUT
	if (_value == TXT("left")) return GamepadStick::Left;
	if (_value == TXT("right")) return GamepadStick::Right;
	if (_value == TXT("dpad") ||
		_value == TXT("d-pad")) return GamepadStick::DPad;
	//
#endif
	return _default;
}

tchar const * const GamepadStick::to_char(GamepadStick::Type _stick)
{
#ifdef AN_STANDARD_INPUT
	if (_stick == GamepadStick::Left) return TXT("left");
	if (_stick == GamepadStick::Right) return TXT("right");
	if (_stick == GamepadStick::DPad) return TXT("d-pad");
	//
#endif
	return TXT("");
}

//

GamepadAxis::Type GamepadAxis::parse(String const & _value, GamepadAxis::Type _default)
{
#ifdef AN_STANDARD_INPUT
	if (_value == TXT("leftX")) return GamepadAxis::LeftX;
	if (_value == TXT("leftY")) return GamepadAxis::LeftY;
	//
	if (_value == TXT("rightX")) return GamepadAxis::RightX;
	if (_value == TXT("rightY")) return GamepadAxis::RightY;
	//
	if (_value == TXT("triggerLeft")) return GamepadAxis::TriggerLeft;
	if (_value == TXT("triggerRight")) return GamepadAxis::TriggerRight;
	//
#endif
	return _default;
}

tchar const * const GamepadAxis::to_char(GamepadAxis::Type _axis)
{
#ifdef AN_STANDARD_INPUT
	if (_axis == GamepadAxis::LeftY) return TXT("leftY");
	if (_axis == GamepadAxis::LeftX) return TXT("leftX");
	//
	if (_axis == GamepadAxis::RightX) return TXT("rightX");
	if (_axis == GamepadAxis::RightY) return TXT("rightY");
	//
	if (_axis == GamepadAxis::TriggerLeft) return TXT("triggerLeft");
	if (_axis == GamepadAxis::TriggerRight) return TXT("triggerRight");
	//
#endif
	return TXT("");
}

//

GamepadStickDir::Type GamepadStickDir::parse(String const & _value, GamepadStickDir::Type _default)
{
#ifdef AN_STANDARD_INPUT
	if (_value == TXT("left")) return GamepadStickDir::Left;
	if (_value == TXT("right")) return GamepadStickDir::Right;
	if (_value == TXT("down")) return GamepadStickDir::Down;
	if (_value == TXT("up")) return GamepadStickDir::Up;
	//
#endif
	return _default;
}

tchar const * const GamepadStickDir::to_char(GamepadStickDir::Type _stick)
{
#ifdef AN_STANDARD_INPUT
	if (_stick == GamepadStickDir::Left) return TXT("left");
	if (_stick == GamepadStickDir::Right) return TXT("right");
	if (_stick == GamepadStickDir::Down) return TXT("down");
	if (_stick == GamepadStickDir::Up) return TXT("up");
	//
#endif
	return TXT("");
}

//

#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	#define GET_SYSTEM_ID_FROM_GAME_CONTROLLER(_gameController) \
		SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(_gameController))
#else
	#error implement for non sdl
#endif
#else
	#define GET_SYSTEM_ID_FROM_GAME_CONTROLLER(_gameController) 0
#endif
	
bool Gamepad::can_create_gamepad(int _systemEnumIdx, Input* _input)
{
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	if (!SDL_IsGameController(_systemEnumIdx))
	{
		return false;
	}
#else
	#error implement for non sdl
#endif
	if (_input == nullptr)
	{
		_input = Input::get();
	}
	if (_input != nullptr)
	{
#ifdef AN_SDL
		SDL_GameController* gameController = SDL_GameControllerOpen(_systemEnumIdx);
		if (!gameController)
		{
			return false;
		}
		for_every_ptr(gamepad, _input->gamepads)
		{
			if (gamepad->systemID == GET_SYSTEM_ID_FROM_GAME_CONTROLLER(gameController))
			{
				return false;
			}
		}
#else
		#error implement for non sdl
#endif
	}
	return true;
#else
	return false;
#endif
}

Gamepad* Gamepad::create(int _systemEnumIdx, Input* _input)
{
	if (can_create_gamepad(_systemEnumIdx, _input))
	{
		return new Gamepad(_systemEnumIdx, _input);
	}
	else
	{
		return nullptr;
	}
}

Gamepad::Gamepad(int _systemEnumIdx, Input* _input)
: input(nullptr)
#ifdef AN_STANDARD_INPUT
, gamepad(nullptr)
#endif
, index(NONE)
, isActive(false)
{
#ifndef AN_STANDARD_INPUT
	an_assert(TXT("how come we created a gamepad?"));
#endif
	an_assert(can_create_gamepad(_systemEnumIdx, _input));

#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	gamepad = SDL_GameControllerOpen(_systemEnumIdx);
#else
	#error implement for non sdl
#endif
	an_assert(gamepad, TXT("should exist as we checked can_create_gamepad"));
	systemID = GET_SYSTEM_ID_FROM_GAME_CONTROLLER(gamepad);

	output_colour_system();
#ifdef AN_SDL
	String gamepadName = String::from_char8(SDL_GameControllerName(gamepad));
	output(TXT("created gamepad \"%S\""), gamepadName.to_char());
#else
	#error implement for non sdl
#endif
	output_colour();
#endif

	input = _input;
	if (input == nullptr)
	{
		input = Input::get();
	}
	an_assert(input != nullptr);

	// find index
	index = 0;
	bool foundIndex = false;
	while (!foundIndex)
	{
		foundIndex = true;
		for_every_ptr(existingGamepad, input->gamepads)
		{
			if (existingGamepad->index == index)
			{
				++index;
				foundIndex = false;
			}
		}
	}

	input->gamepads.push_back(this);

	for_count(int, button, GamepadButton::Max)
	{
		buttons[button] = prevButtons[button] = false;
	}

	for_count(int, axisIdx, GamepadAxis::Max)
	{
		axis[axisIdx] = 0.0f;
		axisSystemMultiplier[axisIdx] = 1.0f;
		axisDeadZone[axisIdx] = 0.05f;
		axisSensitivity[axisIdx] = 1.0f;
	}

#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	// invert Y axis as SDL has positive values for down but we have positive for up
	axisSystemMultiplier[GamepadAxis::LeftY] *= -1.0f;
	axisSystemMultiplier[GamepadAxis::RightY] *= -1.0f;
#endif
#endif
}

Gamepad::~Gamepad()
{
	close();
}

void Gamepad::close()
{
#ifdef AN_STANDARD_INPUT
	if (gamepad)
	{
#ifdef AN_SDL
		SDL_GameControllerClose(gamepad);
#else
		#error implement for non sdl
#endif
		input->gamepads.remove(this);
		gamepad = nullptr;
	}
#endif
}

void Gamepad::store_button(int _systemButtonID, bool _pressed)
{
	if (_systemButtonID >= 0 && _systemButtonID < GamepadButton::Max)
	{
		buttons[_systemButtonID] = _pressed; 
		isActive |= _pressed;
	}
}

void Gamepad::store_axis(int _systemAxisID, float _value)
{
#ifdef AN_STANDARD_INPUT
	if (_systemAxisID >= 0 && _systemAxisID < GamepadAxis::Max)
	{
		axis[_systemAxisID] = _value * axisSystemMultiplier[_systemAxisID];
		isActive |= apply_dead_zone(axis[_systemAxisID], axisDeadZone[_systemAxisID]) != 0.0f;

#ifdef AN_SDL
		if (_systemAxisID == SDL_CONTROLLER_AXIS_TRIGGERLEFT ||
			_systemAxisID == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
		{
			bool buttonPressed = axis[_systemAxisID] >= AXIS_TO_BUTTON_TRESHOLD;
			GamepadButton::Type button = _systemAxisID == SDL_CONTROLLER_AXIS_TRIGGERLEFT ? GamepadButton::LeftTrigger : GamepadButton::RightTrigger;
			buttons[button] = buttonPressed;
			isActive |= buttonPressed;
		}
		if (_systemAxisID == SDL_CONTROLLER_AXIS_LEFTX)
		{
			bool buttonPressedLeft = axis[_systemAxisID] <= -AXIS_TO_BUTTON_TRESHOLD;
			bool buttonPressedRight = axis[_systemAxisID] >= AXIS_TO_BUTTON_TRESHOLD;
			buttons[GamepadButton::LStickLeft] = buttonPressedLeft;
			buttons[GamepadButton::LStickRight] = buttonPressedRight;
			isActive |= buttonPressedLeft || buttonPressedRight;
		}
		if (_systemAxisID == SDL_CONTROLLER_AXIS_LEFTY)
		{
			bool buttonPressedDown = axis[_systemAxisID] <= -AXIS_TO_BUTTON_TRESHOLD;
			bool buttonPressedUp = axis[_systemAxisID] >= AXIS_TO_BUTTON_TRESHOLD;
			buttons[GamepadButton::LStickDown] = buttonPressedDown;
			buttons[GamepadButton::LStickUp] = buttonPressedUp;
			isActive |= buttonPressedDown || buttonPressedUp;
		}
		if (_systemAxisID == SDL_CONTROLLER_AXIS_RIGHTX)
		{
			bool buttonPressedLeft = axis[_systemAxisID] <= -AXIS_TO_BUTTON_TRESHOLD;
			bool buttonPressedRight = axis[_systemAxisID] >= AXIS_TO_BUTTON_TRESHOLD;
			buttons[GamepadButton::RStickLeft] = buttonPressedLeft;
			buttons[GamepadButton::RStickRight] = buttonPressedRight;
			isActive |= buttonPressedLeft || buttonPressedRight;
		}
		if (_systemAxisID == SDL_CONTROLLER_AXIS_RIGHTY)
		{
			bool buttonPressedDown = axis[_systemAxisID] <= -AXIS_TO_BUTTON_TRESHOLD;
			bool buttonPressedUp = axis[_systemAxisID] >= AXIS_TO_BUTTON_TRESHOLD;
			buttons[GamepadButton::RStickDown] = buttonPressedDown;
			buttons[GamepadButton::RStickUp] = buttonPressedUp;
			isActive |= buttonPressedDown || buttonPressedUp;
		}
#else
		#error implement for non sdl
#endif
	}
#endif
}

void Gamepad::advance(float _deltaTime)
{
	isActive = false;

#ifdef AN_STANDARD_INPUT
	if (!gamepad)
#endif
	{
		return;
	}

	for_count(int, button, GamepadButton::Max)
	{
		prevButtons[button] = buttons[button];
	}
}

float Gamepad::apply_dead_zone(float _stick, float _deadZone)
{
	if (_stick > _deadZone)
	{
		return 1.0f - (1.0f - _stick) / (1.0f - _deadZone);
	}
	if (_stick < -_deadZone)
	{
		return -1.0f - (-1.0f - _stick) / (1.0f - _deadZone);
	}
	return 0.0f;
}

Vector2 Gamepad::apply_dead_zone(Vector2 const & _stick, Vector2 const & _deadZone)
{
	return Vector2(apply_dead_zone(_stick.x, _deadZone.x),
				   apply_dead_zone(_stick.y, _deadZone.y));
}

Vector2 Gamepad::get_left_stick()
{
#ifdef AN_STANDARD_INPUT
	return Vector2(apply_dead_zone(axis[GamepadAxis::LeftX], axisDeadZone[GamepadAxis::LeftX]) * axisSensitivity[GamepadAxis::LeftX],
				   apply_dead_zone(axis[GamepadAxis::LeftY], axisDeadZone[GamepadAxis::LeftY]) * axisSensitivity[GamepadAxis::LeftY]);
#else
	return Vector2::zero;
#endif
}

Vector2 Gamepad::get_right_stick()
{
#ifdef AN_STANDARD_INPUT
	return Vector2(apply_dead_zone(axis[GamepadAxis::RightX], axisDeadZone[GamepadAxis::RightX]) * axisSensitivity[GamepadAxis::RightX],
				   apply_dead_zone(axis[GamepadAxis::RightY], axisDeadZone[GamepadAxis::RightY]) * axisSensitivity[GamepadAxis::RightY]);
#else
	return Vector2::zero;
#endif
}

Vector2 Gamepad::get_dpad_stick()
{
	return get_dpad_stick_raw();
}

Vector2 Gamepad::get_left_stick_raw()
{
#ifdef AN_STANDARD_INPUT
	return Vector2(axis[GamepadAxis::LeftX], axis[GamepadAxis::LeftY]);
#else
	return Vector2::zero;
#endif
}

Vector2 Gamepad::get_right_stick_raw()
{
#ifdef AN_STANDARD_INPUT
	return Vector2(axis[GamepadAxis::RightX], axis[GamepadAxis::RightY]);
#else
	return Vector2::zero;
#endif
}

Vector2 Gamepad::get_dpad_stick_raw()
{
	Vector2 result = Vector2::zero;
#ifdef AN_STANDARD_INPUT
	if (buttons[GamepadButton::DPadLeft])
	{
		result.x -= 1.0f;
	}
	if (buttons[GamepadButton::DPadRight])
	{
		result.x += 1.0f;
	}
	if (buttons[GamepadButton::DPadDown])
	{
		result.y -= 1.0f;
	}
	if (buttons[GamepadButton::DPadUp])
	{
		result.y += 1.0f;
	}
#endif
	return result;
}

float Gamepad::get_button_state(GamepadButton::Type _systemButtonID) const
{
#ifdef AN_STANDARD_INPUT
	if (_systemButtonID == GamepadButton::LeftTrigger)
	{
		return axis[GamepadAxis::TriggerLeft];
	}
	if (_systemButtonID == GamepadButton::RightTrigger)
	{
		return axis[GamepadAxis::TriggerRight];
	}
	if (_systemButtonID >= 0 && _systemButtonID < GamepadButton::Max)
	{
		return buttons[_systemButtonID]? 1.0f : 0.0f;
	}
#endif
	return 0.0f;
}

float Gamepad::get_button_state(Array<GamepadButton::Type> const & _systemButtonIDs) const
{
	float state = 0.0f;
	for_every(button, _systemButtonIDs)
	{
		state = max(state, get_button_state(*button));
	}
	return state;
}

bool Gamepad::is_button_pressed(GamepadButton::Type _systemButtonID) const
{
	if (_systemButtonID >= 0 && _systemButtonID < GamepadButton::Max)
	{
		return buttons[_systemButtonID];
	}
	return false;
}

bool Gamepad::is_button_pressed(Array<GamepadButton::Type> const & _systemButtonIDs) const
{
	for_every(button, _systemButtonIDs)
	{
		if (*button >= 0 && *button < GamepadButton::Max)
		{
			if (buttons[*button])
			{
				return true;
			}
		}
	}
	return false;
}

bool Gamepad::has_button_been_pressed(GamepadButton::Type _systemButtonID) const
{
	if (_systemButtonID >= 0 && _systemButtonID < GamepadButton::Max)
	{
		return buttons[_systemButtonID] && ! prevButtons[_systemButtonID];
	}
	return false;
}

bool Gamepad::has_button_been_pressed(Array<GamepadButton::Type> const & _systemButtonIDs) const
{
	for_every(button, _systemButtonIDs)
	{
		if (*button >= 0 && *button < GamepadButton::Max)
		{
			if (buttons[*button] && !prevButtons[*button])
			{
				return true;
			}
		}
	}
	return false;
}

bool Gamepad::has_button_been_released(GamepadButton::Type _systemButtonID) const
{
	if (_systemButtonID >= 0 && _systemButtonID < GamepadButton::Max)
	{
		return !buttons[_systemButtonID] && prevButtons[_systemButtonID];
	}
	return false;
}

bool Gamepad::has_button_been_released(Array<GamepadButton::Type> const & _systemButtonIDs) const
{
	for_every(button, _systemButtonIDs)
	{
		if (*button >= 0 && *button < GamepadButton::Max)
		{
			if (!buttons[*button] && prevButtons[*button])
			{
				return true;
			}
		}
	}
	return false;

}

Vector2 Gamepad::get_stick(GamepadStick::Type _stick)
{
	if (_stick == GamepadStick::Left)
	{
		return get_left_stick();
	}
	if (_stick == GamepadStick::Right)
	{
		return get_right_stick();
	}
	if (_stick == GamepadStick::DPad)
	{
		return get_dpad_stick();
	}
	return Vector2::zero;
}

Vector2 Gamepad::get_stick_raw(GamepadStick::Type _stick)
{
	if (_stick == GamepadStick::Left)
	{
		return get_left_stick_raw();
	}
	if (_stick == GamepadStick::Right)
	{
		return get_right_stick_raw();
	}
	if (_stick == GamepadStick::Right)
	{
		return get_dpad_stick_raw();
	}
	return Vector2::zero;
}

bool Gamepad::is_any_button_pressed() const
{
	bool const * button = buttons;
	for_count(int, i, GamepadButton::Max)
	{
		if (*button)
		{
			return true;
		}
		++button;
	}
	return false;
}

