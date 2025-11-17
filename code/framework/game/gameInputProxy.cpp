#include "gameInputProxy.h"

using namespace Framework;

#define PRESS_TRESHOLD 0.5f

GameInputProxy::GameInputProxy()
{
}

void GameInputProxy::advance()
{
	for_every(stick, sticks)
	{
		stick->prevValue = stick->value;
		stick->value = Vector2::zero;
	}
	for_every(mouse, mouses)
	{
		mouse->movement = Vector2::zero;
	}
	for_every(button, buttons)
	{
		button->prevValue = button->value;
		button->value = 0.0f;
	}
}

void GameInputProxy::set_stick(Name const & _name, Vector2 const & _v)
{
	for_every(stick, sticks)
	{
		if (stick->name == _name)
		{
			stick->value = _v;
			return;
		}
	}

	Stick stick;
	stick.name = _name;
	stick.prevValue = stick.value = _v;
	sticks.push_back(stick);
}

void GameInputProxy::set_mouse(Name const & _name, Vector2 const & _v)
{
	for_every(mouse, mouses)
	{
		if (mouse->name == _name)
		{
			mouse->movement = _v;
			return;
		}
	}

	Mouse mouse;
	mouse.name = _name;
	mouse.movement = _v;
	mouses.push_back(mouse);
}

void GameInputProxy::set_button_state(Name const & _name, float const & _v)
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			button->value = _v;
			return;
		}
	}

	Button button;
	button.name = _name;
	button.prevValue = button.value = _v;
	buttons.push_back(button);
}

Vector2 GameInputProxy::get_stick(Name const & _name) const
{
	for_every(stick, sticks)
	{
		if (stick->name == _name)
		{
			return stick->value;
		}
	}

	return Vector2::zero;
}

Vector2 GameInputProxy::get_mouse(Name const & _name) const
{
	for_every(mouse, mouses)
	{
		if (mouse->name == _name)
		{
			return mouse->movement;
		}
	}

	return Vector2::zero;
}

float GameInputProxy::get_button_state(Name const & _name) const
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			return button->value;
		}
	}
	return 0.0f;
}

bool GameInputProxy::has_button(Name const & _name) const
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			return true;
		}
	}
	return false;
}

bool GameInputProxy::is_button_pressed(Name const & _name) const
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			return button->value >= PRESS_TRESHOLD;
		}
	}
	return 0.0f;
}

bool GameInputProxy::has_button_been_pressed(Name const & _name) const
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			return button->value >= PRESS_TRESHOLD && button->prevValue < PRESS_TRESHOLD;
		}
	}
	return 0.0f;
}

bool GameInputProxy::has_button_been_released(Name const & _name) const
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			return button->value < PRESS_TRESHOLD && button->prevValue >= PRESS_TRESHOLD;
		}
	}
	return 0.0f;
}

bool GameInputProxy::has_stick(Name const & _name) const
{
	for_every(stick, sticks)
	{
		if (stick->name == _name)
		{
			return true;
		}
	}
	return false;
}

void GameInputProxy::clear()
{
	sticks.clear();
	mouses.clear();
	buttons.clear();
}

void GameInputProxy::remove_stick(Name const & _name)
{
	for_every(stick, sticks)
	{
		if (stick->name == _name)
		{
			sticks.remove_fast_at(for_everys_index(stick));
			return;
		}
	}
}

bool GameInputProxy::has_mouse(Name const & _name) const
{
	for_every(mouse, mouses)
	{
		if (mouse->name == _name)
		{
			return true;
		}
	}
	return false;
}

void GameInputProxy::remove_mouse(Name const & _name)
{
	for_every(mouse, mouses)
	{
		if (mouse->name == _name)
		{
			mouses.remove_fast_at(for_everys_index(mouse));
			return;
		}
	}
}

void GameInputProxy::remove_button(Name const & _name)
{
	for_every(button, buttons)
	{
		if (button->name == _name)
		{
			buttons.remove_fast_at(for_everys_index(button));
			return;
		}
	}
}

//

bool GameInputProxyDefinition::Control::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	from = _node->get_name_attribute(TXT("from"), from);
	to = _node->get_name_attribute(TXT("to"), to);

	if (!from.is_valid())
	{
		error_loading_xml(_node, TXT("no \"from\" parameter for game input proxy definition"));
		result = false;
	}
	if (!to.is_valid())
	{
		error_loading_xml(_node, TXT("no \"to\" parameter for game input proxy definition"));
		result = false;
	}

	return result;
}

//

bool GameInputProxyDefinition::Mouse::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	mul = _node->get_float_attribute(TXT("mul"), mul);

	return result;
}

//

bool GameInputProxyDefinition::Stick::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	Name modeName = _node->get_name_attribute(TXT("mode"));
	if (modeName == TXT("map") ||
		modeName == TXT("mapToRange"))
	{
		mode = Mode_MapToRange;
	}

	range.load_from_xml(_node, TXT("range"));

	ifButton = _node->get_name_attribute(TXT("ifButton"));

	return result;
}

//

bool GameInputProxyDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	for_every(node, _node->children_named(TXT("stick")))
	{
		sticks.grow_size(1);
		result &= sticks.get_last().load_from_xml(node);
	}
	for_every(node, _node->children_named(TXT("mouse")))
	{
		mouses.grow_size(1);
		result &= mouses.get_last().load_from_xml(node);
	}
	for_every(node, _node->children_named(TXT("button")))
	{
		buttons.grow_size(1);
		result &= buttons.get_last().load_from_xml(node);
	}

	return result;
}

void GameInputProxyDefinition::process(REF_ GameInputProxy & _proxy, IGameInput const & _from) const
{
	for_every(stick, sticks)
	{
		bool clear = true;
		if (_from.has_stick(stick->from))
		{
			if (!stick->ifButton.is_valid() ||
				_from.is_button_pressed(stick->ifButton))
			{
				Vector2 v = _from.get_stick(stick->from);
				if (stick->mode == Stick::Mode_MapToRange)
				{
					v.x = stick->range.get_at((v.x + 1.0f) * 0.5f);
					v.y = stick->range.get_at((v.y + 1.0f) * 0.5f);
				}
				clear = false;
				_proxy.set_stick(stick->to, v);
			}
		}
		if (clear)
		{
			_proxy.remove_stick(stick->to);
		}
	}
	for_every(mouse, mouses)
	{
		if (_from.has_mouse(mouse->from))
		{
			_proxy.set_mouse(mouse->to, _from.get_mouse(mouse->from) * mouse->mul);
		}
	}
	for_every(button, buttons)
	{
		if (_from.has_button(button->from))
		{
			_proxy.set_button_state(button->to, _from.get_button_state(button->from));
		}
	}
}