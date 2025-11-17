#include "gameInputDefinition.h"

#include "bullshotSystem.h"
#include "game.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\types\names.h"
#include "..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsNoDevelopmentInput, TXT("no development input"));

//

GameInputDefinitions* GameInputDefinitions::s_definitions = nullptr;

//

static bool load_condition(IO::XML::Node const * _node, REF_ RefCountObjectPtr<TagCondition> & condition, tchar const * conditionName)
{
	bool result = true;
	if (auto* attr = _node->get_attribute(conditionName))
	{
		if (!condition.is_set())
		{
			condition = new TagCondition();
		}
		result &= condition->load_from_string(attr->get_as_string());
	}
	return result;
}

static bool save_condition(IO::XML::Node * _node, RefCountObjectPtr<TagCondition> const & condition, tchar const * conditionName)
{
	bool result = true;
	if (condition.is_set() && ! condition->is_empty())
	{
		_node->set_attribute(conditionName, condition->to_string());
	}
	return result;
}

static bool load_key_definition(Array<::System::Key::Type> & _keys, String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			::System::Key::Type key = ::System::Key::parse(token->trim(), ::System::Key::Type::None);
			if (key != ::System::Key::Type::None)
			{
				_keys.push_back_unique(key);
			}
			else
			{
#ifdef AN_STANDARD_INPUT
				error(TXT("invalid key \"%S\""), token->trim().to_char());
#endif
			}
		}
	}
	return result;
}

static String save_key_definition(Array<::System::Key::Type> const & _keys)
{
	String result;
	for_every(key, _keys)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += ::System::Key::to_char(*key);
	}
	return result.replace(String(TXT(" ,")), String::comma()).trim();
}

static bool load_stick_definition(Array<::System::GamepadStick::Type> & _sticks, String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			::System::GamepadStick::Type stick = ::System::GamepadStick::parse(token->trim(), ::System::GamepadStick::Type::None);
			if (stick != ::System::GamepadStick::Type::None)
			{
				_sticks.push_back_unique(stick);
			}
			else
			{
#ifdef AN_STANDARD_INPUT
				error(TXT("invalid gamepad stick \"%S\""), token->trim().to_char());
#endif
			}
		}
	}
	return result;
}

static String save_stick_definition(Array<::System::GamepadStick::Type> const & _sticks)
{
	String result;
	for_every(stick, _sticks)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += ::System::GamepadStick::to_char(*stick);
	}
	return result.replace(String(TXT(" ,")), String::comma()).trim();
}

static bool load_stick_definition(Array<::VR::Input::Stick::WithSource> & _sticks, String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			::VR::Input::Stick::WithSource stick = ::VR::Input::Stick::WithSource::parse(token->trim(), ::VR::Input::Stick::Type::None);
			if (stick.type != ::VR::Input::Stick::Type::None)
			{
				_sticks.push_back_unique(stick);
			}
			else
			{
				error(TXT("invalid vr stick \"%S\""), token->trim().to_char());
			}
		}
	}
	return result;
}

static String save_stick_definition(Array<::VR::Input::Stick::WithSource> const & _sticks)
{
	String result;
	for_every(stick, _sticks)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += ::VR::Input::Stick::WithSource::to_string(*stick);
	}
	return result.replace(String(TXT(" ,")), String::comma()).trim();
}

static bool load_button_definition(Array<::System::GamepadButton::Type> & _gamepadButtons, String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			::System::GamepadButton::Type button = ::System::GamepadButton::parse(token->trim(), ::System::GamepadButton::Type::None);
			if (button != ::System::GamepadButton::Type::None)
			{
				_gamepadButtons.push_back_unique(button);
			}
			else
			{
#ifdef AN_STANDARD_INPUT
				error(TXT("invalid gamepad button \"%S\""), token->trim().to_char());
#endif
			}
		}
	}
	return result;
}

static String save_button_definition(Array<::System::GamepadButton::Type> const & _gamepadButtons)
{
	String result;
	for_every(button, _gamepadButtons)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += ::System::GamepadButton::to_char(*button);
	}
	return result.replace(String(TXT(" ,")), String::comma()).trim();
}

