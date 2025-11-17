#pragma once

#ifdef AN_SDL
#include "SDL.h"
#endif

#include "gamepad.h"
#include "..\containers\list.h"
#include "..\tags\tag.h"
#include "..\types\hand.h"

namespace System
{
	class Gamepad;

	namespace MouseButton
	{
		enum Type
		{
			Left = 0,
			Middle = 1,
			Right = 2
		};
	};

	namespace Key
	{
		enum Type
		{
			None			= -1,
			//
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
			Esc				= SDL_SCANCODE_ESCAPE,
			F1				= SDL_SCANCODE_F1,
			F2				= SDL_SCANCODE_F2,
			F3				= SDL_SCANCODE_F3,
			F4				= SDL_SCANCODE_F4,
			F5				= SDL_SCANCODE_F5,
			F6				= SDL_SCANCODE_F6,
			F7				= SDL_SCANCODE_F7,
			F8				= SDL_SCANCODE_F8,
			F9				= SDL_SCANCODE_F9,
			F10				= SDL_SCANCODE_F10,
			F11				= SDL_SCANCODE_F11,
			F12				= SDL_SCANCODE_F12,
			RightShift		= SDL_SCANCODE_RSHIFT,
			LeftShift		= SDL_SCANCODE_LSHIFT,
			RightCtrl		= SDL_SCANCODE_RCTRL,
			LeftCtrl		= SDL_SCANCODE_LCTRL,
			RightAlt		= SDL_SCANCODE_RALT,
			LeftAlt			= SDL_SCANCODE_LALT,
			A				= SDL_SCANCODE_A,
			B				= SDL_SCANCODE_B,
			C				= SDL_SCANCODE_C,
			D				= SDL_SCANCODE_D,
			E				= SDL_SCANCODE_E,
			F				= SDL_SCANCODE_F,
			G				= SDL_SCANCODE_G,
			H				= SDL_SCANCODE_H,
			I				= SDL_SCANCODE_I,
			J				= SDL_SCANCODE_J,
			K				= SDL_SCANCODE_K,
			L				= SDL_SCANCODE_L,
			M				= SDL_SCANCODE_M,
			N				= SDL_SCANCODE_N,
			O				= SDL_SCANCODE_O,
			P				= SDL_SCANCODE_P,
			Q				= SDL_SCANCODE_Q,
			R				= SDL_SCANCODE_R,
			S				= SDL_SCANCODE_S,
			T				= SDL_SCANCODE_T,
			U				= SDL_SCANCODE_U,
			V				= SDL_SCANCODE_V,
			W				= SDL_SCANCODE_W,
			X				= SDL_SCANCODE_X,
			Y				= SDL_SCANCODE_Y,
			Z				= SDL_SCANCODE_Z,
			N1				= SDL_SCANCODE_1,
			N2				= SDL_SCANCODE_2,
			N3				= SDL_SCANCODE_3,
			N4				= SDL_SCANCODE_4,
			N5				= SDL_SCANCODE_5,
			N6				= SDL_SCANCODE_6,
			N7				= SDL_SCANCODE_7,
			N8				= SDL_SCANCODE_8,
			N9				= SDL_SCANCODE_9,
			N0				= SDL_SCANCODE_0,
			Space			= SDL_SCANCODE_SPACE,
			Backspace		= SDL_SCANCODE_BACKSPACE,
			Return			= SDL_SCANCODE_RETURN,
			Tab				= SDL_SCANCODE_TAB,
			RightArrow		= SDL_SCANCODE_RIGHT,
			LeftArrow		= SDL_SCANCODE_LEFT,
			DownArrow		= SDL_SCANCODE_DOWN,
			UpArrow			= SDL_SCANCODE_UP,
			Minus			= SDL_SCANCODE_MINUS,
			Equals			= SDL_SCANCODE_EQUALS,
			LeftBracket		= SDL_SCANCODE_LEFTBRACKET,
			RightBracket	= SDL_SCANCODE_RIGHTBRACKET,
			Backslash		= SDL_SCANCODE_BACKSLASH,
			Semicolon		= SDL_SCANCODE_SEMICOLON,
			Apostrophe		= SDL_SCANCODE_APOSTROPHE,
			Tilde			= SDL_SCANCODE_GRAVE,
			Comma			= SDL_SCANCODE_COMMA,
			Period			= SDL_SCANCODE_PERIOD,
			Slash			= SDL_SCANCODE_SLASH,
			PageDown		= SDL_SCANCODE_PAGEDOWN,
			PageUp			= SDL_SCANCODE_PAGEUP,
			Insert			= SDL_SCANCODE_INSERT,
			Delete			= SDL_SCANCODE_DELETE,
			Home			= SDL_SCANCODE_HOME,
			End				= SDL_SCANCODE_END,

