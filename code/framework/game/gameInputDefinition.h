#pragma once

#include "..\..\core\containers\map.h"
#include "..\..\core\tags\tagCondition.h"
#include "..\..\core\types\name.h"
#include "..\..\core\system\gamepad.h"
#include "..\..\core\system\input.h"
#include "..\..\core\vr\vrInput.h"
#include "..\..\core\memory\refCountObject.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace Framework
{
	struct ProvidedPrompt
	{
		Name prompt;
		Optional<Hand::Type> forHand;
		ProvidedPrompt() {}
		explicit ProvidedPrompt(Name const& _prompt, Optional<Hand::Type> const& _forHand = NP) : prompt(_prompt), forHand(_forHand) {}

		bool operator==(ProvidedPrompt const& _o) const { return prompt == _o.prompt && forHand == _o.forHand; }
	};
	struct GameInputStick
	{
		Name name;
		Name vrStickPromptHint; // added to prompt after a semicolon
		Array<::System::GamepadStick::Type> gamepadSticks;
		Vector2 gamepadDeadZone; // this is additional dead zone on top of system dead zone
		Array<::VR::Input::Stick::WithSource> vrSticks;
		Vector2 vrStickDeadZone; // this is additional dead zone on top of system dead zone
		bool invertX;
		bool invertY;
		Vector2 sensitivity;
		RefCountObjectPtr<TagCondition> keysCondition;
		RefCountObjectPtr<TagCondition> vrSticksCondition; // similar as buttons

		Array<::System::Key::Type> xMinusKeys;
		Array<::System::Key::Type> xPlusKeys;
		Array<::System::Key::Type> yMinusKeys;
		Array<::System::Key::Type> yPlusKeys;

		GameInputStick();

		bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _intoNode);
		void on_change() {}

		void provide_prompts(OUT_ ArrayStack<ProvidedPrompt>& _prompts) const;
	};

	struct GameInputMouse
	{
		Name name;
		bool useX;
		bool useY;
		bool invertX;
		bool invertY;
		Vector2 sensitivity;
		Vector2 vrSensitivity;
		bool mouse;
		Array<::VR::Input::Mouse::WithSource> vrMouses;
		RefCountObjectPtr<TagCondition> mouseCondition;
		RefCountObjectPtr<TagCondition> vrMousesCondition; // as buttons

		GameInputMouse();

		bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _intoNode);
		void on_change() {}
	};

	struct GameInputButton
	{
		Name name;
		bool allRequired = false;
		Name vrButtonPrompt; // what prompt should it use to display
		Name vrButtonPromptHint; // added to prompt after a semicolon
		Name vrButtonPromptTextId; // just show plain text (loc str identified by this id)
		Array<::System::GamepadButton::Type> gamepadButtons;
		Array<::VR::Input::Button::WithSource> vrButtons;
		Array<::System::Key::Type> keys;
		Array<::System::GamepadStick::Type> gamepadSticks;
		::System::GamepadStickDir::Type gamepadStickDir = ::System::GamepadStickDir::None;
		CACHED_ Array<::System::GamepadButton::Type> gamepadStickDirsAsButtons;
		bool noVR = false; // if "noVR" means that it is always on when there is noVR
		int mouseButtonIdx = NONE;
		RefCountObjectPtr<TagCondition> keysCondition;
		RefCountObjectPtr<TagCondition> mouseCondition;
		RefCountObjectPtr<TagCondition> vrButtonsCondition; // some are built-in (vr, handTracking, usingControllers) some depend on vr devices, some on player options/preferences

		GameInputButton();

		bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _intoNode);
		void on_change();

		void provide_prompts(OUT_ ArrayStack<ProvidedPrompt>& _prompts, OUT_ ArrayStack<Name>& _promptTextIds) const;
	};

	struct GameInputOption
	{
		Name name;
		bool inOptionsNegated = false;
		RefCountObjectPtr<TagCondition> condition;
		// localised string id is automatically created

		bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _intoNode);

		Name get_loc_str_id() const;
	};
	
	class GameInputDefinition
	: public RefCountObject
	{
	public:
		GameInputDefinition();

		Array<GameInputStick> const & get_sticks() const { return sticks; }
		Array<GameInputMouse> const & get_mouses() const { return mouses; }
		Array<GameInputButton> const & get_buttons() const { return buttons; }
		Array<GameInputOption> const & get_options() const { return options; }

		void clear(); // to reload

		void on_change();

		void provide_prompts(Name const& _input, OUT_ ArrayStack<ProvidedPrompt>& _vrButtonPrompts, OUT_ ArrayStack<Name>& _vrButtonPromptTextIds) const;

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node);
		void save_to_xml(IO::XML::Node * _intoNode);

	private:
		// each entry is a separate one, this way we can have quite precise control which input is used (depending on inputTags)
		Array<GameInputStick> sticks;
		Array<GameInputMouse> mouses;
		Array<GameInputButton> buttons;
		//
		Array<GameInputOption> options; // they should be unique
	};

	struct GameInputDefinitions
	{
	public:
		static void initialise_static();
		static void close_static();

		static void clear(); // to reload
		static bool load_from_xml(IO::XML::Node const * _node);
		static void save_to_xml(IO::XML::Node * _container);

		static GameInputDefinition* get_definition(Name const & _name);

	private:
		static GameInputDefinitions* s_definitions;

		Map<Name, GameInputDefinition*> definitions;
	};

};