static bool load_button_definition(Array<::VR::Input::Button::WithSource> & _gamepadButtons, String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			::VR::Input::Button::WithSource button = ::VR::Input::Button::WithSource::parse(token->trim(), ::VR::Input::Button::Type::None);
			if (button.type != ::VR::Input::Button::Type::None)
			{
				_gamepadButtons.push_back_unique(button);
			}
			else
			{
#ifdef AN_STANDARD_INPUT
				error(TXT("invalid gamepad button \"%S\""), token->trim().to_char());
#endif
			}
		}
	}
	return result;
}

static String save_button_definition(Array<::VR::Input::Button::WithSource> const & _gamepadButtons)
{
	String result;
	for_every(button, _gamepadButtons)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += ::VR::Input::Button::WithSource::to_string(*button);
	}
	return result.replace(String(TXT(" ,")), String::comma()).trim();
}

static bool load_mouse_definition(Array<::VR::Input::Mouse::WithSource> & _mouses, String const & _string)
{
	bool result = true;
	List<String> tokens;
	_string.split(String::comma(), tokens);
	for_every(token, tokens)
	{
		if (!token->is_empty())
		{
			::VR::Input::Mouse::WithSource mouse = ::VR::Input::Mouse::WithSource::parse(token->trim());
			_mouses.push_back_unique(mouse);
		}
	}
	return result;
}

static String save_mouse_definition(Array<::VR::Input::Mouse::WithSource> const & _mouses)
{
	String result;
	for_every(mouse, _mouses)
	{
		if (!result.is_empty())
		{
			result += TXT(", ");
		}
		result += ::VR::Input::Mouse::WithSource::to_string(*mouse);
	}
	return result.replace(String(TXT(" ,")), String::comma()).trim();
}

//

GameInputStick::GameInputStick()
: gamepadDeadZone(0.05f, 0.05f)
, vrStickDeadZone(0.0f, 0.0f)
, invertX(false)
, invertY(false)
, sensitivity(1.0f, 1.0f)
{
}

bool GameInputStick::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	vrStickPromptHint = _node->get_name_attribute(TXT("vrStickPromptHint"), vrStickPromptHint);
	//
	result &= load_stick_definition(gamepadSticks, _node->get_string_attribute(TXT("gamepadStick")));
	//
	result &= load_stick_definition(vrSticks, _node->get_string_attribute(TXT("vrStick")));
	//
	invertX = _node->get_bool_attribute(TXT("invertX"), invertX);
	invertY = _node->get_bool_attribute(TXT("invertY"), invertY);
	//
	result &= load_key_definition(xMinusKeys, _node->get_string_attribute(TXT("xMinusKey")));
	result &= load_key_definition(xPlusKeys, _node->get_string_attribute(TXT("xPlusKey")));
	result &= load_key_definition(yMinusKeys, _node->get_string_attribute(TXT("yMinusKey")));
	result &= load_key_definition(yPlusKeys, _node->get_string_attribute(TXT("yPlusKey")));
	//
	sensitivity.x = _node->get_float_attribute(TXT("sensitivity"), sensitivity.x);
	sensitivity.y = _node->get_float_attribute(TXT("sensitivity"), sensitivity.y);
	sensitivity.x = _node->get_float_attribute(TXT("xSensitivity"), sensitivity.x);
	sensitivity.y = _node->get_float_attribute(TXT("ySensitivity"), sensitivity.y);
	//
	gamepadDeadZone.x = _node->get_float_attribute(TXT("gamepadDeadZone"), gamepadDeadZone.x);
	gamepadDeadZone.y = _node->get_float_attribute(TXT("gamepadDeadZone"), gamepadDeadZone.y);
	gamepadDeadZone.x = _node->get_float_attribute(TXT("xGamepadDeadZone"), gamepadDeadZone.x);
	gamepadDeadZone.y = _node->get_float_attribute(TXT("yGamepadDeadZone"), gamepadDeadZone.y);
	//
	vrStickDeadZone.x = _node->get_float_attribute(TXT("vrStickDeadZone"), vrStickDeadZone.x);
	vrStickDeadZone.y = _node->get_float_attribute(TXT("vrStickDeadZone"), vrStickDeadZone.y);
	vrStickDeadZone.x = _node->get_float_attribute(TXT("xvrStickDeadZone"), vrStickDeadZone.x);
	vrStickDeadZone.y = _node->get_float_attribute(TXT("yvrStickDeadZone"), vrStickDeadZone.y);

	result &= load_condition(_node, REF_ keysCondition, TXT("keysCondition"));
	result &= load_condition(_node, REF_ vrSticksCondition, TXT("vrSticksCondition"));

	on_change();

	return result;
}

