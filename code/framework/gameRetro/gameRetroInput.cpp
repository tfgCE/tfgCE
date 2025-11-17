#include "gameRetroInput.h"

#include "gameRetroConfig.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define LOC_STR_NAME_TEMPLATE__INPUT_ACTION TXT("input action %i name")
#define LOC_STR_NAME_TEMPLATE__GAMEPAD_INPUT_DEFINITION TXT("gamepad input; %S")

//

using namespace Framework;

//

bool GameRetroInput::Config::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("inputDefinition")))
	{
		result &= inputDefinition.load_from_xml(node);
	}

	for_every(node, _node->children_named(TXT("gamepadInputDefinition")))
	{
		GamepadInputDefinition gid;
		if (gid.load_from_xml(node))
		{
			bool found = false;
			for_every(g, gamepadInputDefinitions)
			{
				if (g->id == gid.id)
				{
					*g = gid;
					found = true;
					break;
				}
			}
			if (!found)
			{
				gamepadInputDefinitions.push_back(gid);
			}
		}
	}

	for_every(node, _node->children_named(TXT("playerInputDefinition")))
	{
		PlayerInputDefinition pid;
		if (auto* attr = node->get_attribute(TXT("index")))
		{
			int index = attr->get_as_int();
			if (!playerInputDefinitions.is_index_valid(index) && index >= 0)
			{
				playerInputDefinitions.set_size(index + 1);
			}
			if (playerInputDefinitions.is_index_valid(index))
			{
				auto& pid = playerInputDefinitions[index];
				result &= pid.load_from_xml(node);
			}
		}
		else
		{
			error_loading_xml(node, TXT("no index provided"));
			result = false;
		}
	}

	return result;
}

void GameRetroInput::Config::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	// always save, doesn't matter if user or not, the actual bits will differentiate

	auto* node = _container->add_node(TXT("retroInput"));

	inputDefinition.save_to_xml(node, _userSpecific);

	for_every(gid, gamepadInputDefinitions)
	{
		gid->save_to_xml(_container, _userSpecific);
	}

	for_every(pid, playerInputDefinitions)
	{
		pid->save_to_xml(_container, _userSpecific, for_everys_index(pid));
	}
}

//--

bool GameRetroInput::Config::InputDefinition::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("action")))
	{
		if (auto* attr = node->get_attribute(TXT("index")))
		{
			int index = attr->get_as_int();
			if (actions.is_index_valid(index))
			{
				auto& a = actions[index];
				a.id = node->get_name_attribute(TXT("id"), a.id);
				a.name.load_from_xml_child(node, TXT("name"), String::printf(LOC_STR_NAME_TEMPLATE__INPUT_ACTION, index).to_char());
			}
			else
			{
				error_loading_xml(node, TXT("invalid index"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("no index provided"));
			result = false;
		}
	}

	return result;
}

void GameRetroInput::Config::InputDefinition::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	// user config does not require this
	if (!_userSpecific)
	{
		auto* node = _container->add_node(TXT("inputDefinition"));

		for_every(action, actions)
		{
			if (action->id.is_valid() || action->name.is_valid())
			{
				auto* actionNode = node->add_node(TXT("action"));
				actionNode->set_int_attribute(TXT("index"), for_everys_index(action));
				if (action->id.is_valid())
				{
					actionNode->set_attribute(TXT("id"), action->id.to_char());
				}
				if (action->name.is_valid())
				{
					actionNode->set_string_to_child(TXT("name"), action->name.get());
				}
			}
		}
	}
}

//--

bool GameRetroInput::Config::KeyboardInputDefinition::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("stick")))
	{
		result &= stick.load_from_xml(node);
	}

	result &= action.load_from_xml(_node);

	return result;
}

void GameRetroInput::Config::KeyboardInputDefinition::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	// always save, doesn't matter if user or not

	auto* node = _container->add_node(TXT("keyboardInputDefinition"));

	stick.save_to_xml(node, _userSpecific);
	action.save_to_xml(node, _userSpecific);
}

//--

String GameRetroInput::Config::KeyboardInputDefinition::KeySet::to_string() const
{
	String result;

	for_every(key, keys)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += System::Key::to_char(*key);
	}

	return result;
}

bool GameRetroInput::Config::KeyboardInputDefinition::KeySet::parse(String const& _string)
{
	bool result = true;

	keys.clear();
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		System::Key::Type key = System::Key::parse(token->trim(), System::Key::None);
		if (key != System::Key::None)
		{
			if (keys.has_place_left())
			{
				keys.push_back(key);
			}
			else
			{
				error(TXT("can't load \"%S\", too many keys"), _string.to_char());
				result = false;
			}
		}
		else
		{
			error(TXT("key \"%S\" not recognised"), token->trim().to_char());
			result = false;
		}
	}

	return true;
}

