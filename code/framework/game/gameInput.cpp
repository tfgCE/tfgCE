#include "gameInput.h"

#include "..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

GameInput::GameInput()
{
}

void GameInput::use(GameInputDefinition* _gid)
{
	definition = _gid;
}

void GameInput::use(Name const & _gidName)
{
	use(GameInputDefinitions::get_definition(_gidName));
}

void GameInput::use(GameInputIncludeVR::Flags _includeVR, bool _useNormalInputWithVRToo)
{
	useNormalInput = _useNormalInputWithVRToo || _includeVR == GameInputIncludeVR::None;
	includeVR = _includeVR;
}

Vector2 GameInput::get_stick(Name const & _name) const
{
	if (!isEnabled)
	{
		return Vector2::zero;
	}
	Vector2 result = Vector2::zero;
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(stick, gid->get_sticks())
		{
			if (stick->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				// keyboard first
				{
					Vector2 resultKeys = Vector2::zero;
					if (::System::Input::get()->is_key_pressed(stick->xMinusKeys))
					{
						resultKeys.x -= 1.0f;
					}
					if (::System::Input::get()->is_key_pressed(stick->xPlusKeys))
					{
						resultKeys.x += 1.0f;
					}
					if (::System::Input::get()->is_key_pressed(stick->yMinusKeys))
					{
						resultKeys.y -= 1.0f;
					}
					if (::System::Input::get()->is_key_pressed(stick->yPlusKeys))
					{
						resultKeys.y += 1.0f;
					}
					if (!resultKeys.is_zero() &&
						check_condition(stick->keysCondition.get()))
					{
						result += resultKeys;
					}
				}
				// gamepad now
				if (allGamepads)
				{
					for_every_ptr(gamepad, ::System::Input::get()->get_gamepads())
					{
						Vector2 stickValue = Vector2::zero;
						for_every(gamepadStick, stick->gamepadSticks)
						{
							stickValue += gamepad->get_stick(*gamepadStick);
						}
						// clamp stick values (they already have dead zone applied)
						stickValue.x = clamp(stickValue.x, -1.0f, 1.0f);
						stickValue.y = clamp(stickValue.y, -1.0f, 1.0f);

						result += stickValue;
					}
				}
				else
				{
					todo_important(TXT("implement_!"));
				}
			}
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto * vr = VR::IVR::get();
				auto const & vrControls = vr->get_controls();
				bool leftHandOk = (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE) && check_condition(stick->vrSticksCondition.get(), Hand::Left);;
				bool rightHandOk = (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE) && check_condition(stick->vrSticksCondition.get(), Hand::Right);
				Vector2 stickValue = Vector2::zero;
				if (leftHandOk && (includeVR & GameInputIncludeVR::Left) && vr->get_left_hand() != NONE)
				{
					stickValue += vrControls.hands[vr->get_left_hand()].get_stick(stick->vrSticks, stick->vrStickDeadZone);
				}
				if (rightHandOk && (includeVR & GameInputIncludeVR::Right) && vr->get_right_hand() != NONE)
				{
					stickValue += vrControls.hands[vr->get_right_hand()].get_stick(stick->vrSticks, stick->vrStickDeadZone);
				}
				// clamp stick values (they already have dead zone applied)
				stickValue.x = clamp(stickValue.x, -1.0f, 1.0f);
				stickValue.y = clamp(stickValue.y, -1.0f, 1.0f);

				result += stickValue;
			}
			// apply sensitivity
			result *= stick->sensitivity;
			// apply invertness
			result.x *= stick->invertX ? -1.0f : 1.0f;
			result.y *= stick->invertY ? -1.0f : 1.0f;
		}
	}
	// clamping should be done by game logic
	return result;
}