			KP_N1			= SDL_SCANCODE_KP_1,
			KP_N2			= SDL_SCANCODE_KP_2,
			KP_N3			= SDL_SCANCODE_KP_3,
			KP_N4			= SDL_SCANCODE_KP_4,
			KP_N5			= SDL_SCANCODE_KP_5,
			KP_N6			= SDL_SCANCODE_KP_6,
			KP_N7			= SDL_SCANCODE_KP_7,
			KP_N8			= SDL_SCANCODE_KP_8,
			KP_N9			= SDL_SCANCODE_KP_9,
			KP_N0			= SDL_SCANCODE_KP_0,
			KP_Divide		= SDL_SCANCODE_KP_DIVIDE,
			KP_Multiply		= SDL_SCANCODE_KP_MULTIPLY,
			KP_Minus		= SDL_SCANCODE_KP_MINUS,
			KP_Plus			= SDL_SCANCODE_KP_PLUS,
			KP_Enter		= SDL_SCANCODE_KP_ENTER,
			KP_Period		= SDL_SCANCODE_KP_PERIOD,
#else
			#error implement for non sdl
#endif

			SPECIAL		= 500,
			MouseWheelLeft,
			MouseWheelRight,
			MouseWheelDown,
			MouseWheelUp,
			InUse, // always true
#else
			// it's ok to have nothing here
			AKey			= 0,
#endif

			NUM
		};

		Type parse(String const & _value, Type _default);
		tchar const * const to_char(Key::Type _key);
		tchar const * const to_ui_char(Key::Type _key);
	};

	class Input
	{
		friend class Core;

	public:
		static void create();
		static void terminate();
		static Input* get() { return s_current; }

		Tags const& get_usage_tags(Optional<Hand::Type> const & _hand = NP) const;
		void set_usage_tag(Name const& _tag, Optional<Hand::Type> const& _hand = NP);
		void remove_usage_tag(Name const& _tag, Optional<Hand::Type> const& _hand = NP);
		void set_usage_tags_from(Tags const & _tags, Optional<Hand::Type> const& _hand = NP);
		void remove_usage_tags(Tags const & _tags, Optional<Hand::Type> const& _hand = NP);

		void init_cursor();
		void show_cursor(bool _show);
		void grab(bool _grab);
		void update_grab();
		bool is_grabbed() const { return inputIsGrabbed; }

		inline bool is_key_pressed(Key::Type _key) const;
		inline bool is_key_pressed(Array<Key::Type> const & _keys, bool _allRequired = false) const;
		inline bool has_key_been_pressed(Key::Type _key) const;
		bool has_key_been_hold_pressed(Key::Type _key) const; // if held will act as it was pressed multiple times, after second, after half, after 0.1s
		inline bool has_key_been_pressed(Array<Key::Type> const & _keys) const;
		inline bool has_key_been_released(Key::Type _key) const;
		inline bool has_key_been_released(Array<Key::Type> const & _keys) const;

		inline Vector2 get_mouse_window_location() const { return mouseWindowLocation; }
		inline Vector2 get_mouse_relative_location() const;
		inline float get_mouse_button_pressed_time(int32 _button) const;
		inline bool is_mouse_button_pressed(int32 _button) const;
		inline bool was_mouse_button_pressed(int32 _button) const;
		inline bool has_mouse_button_been_pressed(int32 _button) const;
		inline bool has_mouse_button_been_released(int32 _button) const;
		inline bool is_mouse_button_valid(int32 _button) const;

		List<Gamepad*> const & get_gamepads() const { return gamepads; }
		Gamepad* get_gamepad(int _index) const;
		Gamepad* get_gamepad_by_system_id(int _systemID) const;

		bool is_ok() const { return true; }

		bool is_keyboard_active() const { return keyboardIsActive; }
		bool is_mouse_active() const { return mouseIsActive; }
		bool is_mouse_button_active() const { return mouseButtonIsActive; }
		bool is_gamepad_active() const { return gamepadIsActive; }
		void update_gamepad_is_active();

		bool is_any_button_or_key_pressed() const;

	private:
		static const int32 c_keyNum = Key::NUM;
		static const int32 c_mouseButtonNum = 3;
		static Input* s_current;

#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
		static SDL_Cursor* s_cursor;
#else
		#error implement for non sdl
#endif
#endif

		Tags usageTagsGeneral; // allows to define whether something is in use or not - options
		Tags usageTagsPerHand[Hand::MAX];

		bool inputIsGrabbed; // as requested
		bool cursorIsVisible;

		Vector2 mouseWindowLocation;
		Vector2 lastMouseMotion;
		VectorInt2 lastMouseWheelMotion;
		bool mouseButtons[c_mouseButtonNum];
		bool prevMouseButtons[c_mouseButtonNum];
		float mouseButtonPressedTimes[c_mouseButtonNum];
		
		bool keys[c_keyNum];
		bool prevKeys[c_keyNum];
		float keysPressedTime[c_keyNum];
		float prevKeysPressedTime[c_keyNum];

		List<Gamepad*> gamepads;

		bool keyboardIsActive;
		bool mouseIsActive;
		bool mouseButtonIsActive;
		bool gamepadIsActive;

		Input();
		~Input();

		void prepare_for_events();
		void advance(float _deltaTime);
		void post_events();

		void accumulate_mouse_motion(Vector2 _add);
		void accumulate_mouse_wheel_motion(VectorInt2 _add);
		void store_mouse_button(int32 _button, bool _state);
		void store_mouse_window_location(Vector2 const & _loc);

		void initialise_gamepads();

		friend class Gamepad;
	};

	#include "input.inl"

};