bool GameRetroInput::Config::KeyboardInputDefinition::KeySet::is_pressed(::System::Input const* _input) const
{
	for_every(key, keys)
	{
		if (_input->is_key_pressed(*key))
		{
			return true;
		}
	}
	return false;
}

//--

bool GameRetroInput::Config::KeyboardInputDefinition::Stick::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	if (auto* attr = _node->get_attribute(TXT("left")))
	{
		result &= left.parse(attr->get_as_string());
	}
	if (auto* attr = _node->get_attribute(TXT("right")))
	{
		result &= right.parse(attr->get_as_string());
	}
	if (auto* attr = _node->get_attribute(TXT("up")))
	{
		result &= up.parse(attr->get_as_string());
	}
	if (auto* attr = _node->get_attribute(TXT("down")))
	{
		result &= down.parse(attr->get_as_string());
	}

	return result;
}

void GameRetroInput::Config::KeyboardInputDefinition::Stick::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	// always save, doesn't matter if user or not

	auto* node = _container->add_node(TXT("stick"));

	node->set_attribute(TXT("left"), left.to_string());
	node->set_attribute(TXT("right"), right.to_string());
	node->set_attribute(TXT("up"), up.to_string());
	node->set_attribute(TXT("down"), down.to_string());
}

//--

bool GameRetroInput::Config::KeyboardInputDefinition::Action::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("action")))
	{
		if (auto* attr = node->get_attribute(TXT("index")))
		{
			int index = attr->get_as_int();
			if (actions.is_index_valid(index))
			{
				auto& a = actions[index];
				a.keys.clear();
				if (auto* attr = node->get_attribute(TXT("key")))
				{
					result &= a.parse(attr->get_as_string());
				}
				a.forceClear = ! a.keys.is_empty();
			}
			else
			{
				error_loading_xml(node, TXT("invalid index"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("no index provided"));
			result = false;
		}
	}

	return result;
}

void GameRetroInput::Config::KeyboardInputDefinition::Action::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	// always save, doesn't matter if user or not

	for_every(action, actions)
	{
		if (! action->keys.is_empty() || action->forceClear)
		{
			auto* actionNode = _container->add_node(TXT("action"));
			actionNode->set_int_attribute(TXT("index"), for_everys_index(action));
			if (! action->keys.is_empty())
			{
				actionNode->set_attribute(TXT("key"), action->to_string().to_char());
			}
		}
	}
}

//--

bool GameRetroInput::Config::GamepadInputDefinition::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	id = _node->get_name_attribute(TXT("id"), id);
	error_loading_xml_on_assert(id.is_valid(), result, _node, TXT("no \"id\" provided for gamepad input definition"));

	name.load_from_xml_child(_node, TXT("name"), String::printf(LOC_STR_NAME_TEMPLATE__GAMEPAD_INPUT_DEFINITION, id.to_char()).to_char());

	useLeftStick = false;
	useRightStick = false;
	useLeftStick = _node->get_bool_attribute_or_from_child_presence(TXT("useLeftStick"), useLeftStick);
	useRightStick = _node->get_bool_attribute_or_from_child_presence(TXT("useRightStick"), useRightStick);
	useLeftStick = _node->get_bool_attribute_or_from_child_presence(TXT("useBothSticks"), useLeftStick);
	useRightStick = _node->get_bool_attribute_or_from_child_presence(TXT("useBothSticks"), useRightStick);

	useDPadStick = false;
	useDPadStick = _node->get_bool_attribute_or_from_child_presence(TXT("useDPadStick"), useDPadStick);

	if (!useLeftStick && !useRightStick && !useDPadStick)
	{
		useLeftStick = true;
		useRightStick = true;
	}

	result &= action.load_from_xml(_node);

	return result;
}

void GameRetroInput::Config::GamepadInputDefinition::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	// user does not save gamepad input definition
	if (!_userSpecific)
	{
		auto* node = _container->add_node(TXT("gamepadInputDefinition"));

		node->set_attribute(TXT("id"), id.to_char());
		node->set_string_to_child(TXT("name"), name.get());

		if (useLeftStick && useRightStick)
		{
			node->add_node(TXT("useBothSticks"));
		}
		else
		{
			if (useLeftStick) node->add_node(TXT("useLeftStick"));
			if (useRightStick) node->add_node(TXT("useRightStick"));
		}

		if (useDPadStick)
		{
			node->add_node(TXT("useDPadStick"));
		}
	
		action.save_to_xml(node, _userSpecific);
	}
}