Vector2 GameInput::get_mouse(Name const & _name) const
{
	if (!isEnabled)
	{
		return Vector2::zero;
	}
	Vector2 result = Vector2::zero;
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(mouse, gid->get_mouses())
		{
			if (mouse->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				if (mouse->mouse)
				{
					Vector2 mouseMovement = ::System::Input::get()->get_mouse_relative_location();
					if (!mouse->useX)
					{
						mouseMovement.x = 0.0f;
					}
					if (!mouse->useY)
					{
						mouseMovement.y = 0.0f;
					}
					// apply sensitivity
					mouseMovement *= mouse->sensitivity;
					if (!mouseMovement.is_zero() &&
						check_condition(mouse->mouseCondition.get()))
					{
						result += mouseMovement;
					}
				}
			}
			// vr?
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto * vr = VR::IVR::get();
				auto const & vrControls = vr->get_controls();
				bool leftHandOk = (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE) && check_condition(mouse->vrMousesCondition.get(), Hand::Left);;
				bool rightHandOk = (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE) && check_condition(mouse->vrMousesCondition.get(), Hand::Right);
				Vector2 mouseValue = Vector2::zero;
				if (leftHandOk && (includeVR & GameInputIncludeVR::Left) && vr->get_left_hand() != NONE)
				{
					mouseValue += vrControls.hands[vr->get_left_hand()].get_mouse_relative_location(mouse->vrMouses);
				}
				if (rightHandOk && (includeVR & GameInputIncludeVR::Right) && vr->get_right_hand() != NONE)
				{
					mouseValue += vrControls.hands[vr->get_right_hand()].get_mouse_relative_location(mouse->vrMouses);
				}
				if (!mouse->useX)
				{
					mouseValue.x = 0.0f;
				}
				if (!mouse->useY)
				{
					mouseValue.y = 0.0f;
				}
				todo_note(TXT("hardcoded vr mouse scale"))
				mouseValue *= 18.0f;
				// apply senistivity
				mouseValue *= mouse->vrSensitivity;
				result += mouseValue;
			}

			// apply invertness
			result.x *= mouse->invertX ? -1.0f : 1.0f;
			result.y *= mouse->invertY ? -1.0f : 1.0f;
		}
	}
	return result;
}

bool GameInput::check_condition(TagCondition const * _condition, Optional<Hand::Type> const& _hand)
{
	if (!_condition)
	{
		return true;
	}
	return _condition->check(::System::Input::get()->get_usage_tags(_hand));
}

float GameInput::get_button_state(Name const & _name) const
{
	if (!isEnabled)
	{
		return 0.0f;
	}
	float buttonState = 0.0f;
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(button, gid->get_buttons())
		{
			if (button->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				// keyboard first
				if (!button->keys.is_empty() &&
					::System::Input::get()->is_key_pressed(button->keys, button->allRequired) &&
					check_condition(button->keysCondition.get()))
				{
					buttonState = 1.0f;
				}
				// mouse next
				if (button->mouseButtonIdx != NONE &&
					::System::Input::get()->is_mouse_button_pressed(button->mouseButtonIdx) &&
					check_condition(button->mouseCondition.get()))
				{
					buttonState = 1.0f;
				}
				// gamepad now
				if (allGamepads)
				{
					float newButtonState = 0.0f;
					for_every_ptr(gamepad, ::System::Input::get()->get_gamepads())
					{
						newButtonState = max(newButtonState, gamepad->get_button_state(button->gamepadButtons));
						newButtonState = max(newButtonState, gamepad->get_button_state(button->gamepadStickDirsAsButtons));
					}
					if (newButtonState > 0.0f)
					{
						an_assert(!button->allRequired, TXT("not implemented for non vr!"));
					}
					buttonState = max(buttonState, newButtonState);
				}
				else
				{
					todo_important(TXT("implement_!"));
				}
			}
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto * vr = VR::IVR::get();
				auto const & vrControls = vr->get_controls();
				bool leftHandOk = (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE) && check_condition(button->vrButtonsCondition.get(), Hand::Left);;
				bool rightHandOk = (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE) && check_condition(button->vrButtonsCondition.get(), Hand::Right);
				float newState = button->allRequired && !button->vrButtons .is_empty()? 1.0f : 0.0f;
				for_every(vrButton, button->vrButtons)
				{
					float buttonState = (leftHandOk? vrControls.hands[vr->get_left_hand()].get_button_state(*vrButton).get(false) : 0.0f) +
									    (rightHandOk? vrControls.hands[vr->get_right_hand()].get_button_state(*vrButton).get(false) : 0.0f);
					if (button->allRequired)
					{
						newState *= buttonState;
					}
					else
					{
						newState = max(newState, buttonState);
					}
				}
				buttonState = max(buttonState, newState);
			}
			else if (button->noVR)
			{
				buttonState = 1.0f;
			}
		}
	}
	return buttonState;
}

bool GameInput::is_button_available(Name const & _name) const
{
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(button, gid->get_buttons())
		{
			if (button->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				// keyboard first
				if (!button->keys.is_empty() &&
					check_condition(button->keysCondition.get()))
				{
					return true;
				}
				// mouse next
				if (button->mouseButtonIdx != NONE &&
					check_condition(button->mouseCondition.get()))
				{
					return true;
				}
				// gamepad now
				if (allGamepads &&
					! button->gamepadButtons.is_empty() &&
					!button->gamepadStickDirsAsButtons.is_empty())
				{
					if (! ::System::Input::get()->get_gamepads().is_empty())
					{
						return true;
					}
				}
			}
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto* vr = VR::IVR::get();
				auto const& vrControls = vr->get_controls();
				bool leftHandOk = (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE) && check_condition(button->vrButtonsCondition.get(), Hand::Left);;
				bool rightHandOk = (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE) && check_condition(button->vrButtonsCondition.get(), Hand::Right);
				if (leftHandOk || rightHandOk)
				{
					for_every(vrButton, button->vrButtons)
					{
						if ((leftHandOk && vrControls.hands[vr->get_left_hand()].is_button_available(*vrButton)) ||
							(rightHandOk && vrControls.hands[vr->get_right_hand()].is_button_available(*vrButton)))
						{
							return true;
						}
					}
				}
			}
		}
	}
	return false;
}