void GameInputStick::save_to_xml(IO::XML::Node * _intoNode)
{
	_intoNode->set_attribute(TXT("name"), name.to_string());
	if (vrStickPromptHint.is_valid())
	{
		_intoNode->set_attribute(TXT("vrStickPromptHint"), vrStickPromptHint.to_string());
	}
	{	String savedDef = save_stick_definition(gamepadSticks);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("gamepadStick"), savedDef);
		}
	}
	{	String savedDef = save_stick_definition(vrSticks);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("vrStick"), savedDef);
		}
	}
	if (invertX) { _intoNode->set_bool_attribute(TXT("invertX"), invertX); }
	if (invertY) { _intoNode->set_bool_attribute(TXT("invertY"), invertY); }
	if (sensitivity.x == sensitivity.y)
	{
		if (sensitivity.x != 1.0f) { _intoNode->set_float_attribute(TXT("sensitivity"), sensitivity.x); }
	}
	else
	{
		if (sensitivity.x != 1.0f) { _intoNode->set_float_attribute(TXT("xSensitivity"), sensitivity.x); }
		if (sensitivity.y != 1.0f) { _intoNode->set_float_attribute(TXT("ySensitivity"), sensitivity.y); }
	}
	{	String savedDef = save_key_definition(xMinusKeys);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("xMinusKey"), savedDef);
		}
	}
	{	String savedDef = save_key_definition(xPlusKeys);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("xPlusKey"), savedDef);
		}
	}
	{	String savedDef = save_key_definition(yMinusKeys);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("yMinusKey"), savedDef);
		}
	}
	{	String savedDef = save_key_definition(yPlusKeys);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("yPlusKey"), savedDef);
		}
	}
	if (gamepadDeadZone.x == gamepadDeadZone.y)
	{
		if (gamepadDeadZone.x != 0.05f) { _intoNode->set_float_attribute(TXT("gamepadDeadZone"), gamepadDeadZone.x); }
	}
	else
	{
		if (gamepadDeadZone.x != 0.05f) { _intoNode->set_float_attribute(TXT("xGamepadDeadZone"), gamepadDeadZone.x); }
		if (gamepadDeadZone.y != 0.05f) { _intoNode->set_float_attribute(TXT("yGamepadDeadZone"), gamepadDeadZone.y); }
	}
	if (vrStickDeadZone.x == vrStickDeadZone.y)
	{
		if (vrStickDeadZone.x != 0.05f) { _intoNode->set_float_attribute(TXT("vrStickDeadZone"), vrStickDeadZone.x); }
	}
	else
	{
		if (vrStickDeadZone.x != 0.05f) { _intoNode->set_float_attribute(TXT("xvrStickDeadZone"), vrStickDeadZone.x); }
		if (vrStickDeadZone.y != 0.05f) { _intoNode->set_float_attribute(TXT("yvrStickDeadZone"), vrStickDeadZone.y); }
	}

	save_condition(_intoNode, keysCondition, TXT("keysCondition"));
	save_condition(_intoNode, vrSticksCondition, TXT("vrSticksCondition"));
}