//--

String GameRetroInput::Config::GamepadInputDefinition::ButtonSet::to_string() const
{
	String result;

	for_every(button, buttons)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += System::GamepadButton::to_char(*button);
	}

	return result;
}

bool GameRetroInput::Config::GamepadInputDefinition::ButtonSet::parse(String const& _string)
{
	bool result = true;

	buttons.clear();
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		System::GamepadButton::Type button = System::GamepadButton::parse(token->trim(), System::GamepadButton::None);
		if (button != System::GamepadButton::None)
		{
			if (buttons.has_place_left())
			{
				buttons.push_back(button);
			}
			else
			{
				error(TXT("can't load \"%S\", too many buttons"), _string.to_char());
				result = false;
			}
		}
		else
		{
			error(TXT("button \"%S\" not recognised"), token->trim().to_char());
			result = false;
		}
	}

	return true;
}

bool GameRetroInput::Config::GamepadInputDefinition::ButtonSet::is_pressed(::System::Gamepad const* _gamepad) const
{
	for_every(button, buttons)
	{
		if (_gamepad->is_button_pressed(*button))
		{
			return true;
		}
	}
	return false;
}

//--

bool GameRetroInput::Config::GamepadInputDefinition::Action::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("action")))
	{
		if (auto* attr = node->get_attribute(TXT("index")))
		{
			int index = attr->get_as_int();
			if (actions.is_index_valid(index))
			{
				auto& a = actions[index];
				a.buttons.clear();
				if (auto* attr = node->get_attribute(TXT("button")))
				{
					result &= a.parse(attr->get_as_string());
				}
				a.forceClear = ! a.buttons.is_empty();
			}
			else
			{
				error_loading_xml(node, TXT("invalid index"));
				result = false;
			}
		}
		else
		{
			error_loading_xml(node, TXT("no index provided"));
			result = false;
		}
	}

	return result;
}

void GameRetroInput::Config::GamepadInputDefinition::Action::save_to_xml(IO::XML::Node* _container, bool _userSpecific) const
{
	for_every(action, actions)
	{
		if (! action->buttons.is_empty() || action->forceClear)
		{
			auto* actionNode = _container->add_node(TXT("action"));
			actionNode->set_int_attribute(TXT("index"), for_everys_index(action));
			if (! action->buttons.is_empty())
			{
				actionNode->set_attribute(TXT("button"), action->to_string().to_char());
			}
		}
	}
}

//--

bool GameRetroInput::Config::PlayerInputDefinition::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	gamepadInputDefinitionId = Name::invalid();

	for_every(node, _node->children_named(TXT("gamepadInputDefinition")))
	{
		gamepadInputDefinitionId = node->get_name_attribute(TXT("id"), gamepadInputDefinitionId);
	}

	for_every(node, _node->children_named(TXT("keyboardInputDefinition")))
	{
		result &= keyboardInputDefinition.load_from_xml(node);
	}

	return result;
}

void GameRetroInput::Config::PlayerInputDefinition::save_to_xml(IO::XML::Node* _container, bool _userSpecific, int _index) const
{
	// always save, doesn't matter if user or not

	auto* node = _container->add_node(TXT("playerInputDefinition"));

	if (gamepadInputDefinitionId.is_valid())
	{
		auto* n = node->add_node(TXT("gamepadInputDefinition"));
		n->set_attribute(TXT("id"), gamepadInputDefinitionId.to_char());
	}

	keyboardInputDefinition.save_to_xml(node, _userSpecific);
}

//--

GameRetroInput::State::State()
{
	playerInputStates.set_size(GameRetroInput::PLAYER_COUNT);
	keyboard.keys.set_size(System::Key::NUM);
}

void GameRetroInput::State::update_start()
{
	for_every(pis, playerInputStates)
	{
		pis->stick = Vector2::zero;
		pis->stickDir = VectorInt2::zero;
		for_every(a, pis->actions)
		{
			a->resetLastPressTimeOnPress = ! a->pressed;

			a->wasPressed = a->pressed;
			a->pressed = false;
		}
	}

	for_every(k, keyboard.keys)
	{
		k->wasPressed = k->pressed;
		k->pressed = false;
	}
}