bool GameInput::has_button(Name const & _name) const
{
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(button, gid->get_buttons())
		{
			if (button->name != _name)
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

bool GameInput::is_button_pressed(Name const & _name) const
{
	if (!isEnabled)
	{
		return false;
	}
	bool isButtonPressed = false;
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(button, gid->get_buttons())
		{
			if (button->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				// keyboard first
				if (!button->keys.is_empty() &&
					::System::Input::get()->is_key_pressed(button->keys, button->allRequired) &&
					check_condition(button->keysCondition.get()))
				{
					isButtonPressed = true;
				}
				// mouse next
				if (button->mouseButtonIdx != NONE &&
					::System::Input::get()->is_mouse_button_pressed(button->mouseButtonIdx) &&
					check_condition(button->mouseCondition.get()))
				{
					isButtonPressed = true;
				}
				// gamepad now
				if (allGamepads)
				{
					for_every_ptr(gamepad, ::System::Input::get()->get_gamepads())
					{
						if (gamepad->is_button_pressed(button->gamepadButtons))
						{
							isButtonPressed = true;
						}

						if (gamepad->is_button_pressed(button->gamepadStickDirsAsButtons))
						{
							isButtonPressed = true;
						}
					}
				}
				else
				{
					todo_important(TXT("implement_!"));
				}
			}
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto * vr = VR::IVR::get();
				auto const & vrControls = vr->get_controls();
				bool leftHandOk = (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE) && check_condition(button->vrButtonsCondition.get(), Hand::Left);;
				bool rightHandOk = (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE) && check_condition(button->vrButtonsCondition.get(), Hand::Right);
				bool result = button->allRequired && !button->vrButtons.is_empty();
				for_every(vrButton, button->vrButtons)
				{
					bool buttonOk = (leftHandOk && vrControls.hands[vr->get_left_hand()].is_button_pressed(*vrButton).get(false)) ||
									(rightHandOk && vrControls.hands[vr->get_right_hand()].is_button_pressed(*vrButton).get(false));
					if (button->allRequired)
					{
						if (!buttonOk)
						{
							result = false;
							break;
						}
					}
					else
					{
						if (buttonOk)
						{
							result = true;
							break;
						}
					}
				}
				isButtonPressed |= result;
			}
			else if (button->noVR)
			{
				isButtonPressed = true;
			}
		}
	}
	return isButtonPressed;
}

bool GameInput::has_button_been_pressed(Name const & _name) const
{
	if (!isEnabled)
	{
		return false;
	}
	bool buttonWasPressed = false;
	bool buttonWasAlreadyPressed = false;
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(button, gid->get_buttons())
		{
			if (button->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				// keyboard first
				if (!button->keys.is_empty())
				{
					if (::System::Input::get()->has_key_been_pressed(button->keys) &&
						check_condition(button->keysCondition.get()))
					{
						buttonWasPressed = true;
					}
					else if (::System::Input::get()->is_key_pressed(button->keys) &&
						check_condition(button->keysCondition.get()))
					{
						buttonWasAlreadyPressed = true;
					}
				}
				if (button->mouseButtonIdx != NONE)
				{
					if (::System::Input::get()->has_mouse_button_been_pressed(button->mouseButtonIdx) &&
						check_condition(button->mouseCondition.get()))
					{
						buttonWasPressed = true;
					}
					else if (::System::Input::get()->is_mouse_button_pressed(button->mouseButtonIdx) &&
						check_condition(button->mouseCondition.get()))
					{
						buttonWasAlreadyPressed = true;
					}
				}
				// gamepad now
				if (allGamepads)
				{
					for_every_ptr(gamepad, ::System::Input::get()->get_gamepads())
					{
						if (gamepad->has_button_been_pressed(button->gamepadButtons))
						{
							buttonWasPressed = true;
						}
						else if (gamepad->is_button_pressed(button->gamepadButtons))
						{
							buttonWasAlreadyPressed = true;
						}

						if (gamepad->has_button_been_pressed(button->gamepadStickDirsAsButtons))
						{
							buttonWasPressed = true;
						}
						else if (gamepad->is_button_pressed(button->gamepadStickDirsAsButtons))
						{
							buttonWasAlreadyPressed = true;
						}
					}
				}
				else
				{
					todo_important(TXT("implement_!"));
				}
			}
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto * vr = VR::IVR::get();
				auto const & vrControls = vr->get_controls();
				if (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE)
				{
					if (check_condition(button->vrButtonsCondition.get(), Hand::Left))
					{
						if (vrControls.hands[vr->get_left_hand()].has_button_been_pressed(button->vrButtons))
						{
							buttonWasPressed = true;
						}
						else if (vrControls.hands[vr->get_left_hand()].is_button_pressed(button->vrButtons))
						{
							buttonWasAlreadyPressed = true;
						}
					}
				}
				if (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE)
				{
					if (check_condition(button->vrButtonsCondition.get(), Hand::Right))
					{
						if (vrControls.hands[vr->get_right_hand()].has_button_been_pressed(button->vrButtons))
						{
							buttonWasPressed = true;
						}
						else if (vrControls.hands[vr->get_right_hand()].is_button_pressed(button->vrButtons))
						{
							buttonWasAlreadyPressed = true;
						}
					}
				}
				// no noVR check - it is static
			}
		}
	}
	return buttonWasPressed && !buttonWasAlreadyPressed;
}

