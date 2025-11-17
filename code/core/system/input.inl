bool Input::is_key_pressed(Key::Type _key) const
{
	an_assert(s_current != nullptr);
	return s_current->keys[_key];
}

bool Input::is_key_pressed(Array<Key::Type> const & _keys, bool _allRequired) const
{
	if (_allRequired)
	{
		if (_keys.is_empty())
		{
			return false;
		}
		for_every(key, _keys)
		{
			if (! is_key_pressed(*key))
			{
				return false;
			}
		}
		return true;
	}
	else
	{
		for_every(key, _keys)
		{
			if (is_key_pressed(*key))
			{
				return true;
			}
		}
		return false;
	}
}

bool Input::has_key_been_pressed(Key::Type _key) const
{
	an_assert(s_current != nullptr);
	return s_current->keys[_key] && ! s_current->prevKeys[_key];
}

bool Input::has_key_been_pressed(Array<Key::Type> const & _keys) const
{
	for_every(key, _keys)
	{
		if (has_key_been_pressed(*key))
		{
			return true;
		}
	}
	return false;
}

bool Input::has_key_been_released(Key::Type _key) const
{
	an_assert(s_current != nullptr);
	return ! s_current->keys[_key] && s_current->prevKeys[_key];
}

bool Input::has_key_been_released(Array<Key::Type> const & _keys) const
{
	for_every(key, _keys)
	{
		if (has_key_been_released(*key))
		{
			return true;
		}
	}
	return false;
}

Vector2 Input::get_mouse_relative_location() const
{
	return s_current->lastMouseMotion;
}

bool Input::is_mouse_button_valid(int32 _button) const
{
	return _button >= 0 && _button < 3;
}
		
float Input::get_mouse_button_pressed_time(int32 _button) const
{
	an_assert(s_current != nullptr);
	if (! is_mouse_button_valid(_button)) { return 0.0f; }
	return s_current->mouseButtonPressedTimes[_button];
}

bool Input::is_mouse_button_pressed(int32 _button) const
{
	an_assert(s_current != nullptr);
	if (! is_mouse_button_valid(_button)) { return false; }
	return s_current->mouseButtons[_button];
}

bool Input::was_mouse_button_pressed(int32 _button) const
{
	an_assert(s_current != nullptr);
	if (! is_mouse_button_valid(_button)) { return false; }
	return s_current->prevMouseButtons[_button];
}

bool Input::has_mouse_button_been_pressed(int32 _button) const
{
	an_assert(s_current != nullptr);
	if (! is_mouse_button_valid(_button)) { return false; }
	return s_current->mouseButtons[_button] && ! s_current->prevMouseButtons[_button];
}

bool Input::has_mouse_button_been_released(int32 _button) const
{
	an_assert(s_current != nullptr);
	if (! is_mouse_button_valid(_button)) { return false; }
	return ! s_current->mouseButtons[_button] && s_current->prevMouseButtons[_button];
}