void GameRetroInput::State::read_input(float _deltaTime)
{
	auto const* grc = GameRetroConfig::get_as<GameRetroConfig>();
	an_assert(grc);
	auto& gri = grc->get_input();

	if (auto* input = ::System::Input::get())
	{
		{
			int keyIdx = 0;
			for_every(k, keyboard.keys)
			{
				k->pressed = input->is_key_pressed((System::Key::Type)keyIdx);
				++keyIdx;
			}
		}

		ArrayStatic<bool, GameRetroInput::ACTION_COUNT> actionPressed;
		actionPressed.set_size(GameRetroInput::ACTION_COUNT);

		auto* confPlayer = gri.playerInputDefinitions.begin();
		auto* statPlayer = playerInputStates.begin();
		an_assert(gri.playerInputDefinitions.get_size() == GameRetroInput::PLAYER_COUNT);
		an_assert(playerInputStates.get_size() == GameRetroInput::PLAYER_COUNT);
		for(int playerIdx = 0; playerIdx < GameRetroInput::PLAYER_COUNT; ++ playerIdx, ++confPlayer, ++statPlayer)
		{
			memory_zero(actionPressed.get_data(), actionPressed.get_data_size());

			// read from keyboard
			{
				auto& confKeyboard = confPlayer->keyboardInputDefinition;

				// stick
				{
					if (confKeyboard.stick.left.is_pressed(input)) { statPlayer->stick.x = -1.0f; }
					if (confKeyboard.stick.right.is_pressed(input)) { statPlayer->stick.x = 1.0f; }
					if (confKeyboard.stick.down.is_pressed(input)) { statPlayer->stick.y = -1.0f; }
					if (confKeyboard.stick.up.is_pressed(input)) { statPlayer->stick.y = 1.0f; }
				}

				// actions
				{
					auto* confA = confKeyboard.action.actions.begin();
					auto* statA = statPlayer->actions.begin();
					auto* presA = actionPressed.begin();
					an_assert(statPlayer->actions.get_size() == GameRetroInput::ACTION_COUNT);
					an_assert(confKeyboard.action.actions.get_size() == GameRetroInput::ACTION_COUNT);
					for (int actionIdx = 0; actionIdx < GameRetroInput::ACTION_COUNT; ++actionIdx, ++confA, ++statA, ++presA)
					{
						if (confA->is_pressed(input))
						{
							*presA = true;
							statA->pressed = true;
						}
					}
				}
			}

			// read from gamepad
			{
				if (!statPlayer->gamepadSystemId.is_set())
				{
					// find a gamepad that has a button pressed and is not used yet
					Optional<int> foundGamepadSystemId = find_active_unasigned_gamepad(input);
					if (foundGamepadSystemId.is_set())
					{
						statPlayer->gamepadSystemId = foundGamepadSystemId.get();
						statPlayer->waitForNoGamepadControls = true;
					}
				}
				else if (auto* gamepad = input->get_gamepad_by_system_id(statPlayer->gamepadSystemId.get()))
				{
					if (statPlayer->waitForNoGamepadControls)
					{
						if (!is_gamepad_active(gamepad))
						{
							statPlayer->waitForNoGamepadControls = false;
						}
					}
					else
					{
						// update index
						if (statPlayer->gamepadInputDefinitionIdx == NONE ||
							statPlayer->gamepadInputDefinitionIdxCachedForId != confPlayer->gamepadInputDefinitionId)
						{
							statPlayer->gamepadInputDefinitionIdxCachedForId = confPlayer->gamepadInputDefinitionId;
							statPlayer->gamepadInputDefinitionIdx = 0;
							for_every(gid, gri.gamepadInputDefinitions)
							{
								if (confPlayer->gamepadInputDefinitionId == gid->id)
								{
									statPlayer->gamepadInputDefinitionIdx = for_everys_index(gid);
								}
							}
						}
						if (gri.gamepadInputDefinitions.is_index_valid(statPlayer->gamepadInputDefinitionIdx))
						{
							auto& confGamepad = gri.gamepadInputDefinitions[statPlayer->gamepadInputDefinitionIdx];

							// stick
							{
								if (confGamepad.useLeftStick) statPlayer->stick.maximise_off_zero(gamepad->get_left_stick());
								if (confGamepad.useRightStick) statPlayer->stick.maximise_off_zero(gamepad->get_right_stick());
								if (confGamepad.useDPadStick) statPlayer->stick.maximise_off_zero(gamepad->get_dpad_stick());
							}

							// actions
							{
								auto* confA = confGamepad.action.actions.begin();
								auto* statA = statPlayer->actions.begin();
								auto* presA = actionPressed.begin();
								an_assert(statPlayer->actions.get_size() == GameRetroInput::ACTION_COUNT);
								an_assert(confGamepad.action.actions.get_size() == GameRetroInput::ACTION_COUNT);
								for (int actionIdx = 0; actionIdx < GameRetroInput::ACTION_COUNT; ++actionIdx, ++confA, ++statA, ++presA)
								{
									if (confA->is_pressed(gamepad))
									{
										*presA = true;
										statA->pressed = true;
									}
								}
							}
						}
					}
				}
				else
				{
					statPlayer->forget_gamepad();
				}
			}

			// update lastPressTime
			{
				auto* statA = statPlayer->actions.begin();
				auto* presA = actionPressed.begin();
				an_assert(statPlayer->actions.get_size() == GameRetroInput::ACTION_COUNT);
				for (int actionIdx = 0; actionIdx < GameRetroInput::ACTION_COUNT; ++actionIdx, ++statA, ++presA)
				{
					if (*presA)
					{
						if (statA->resetLastPressTimeOnPress)
						{
							statA->lastPressTime = _deltaTime;
							statA->resetLastPressTimeOnPress = false;
						}
						else
						{
							statA->lastPressTime += _deltaTime;
						}
					}
				}
			}
		}
	}
}