void GameInputStick::provide_prompts(OUT_ ArrayStack<ProvidedPrompt>& _prompts) const
{
	todo_note(TXT("we support vr sticks only right now!"));
	if (auto* vr = VR::IVR::get())
	{
		auto const& vrControls = vr->get_controls();

		for_every(vrStick, vrSticks)
		{
			if (vrStick->source & vrControls.hands[0].source ||
				vrStick->source & vrControls.hands[1].source)
			{
				// we want hand's source
				String p;
				p = String::printf(TXT("%S%S %S"), VR::Input::Devices::get_for_prompt_name(vrControls.hands[0].source).to_char(), vr->get_prompt_controller_suffix(vrControls.hands[0].source), VR::Input::Stick::to_char(vrStick->type));
				if (vrStickPromptHint.is_valid())
				{
					p += TXT("; ");
					p += vrStickPromptHint.to_string();
				}
				_prompts.push_back_unique(ProvidedPrompt(Name(p.trim()), vrStick->hand));
			}
		}
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	else if (MainConfig::global().get_pretend_vr_input_system().is_valid())
	{
		auto deviceId = VR::Input::Devices::find(String::empty(), String::empty(), String::empty()); // should get through forced
		if (deviceId != VR::Input::Devices::all)
		{
			for_every(vrStick, vrSticks)
			{
				if (vrStick->source & deviceId ||
					vrStick->source & deviceId)
				{
					// we want hand's source
					String p;
					p = String::printf(TXT("%S %S"), VR::Input::Devices::get_for_prompt_name(deviceId).to_char(), VR::Input::Stick::to_char(vrStick->type));
					if (vrStickPromptHint.is_valid())
					{
						p += TXT("; ");
						p += vrStickPromptHint.to_string();
					}
					_prompts.push_back_unique(ProvidedPrompt(Name(p.trim()), vrStick->hand));
				}
			}
		}
	}
#endif
}

//

GameInputMouse::GameInputMouse()
: useX(true)
, useY(true)
, invertX(false)
, invertY(false)
, sensitivity(1.0f, 1.0f)
, vrSensitivity(1.0f, 1.0f)
, mouse(true)
{
}

bool GameInputMouse::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	//
	useX = _node->get_bool_attribute(TXT("useX"), useX);
	useY = _node->get_bool_attribute(TXT("useY"), useY);
	//
	invertX = _node->get_bool_attribute(TXT("invertX"), invertX);
	invertY = _node->get_bool_attribute(TXT("invertY"), invertY);
	//
	mouse = _node->get_bool_attribute(TXT("mouse"), mouse);
	//
	sensitivity.x = _node->get_float_attribute(TXT("sensitivity"), sensitivity.x);
	sensitivity.y = _node->get_float_attribute(TXT("sensitivity"), sensitivity.y);
	sensitivity.x = _node->get_float_attribute(TXT("xSensitivity"), sensitivity.x);
	sensitivity.y = _node->get_float_attribute(TXT("ySensitivity"), sensitivity.y);
	//
	result &= load_mouse_definition(vrMouses, _node->get_string_attribute(TXT("vrMouse")));
	//
	vrSensitivity.x = _node->get_float_attribute(TXT("vrSensitivity"), vrSensitivity.x);
	vrSensitivity.y = _node->get_float_attribute(TXT("vrSensitivity"), vrSensitivity.y);
	vrSensitivity.x = _node->get_float_attribute(TXT("xvrSensitivity"), vrSensitivity.x);
	vrSensitivity.y = _node->get_float_attribute(TXT("yvrSensitivity"), vrSensitivity.y);

	result &= load_condition(_node, REF_ mouseCondition, TXT("mouseCondition"));
	result &= load_condition(_node, REF_ vrMousesCondition, TXT("vrMousesCondition"));

	on_change();

	return result;
}

