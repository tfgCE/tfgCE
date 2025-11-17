#pragma once

#include "..\..\core\containers\arrayStatic.h"
#include "..\..\core\system\input.h"

#include "..\text\localisedString.h"

namespace Framework
{
	struct GameRetroInput
	{
		// 4 buttons cross layout + 2 bumpers + start + select, can be used for whatever purpose
		static const int ACTION_COUNT = 8;
		static const int PLAYER_COUNT = 4;

		struct Config
		{
			// this is to show to the player, etc
			struct InputDefinition
			{
				struct ActionDefinition
				{
					Name id;
					Framework::LocalisedString name;
				};
				ArrayStatic<ActionDefinition, GameRetroInput::ACTION_COUNT> actions;

				InputDefinition() { actions.set_size(GameRetroInput::ACTION_COUNT); }

				bool load_from_xml(IO::XML::Node const* _node);
				void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;
			};

			struct KeyboardInputDefinition
			{
				struct KeySet
				{
					ArrayStatic<System::Key::Type, 8> keys;
					bool forceClear = false; // if set, will save even if no keys provided

					void clear() { keys.clear(); }
					void add(System::Key::Type _key) { if (keys.has_place_left()) keys.push_back(_key); }

					String to_string() const;
					bool parse(String const& _string);

					bool is_pressed(::System::Input const* _input) const;
				};

				struct Stick
				{
					KeySet left;
					KeySet right;
					KeySet up;
					KeySet down;

					bool load_from_xml(IO::XML::Node const* _node);
					void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;
				} stick;

				struct Action
				{
					ArrayStatic<KeySet, GameRetroInput::ACTION_COUNT> actions;

					Action() { actions.set_size(GameRetroInput::ACTION_COUNT); }

					bool load_from_xml(IO::XML::Node const* _node);
					void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;
				} action;

				bool load_from_xml(IO::XML::Node const* _node);
				void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;
			};

			struct GamepadInputDefinition
			{
				Name id;
				Framework::LocalisedString name; // scheme visible to the player

				bool useLeftStick = true;
				bool useRightStick = true;
				bool useDPadStick = true;

				struct ButtonSet
				{
					ArrayStatic<System::GamepadButton::Type, 8> buttons;
					bool forceClear = false; // if set, will save even if no keys provided

					void clear() { buttons.clear(); }
					void add(System::GamepadButton::Type _button) { if (buttons.has_place_left()) buttons.push_back(_button); }

					String to_string() const;
					bool parse(String const& _string);

					bool is_pressed(::System::Gamepad const* _gamepad) const;
				};

				struct Action
				{
					ArrayStatic<ButtonSet, GameRetroInput::ACTION_COUNT> actions;

					Action() { actions.set_size(GameRetroInput::ACTION_COUNT); }

					bool load_from_xml(IO::XML::Node const* _node);
					void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;
				} action;

				bool load_from_xml(IO::XML::Node const* _node);
				void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;
			};

			// which gamepad definition uses and individual keys
			struct PlayerInputDefinition
			{
				Name gamepadInputDefinitionId;

				KeyboardInputDefinition keyboardInputDefinition;

				bool load_from_xml(IO::XML::Node const* _node);
				void save_to_xml(IO::XML::Node* _container, bool _userSpecific, int _index) const;
			};

			float stickDirDeadZone = 0.5f;
			InputDefinition inputDefinition;
			Array<GamepadInputDefinition> gamepadInputDefinitions;
			ArrayStatic<PlayerInputDefinition, GameRetroInput::PLAYER_COUNT> playerInputDefinitions;

			bool load_from_xml(IO::XML::Node const* _node);
			void save_to_xml(IO::XML::Node* _container, bool _userSpecific) const;

			Config() { playerInputDefinitions.set_size(GameRetroInput::PLAYER_COUNT); }
		};

		struct State
		{
			struct PlayerInputState
			{
				struct ActionState
				{
				public:
					bool is_pressed() const { return pressed; }
					bool has_been_pressed() const { return pressed && !wasPressed; }
					bool has_been_released() const { return !pressed && wasPressed; }
					bool has_been_clicked() const { return has_been_released() && lastPressTime < CLICK_TIME; }

				private:
					const float CLICK_TIME = 0.3f;

					bool pressed = false;
					bool wasPressed = false;
					float lastPressTime = 0.0f;

					bool resetLastPressTimeOnPress = false;

					friend struct State;
				};

				PlayerInputState() { actions.set_size(GameRetroInput::ACTION_COUNT); }

				Vector2 const & get_stick() const { return stick; }
				VectorInt2 const& get_stick_dir() const { return stickDir; }
				ActionState const& get_action(int _idx) const { return actions[_idx]; }

			public:
				// gamepad management is done automatically
				// if a button is pressed on a gamepad that is not read yet, it will be activated for the first available player
				void forget_gamepad() { gamepadSystemId.clear();  }

			private:
				Optional<int> gamepadSystemId;
				bool waitForNoGamepadControls = false; // after attaching gamepad will wait for no controls

				CACHED_ int gamepadInputDefinitionIdx = NONE;
				CACHED_ Name gamepadInputDefinitionIdxCachedForId;

				Vector2 stick = Vector2::zero;
				VectorInt2 stickDir = VectorInt2::zero;
				ArrayStatic<ActionState, GameRetroInput::ACTION_COUNT> actions;

				friend struct State;
			};

			struct Keyboard
			{
				bool is_pressed(::System::Key::Type _key) const { return keys[_key].pressed; }
				bool has_been_pressed(::System::Key::Type _key) const { return keys[_key].pressed && !keys[_key].wasPressed; }
				bool has_been_released(::System::Key::Type _key) const { return !keys[_key].pressed && keys[_key].wasPressed; }

			private:
				struct Key
				{
					bool pressed = false;
					bool wasPressed = false;
				};
				ArrayStatic<Key, ::System::Key::NUM> keys;

				friend struct State;
			};

			State();

			PlayerInputState const& get_player_state(int _idx) const { return playerInputStates[_idx]; }

			Keyboard const& get_keyboard() const { return keyboard; }

			// update has to be done in three steps
			void update_start(); // clear actions and stick
			void read_input(float _deltaTime); // if an action is activated, store this here
			void update_end(); // determine stuff post update

		private:
			ArrayStatic<PlayerInputState, PLAYER_COUNT> playerInputStates;
			Keyboard keyboard;
			
			bool is_gamepad_used(int _gamepadSystemId) const;
			bool is_gamepad_active(::System::Gamepad const* _gamepad) const;
			Optional<int> find_active_unasigned_gamepad(::System::Input const* _input) const;
		};
	};
};