void GameRetroInput::State::update_end()
{
	auto const* grc = GameRetroConfig::get_as<GameRetroConfig>();
	an_assert(grc);
	auto& gri = grc->get_input();
	
	float stickDirDeadZone = max(0.0001f, gri.stickDirDeadZone);

	for_every(pis, playerInputStates)
	{
		pis->stickDir = VectorInt2::zero;
		if (pis->stick.x >=  stickDirDeadZone) pis->stickDir.x =  1;
		if (pis->stick.x <= -stickDirDeadZone) pis->stickDir.x = -1;
		if (pis->stick.y >=  stickDirDeadZone) pis->stickDir.y =  1;
		if (pis->stick.y <= -stickDirDeadZone) pis->stickDir.y = -1;
	}
}

bool GameRetroInput::State::is_gamepad_used(int _gamepadSystemId) const
{
	for_every(pis, playerInputStates)
	{
		if (pis->gamepadSystemId.is_set() &&
			pis->gamepadSystemId.get() == _gamepadSystemId)
		{
			return true;
		}
	}
	return false;
}

bool GameRetroInput::State::is_gamepad_active(::System::Gamepad const* gamepad) const
{
	if (gamepad)
	{
#ifdef AN_STANDARD_INPUT
		if (gamepad->is_button_pressed(System::GamepadButton::A)
		 || gamepad->is_button_pressed(System::GamepadButton::B)
		 || gamepad->is_button_pressed(System::GamepadButton::X)
		 || gamepad->is_button_pressed(System::GamepadButton::Y)
		 || gamepad->is_button_pressed(System::GamepadButton::LeftStick)
		 || gamepad->is_button_pressed(System::GamepadButton::RightStick)
		 || gamepad->is_button_pressed(System::GamepadButton::LeftBumper)
		 || gamepad->is_button_pressed(System::GamepadButton::RightBumper)
		 || gamepad->is_button_pressed(System::GamepadButton::LeftTrigger)
		 || gamepad->is_button_pressed(System::GamepadButton::RightTrigger)
		 || gamepad->is_button_pressed(System::GamepadButton::DPadUp)
		 || gamepad->is_button_pressed(System::GamepadButton::DPadUp)
		 || gamepad->is_button_pressed(System::GamepadButton::DPadDown)
		 || gamepad->is_button_pressed(System::GamepadButton::DPadLeft)
		 || gamepad->is_button_pressed(System::GamepadButton::DPadRight)
		 || gamepad->is_button_pressed(System::GamepadButton::Back)
		 || gamepad->is_button_pressed(System::GamepadButton::Start)
		 || gamepad->is_button_pressed(System::GamepadButton::LStickUp)
		 || gamepad->is_button_pressed(System::GamepadButton::LStickDown)
		 || gamepad->is_button_pressed(System::GamepadButton::LStickLeft)
		 || gamepad->is_button_pressed(System::GamepadButton::LStickRight)
		 || gamepad->is_button_pressed(System::GamepadButton::RStickUp)
		 || gamepad->is_button_pressed(System::GamepadButton::RStickDown)
		 || gamepad->is_button_pressed(System::GamepadButton::RStickLeft)
		 || gamepad->is_button_pressed(System::GamepadButton::RStickRight))
		{
			return true;
		}
#endif
	}
	return false;
}

Optional<int> GameRetroInput::State::find_active_unasigned_gamepad(::System::Input const* _input) const
{
	for_every_ptr(gamepad, _input->get_gamepads())
	{
		int gamepadSystemId = gamepad->get_system_id();
		if (!is_gamepad_used(gamepadSystemId))
		{
			if (is_gamepad_active(gamepad))
			{
				return gamepadSystemId;
			}
		}
	}
	return NP;
}