void GameInputMouse::save_to_xml(IO::XML::Node * _intoNode)
{
	_intoNode->set_attribute(TXT("name"), name.to_string());
	if (! useX) { _intoNode->set_bool_attribute(TXT("useX"), useX); }
	if (! useY) { _intoNode->set_bool_attribute(TXT("useY"), useY); }
	if (invertX) { _intoNode->set_bool_attribute(TXT("invertX"), invertX); }
	if (invertY) { _intoNode->set_bool_attribute(TXT("invertY"), invertY); }
	if (!mouse) { _intoNode->set_bool_attribute(TXT("mouse"), mouse); }
	if (sensitivity.x == sensitivity.y)
	{
		if (sensitivity.x != 1.0f) { _intoNode->set_float_attribute(TXT("sensitivity"), sensitivity.x); }
	}
	else
	{
		if (sensitivity.x != 1.0f) { _intoNode->set_float_attribute(TXT("xSensitivity"), sensitivity.x); }
		if (sensitivity.y != 1.0f) { _intoNode->set_float_attribute(TXT("ySensitivity"), sensitivity.y); }
	}
	{	String savedDef = save_mouse_definition(vrMouses);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("vrMouse"), savedDef);
		}
	}
	if (vrSensitivity.x == vrSensitivity.y)
	{
		if (vrSensitivity.x != 1.0f) { _intoNode->set_float_attribute(TXT("vrSensitivity"), vrSensitivity.x); }
	}
	else
	{
		if (vrSensitivity.x != 1.0f) { _intoNode->set_float_attribute(TXT("xvrSensitivity"), vrSensitivity.x); }
		if (vrSensitivity.y != 1.0f) { _intoNode->set_float_attribute(TXT("yvrSensitivity"), vrSensitivity.y); }
	}
	save_condition(_intoNode, mouseCondition, TXT("mouseCondition"));
	save_condition(_intoNode, vrMousesCondition, TXT("vrMousesCondition"));
}

//

GameInputButton::GameInputButton()
{
}

void GameInputButton::on_change()
{
	gamepadStickDirsAsButtons.clear();
	if (gamepadStickDir != ::System::GamepadStickDir::None)
	{
		for_every(gamepadStick, gamepadSticks)
		{
			::System::GamepadButton::Type asButton = ::System::GamepadButton::from_stick(*gamepadStick, gamepadStickDir);
			if (asButton != ::System::GamepadButton::None)
			{
				gamepadStickDirsAsButtons.push_back_unique(asButton);
			}
		}
	}
}

bool GameInputButton::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	vrButtonPrompt = _node->get_name_attribute(TXT("vrButtonPrompt"), vrButtonPrompt);
	vrButtonPromptHint = _node->get_name_attribute(TXT("vrButtonPromptHint"), vrButtonPromptHint);
	vrButtonPromptTextId = _node->get_name_attribute(TXT("vrButtonPromptTextId"), vrButtonPromptTextId);
	//
	allRequired = _node->get_bool_attribute_or_from_child_presence(TXT("allRequired"), allRequired);
	//
	result &= load_stick_definition(gamepadSticks, _node->get_string_attribute(TXT("gamepadStick")));
	gamepadStickDir = ::System::GamepadStickDir::parse(_node->get_string_attribute(TXT("gamepadStickDir")), ::System::GamepadStickDir::None);

	result &= load_button_definition(gamepadButtons, _node->get_string_attribute(TXT("gamepadButton")));
	//
	result &= load_button_definition(vrButtons, _node->get_string_attribute(TXT("vrButton")));
	//
	result &= load_key_definition(keys, _node->get_string_attribute(TXT("key")));
	//
	noVR = _node->get_bool_attribute_or_from_child_presence(TXT("noVR"), noVR);
	mouseButtonIdx = _node->get_int_attribute(TXT("mouseButtonIdx"), mouseButtonIdx);

	result &= load_condition(_node, REF_ keysCondition, TXT("keysCondition"));
	result &= load_condition(_node, REF_ mouseCondition, TXT("mouseCondition"));
	result &= load_condition(_node, REF_ vrButtonsCondition, TXT("vrButtonsCondition"));

	on_change();

	return result;
}