bool GameInput::has_button_been_released(Name const & _name) const
{
	if (!isEnabled)
	{
		return false;
	}
	bool buttonWasRelased = false;
	bool buttonIsStillPressed = false;
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(button, gid->get_buttons())
		{
			if (button->name != _name)
			{
				continue;
			}
			if (includeVR == GameInputIncludeVR::None || useNormalInput)
			{
				// keyboard first
				if (!button->keys.is_empty())
				{
					if (::System::Input::get()->has_key_been_released(button->keys) &&
						check_condition(button->keysCondition.get()))
					{
						buttonWasRelased = true;
					}
					if (::System::Input::get()->is_key_pressed(button->keys) &&
						check_condition(button->keysCondition.get()))
					{
						buttonIsStillPressed = true;
					}
				}
				if (button->mouseButtonIdx != NONE)
				{
					if (::System::Input::get()->has_mouse_button_been_released(button->mouseButtonIdx) &&
						check_condition(button->mouseCondition.get()))
					{
						buttonWasRelased = true;
					}
					else if (::System::Input::get()->is_mouse_button_pressed(button->mouseButtonIdx) &&
						check_condition(button->mouseCondition.get()))
					{
						buttonIsStillPressed = true;
					}
				}
				// gamepad now
				if (allGamepads)
				{
					for_every_ptr(gamepad, ::System::Input::get()->get_gamepads())
					{
						if (gamepad->has_button_been_released(button->gamepadButtons))
						{
							buttonWasRelased = true;
						}
						if (gamepad->is_button_pressed(button->gamepadButtons))
						{
							buttonIsStillPressed = true;
						}
						if (gamepad->has_button_been_released(button->gamepadStickDirsAsButtons))
						{
							buttonWasRelased = true;
						}
						if (gamepad->is_button_pressed(button->gamepadStickDirsAsButtons))
						{
							buttonIsStillPressed = true;
						}
					}
				}
				else
				{
					todo_important(TXT("implement_!"));
				}
			}
			if (includeVR != 0 && VR::IVR::can_be_used())
			{
				auto * vr = VR::IVR::get();
				auto const & vrControls = vr->get_controls();
				if (includeVR & GameInputIncludeVR::Left && vr->get_left_hand() != NONE)
				{
					if (check_condition(button->vrButtonsCondition.get(), Hand::Left))
					{
						if (vrControls.hands[vr->get_left_hand()].has_button_been_released(button->vrButtons))
						{
							buttonWasRelased = true;
						}
						else if (vrControls.hands[vr->get_left_hand()].is_button_pressed(button->vrButtons))
						{
							buttonIsStillPressed = true;
						}
					}
				}
				if (includeVR & GameInputIncludeVR::Right && vr->get_right_hand() != NONE)
				{
					if (check_condition(button->vrButtonsCondition.get(), Hand::Right))
					{
						if (vrControls.hands[vr->get_right_hand()].has_button_been_released(button->vrButtons))
						{
							buttonWasRelased = true;
						}
						else if (vrControls.hands[vr->get_right_hand()].is_button_pressed(button->vrButtons))
						{
							buttonIsStillPressed = true;
						}
					}
				}
				// no noVR check - it is static
			}
		}
	}
	return buttonWasRelased && !buttonIsStillPressed;
}

bool GameInput::has_stick(Name const & _name) const
{
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(stick, gid->get_sticks())
		{
			if (stick->name != _name)
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

bool GameInput::has_mouse(Name const & _name) const
{
	if (GameInputDefinition const * gid = definition.get())
	{
		for_every(mouse, gid->get_mouses())
		{
			if (mouse->name != _name)
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