void GameInputButton::save_to_xml(IO::XML::Node * _intoNode)
{
	_intoNode->set_attribute(TXT("name"), name.to_string());
	if (vrButtonPrompt.is_valid())
	{
		_intoNode->set_attribute(TXT("vrButtonPrompt"), vrButtonPrompt.to_string());
	}
	if (vrButtonPromptHint.is_valid())
	{
		_intoNode->set_attribute(TXT("vrButtonPromptHint"), vrButtonPromptHint.to_string());
	}
	if (vrButtonPromptTextId.is_valid())
	{
		_intoNode->set_attribute(TXT("vrButtonPromptTextId"), vrButtonPromptTextId.to_string());
	}
	if (allRequired)
	{
		_intoNode->set_bool_attribute(TXT("allRequired"), allRequired);
	}
	{	String savedDef = save_stick_definition(gamepadSticks);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("gamepadStick"), savedDef);
		}
	}
	if (gamepadStickDir != ::System::GamepadStickDir::None)
	{
		_intoNode->set_attribute(TXT("gamepadStickDir"), String(::System::GamepadStickDir::to_char(gamepadStickDir)));
	}
	//
	{	String savedDef = save_button_definition(gamepadButtons);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("gamepadButton"), savedDef);
		}
	}
	//
	{	String savedDef = save_button_definition(vrButtons);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("vrButton"), savedDef);
		}
	}
	//
	{	String savedDef = save_key_definition(keys);
		if (!savedDef.is_empty())
		{
			_intoNode->set_attribute(TXT("key"), savedDef);
		}
	}
	//
	if (noVR)
	{
		_intoNode->set_bool_attribute(TXT("noVR"), noVR);
	}
	if (mouseButtonIdx != NONE)
	{
		_intoNode->set_int_attribute(TXT("mouseButtonIdx"), mouseButtonIdx);
	}
	save_condition(_intoNode, keysCondition, TXT("keysCondition"));
	save_condition(_intoNode, mouseCondition, TXT("mouseCondition"));
	save_condition(_intoNode, vrButtonsCondition, TXT("vrButtonsCondition"));
}

void GameInputButton::provide_prompts(OUT_ ArrayStack<ProvidedPrompt>& _prompts, OUT_ ArrayStack<Name>& _promptTextIds) const
{
	todo_note(TXT("we support vr buttons only right now! (or explicit)"));
	{
		if (! vrButtonsCondition.is_set() ||
			vrButtonsCondition->check(::System::Input::get()->get_usage_tags(Hand::Left)) ||
			vrButtonsCondition->check(::System::Input::get()->get_usage_tags(Hand::Right)))
		{
			if (vrButtonPrompt.is_valid())
			{
				_prompts.push_back_unique(ProvidedPrompt(vrButtonPrompt));
			}
			else if (vrButtonPromptTextId.is_valid())
			{
				_promptTextIds.push_back_unique(vrButtonPromptTextId);
			}
			else if (auto* vr = VR::IVR::get())
			{
				auto const& vrControls = vr->get_controls();

				for_every(vrButton, vrButtons)
				{
					if (vrButton->source & vrControls.hands[0].source ||
						vrButton->source & vrControls.hands[1].source)
					{
						// we want hand's source
						String p;
						p = String::printf(TXT("%S%S %S"), VR::Input::Devices::get_for_prompt_name(vrControls.hands[0].source).to_char(), vr->get_prompt_controller_suffix(vrControls.hands[0].source), VR::Input::Button::to_char(vrButton->type));
						if (vrButton->released)
						{
							p += TXT("; released");
						}
						if (vrButtonPromptHint.is_valid())
						{
							p += TXT("; ");
							p += vrButtonPromptHint.to_string();
						}
						_prompts.push_back_unique(ProvidedPrompt(Name(p.trim()), vrButton->hand));
					}
				}
			}
#ifdef AN_DEVELOPMENT_OR_PROFILER
			else if (MainConfig::global().get_pretend_vr_input_system().is_valid())
			{
				auto deviceId = VR::Input::Devices::find(String::empty(), String::empty(), String::empty()); // should get through forced
				if (deviceId != VR::Input::Devices::all)
				{
					for_every(vrButton, vrButtons)
					{
						if (vrButton->source & deviceId ||
							vrButton->source & deviceId)
						{
							// we want hand's source
							String p;
							p = String::printf(TXT("%S %S"), VR::Input::Devices::get_for_prompt_name(deviceId).to_char(), VR::Input::Button::to_char(vrButton->type));
							if (vrButton->released)
							{
								p += TXT("; released");
							}
							if (vrButtonPromptHint.is_valid())
							{
								p += TXT("; ");
								p += vrButtonPromptHint.to_string();
							}
							_prompts.push_back_unique(ProvidedPrompt(Name(p.trim()), vrButton->hand));
						}
					}
				}
			}
#endif
		}
	}
}

//

bool GameInputOption::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	inOptionsNegated = _node->get_bool_attribute_or_from_child_presence(TXT("inOptionsNegated"), inOptionsNegated);
	//
	result &= load_condition(_node, REF_ condition, TXT("condition"));

	return result;
}

void GameInputOption::save_to_xml(IO::XML::Node * _intoNode)
{
	_intoNode->set_attribute(TXT("name"), name.to_string());
	if (inOptionsNegated)
	{
		_intoNode->set_bool_attribute(TXT("inOptionsNegated"), inOptionsNegated);
	}
	save_condition(_intoNode, condition, TXT("condition"));
}

Name GameInputOption::get_loc_str_id() const
{
	return Name(String::printf(TXT("game input option; %S"), name.to_char()));
}

//

GameInputDefinition::GameInputDefinition()
{
}

bool GameInputDefinition::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(stickNode, _node->children_named(TXT("stick")))
	{
		GameInputStick inputDef;
		if (inputDef.load_from_xml(stickNode))
		{
			sticks.push_back(inputDef);
		}
		else
		{
			error_loading_xml(stickNode, TXT("could not load input definition"));
		}
	}

	for_every(mouseNode, _node->children_named(TXT("mouse")))
	{
		GameInputMouse inputDef;
		if (inputDef.load_from_xml(mouseNode))
		{
			mouses.push_back(inputDef);
		}
		else
		{
			error_loading_xml(mouseNode, TXT("could not load input definition"));
		}
	}

	for_every(buttonNode, _node->children_named(TXT("button")))
	{
		GameInputButton inputDef;
		if (inputDef.load_from_xml(buttonNode))
		{
			buttons.push_back(inputDef);
		}
		else
		{
			error_loading_xml(buttonNode, TXT("could not load input definition"));
		}
	}

	for_every(optionNode, _node->children_named(TXT("option")))
	{
		GameInputOption optionDef;
		if (optionDef.load_from_xml(optionNode))
		{
			bool alreadyExists = false;
			for_every(opt, options)
			{
				if (opt->name == optionDef.name)
				{
					*opt = optionDef;
					alreadyExists = true;
					break;
				}
			}
			if (!alreadyExists)
			{
				options.push_back(optionDef);
			}
		}
		else
		{
			error_loading_xml(optionNode, TXT("could not load option definition"));
		}
	}

	return result;
}

void GameInputDefinition::save_to_xml(IO::XML::Node * _intoNode)
{
	for_every(stick, sticks)
	{
		IO::XML::Node* node = _intoNode->add_node(TXT("stick"));
		stick->save_to_xml(node);
	}

	for_every(mouse, mouses)
	{
		IO::XML::Node* node = _intoNode->add_node(TXT("mouse"));
		mouse->save_to_xml(node);
	}

	for_every(button, buttons)
	{
		IO::XML::Node* node = _intoNode->add_node(TXT("button"));
		button->save_to_xml(node);
	}

	for_every(option, options)
	{
		IO::XML::Node* node = _intoNode->add_node(TXT("option"));
		option->save_to_xml(node);
	}
}

void GameInputDefinition::clear()
{
	sticks.clear();
	mouses.clear();
	buttons.clear();
	//
	options.clear();
}

void GameInputDefinition::on_change()
{
	for_every(stick, sticks)
	{
		stick->on_change();
	}

	for_every(mouse, mouses)
	{
		mouse->on_change();
	}

	for_every(button, buttons)
	{
		button->on_change();
	}
}

void GameInputDefinition::provide_prompts(Name const& _input, OUT_ ArrayStack<ProvidedPrompt>& _prompts, OUT_ ArrayStack<Name>& _promptTextIds) const
{
	for_every(stick, sticks)
	{
		if (stick->name == _input)
		{
			stick->provide_prompts(_prompts);
		}
	}
	for_every(button, buttons)
	{
		if (button->name == _input)
		{
			button->provide_prompts(_prompts, _promptTextIds);
		}
	}
}

//

void GameInputDefinitions::initialise_static()
{
	an_assert(s_definitions == nullptr);
	s_definitions = new GameInputDefinitions();
}

void GameInputDefinitions::close_static()
{
	if (s_definitions)
	{
		for_every_ptr(gid, s_definitions->definitions)
		{
			gid->release_ref();
		}
		delete_and_clear(s_definitions);
	}
}

void GameInputDefinitions::clear()
{
	if (s_definitions)
	{
		for_every_ptr(gid, s_definitions->definitions)
		{
			gid->clear();
		}
	}
}

bool GameInputDefinitions::load_from_xml(IO::XML::Node const * _node)
{
	an_assert(s_definitions != nullptr);
	if (_node->get_bool_attribute_or_from_child_presence(TXT("developmentOnly")))
	{
		if (auto* game = Game::get())
		{
			if (game->does_want_to_create_main_config_only())
			{
				bool skip = true;
#ifndef AN_RELEASE
				if (_node->get_bool_attribute_or_from_child_presence(TXT("keepForMainConfigIfNotFinal")))
				{
					skip = false;
				}
#endif
				if (skip)
				{
					return true;
				}
			}
		}
#ifdef AN_ALLOW_BULLSHOTS
		if (BullshotSystem::is_setting_active(NAME(bsNoDevelopmentInput)))
		{
			return true;
		}
#endif
#ifndef AN_DEVELOPMENT_OR_PROFILER
		return true; // skip it
#endif
	}
	if (_node->get_bool_attribute_or_from_child_presence(TXT("notDevelopment")))
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
		bool createMainConfig = false;
		if (auto* game = Game::get())
		{
			if (game->does_want_to_create_main_config_only())
			{
				createMainConfig = true;
			}
		}
		if (!createMainConfig)
		{
#ifdef AN_ALLOW_BULLSHOTS
			if (BullshotSystem::is_setting_active(NAME(bsNoDevelopmentInput)))
			{
				// use it!
			}
			else
#endif
			{
				return true; // skip it
			}
		}
#endif
	}
	Name defName = _node->get_name_attribute(TXT("name"));
	if (!defName.is_valid())
	{
		defName = Names::default_;
	}
	GameInputDefinition* gid = nullptr;
	if (GameInputDefinition** existingGID = s_definitions->definitions.get_existing(defName))
	{
		gid = *existingGID;
	}
	if (gid == nullptr)
	{
		gid = new GameInputDefinition();
		gid->add_ref();
		s_definitions->definitions[defName] = gid;
	}
	return gid->load_from_xml(_node);
}

void GameInputDefinitions::save_to_xml(IO::XML::Node * _container)
{
	an_assert(s_definitions != nullptr);

	for_every_ptr(definition, s_definitions->definitions)
	{
		IO::XML::Node* node = _container->add_node(TXT("gameInputDefinition"));
		node->set_attribute(TXT("name"), for_everys_map_key(definition).to_string());
		definition->save_to_xml(node);
	}
}

GameInputDefinition* GameInputDefinitions::get_definition(Name const & _name)
{
	an_assert(s_definitions != nullptr);
	if (GameInputDefinition** existingGID = s_definitions->definitions.get_existing(_name))
	{
		return *existingGID;
	}
	else
	{
		return nullptr;
	}
}
