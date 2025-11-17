#include "input.h"

#include "core.h"

#include "video\video2d.h"
#include "video\video3d.h"

#include "..\mainConfig.h"

using namespace System;

Input* Input::s_current = nullptr;
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
SDL_Cursor* Input::s_cursor = nullptr;
#else
#error implement for non sdl
#endif
#endif

Key::Type Key::parse(String const & _value, Key::Type _default)
{
#ifdef AN_STANDARD_INPUT
	if (String::compare_icase(_value, TXT("a"))) return Key::A;
	if (String::compare_icase(_value, TXT("b"))) return Key::B;
	if (String::compare_icase(_value, TXT("c"))) return Key::C;
	if (String::compare_icase(_value, TXT("d"))) return Key::D;
	if (String::compare_icase(_value, TXT("e"))) return Key::E;
	if (String::compare_icase(_value, TXT("f"))) return Key::F;
	if (String::compare_icase(_value, TXT("g"))) return Key::G;
	if (String::compare_icase(_value, TXT("h"))) return Key::H;
	if (String::compare_icase(_value, TXT("i"))) return Key::I;
	if (String::compare_icase(_value, TXT("j"))) return Key::J;
	if (String::compare_icase(_value, TXT("k"))) return Key::K;
	if (String::compare_icase(_value, TXT("l"))) return Key::L;
	if (String::compare_icase(_value, TXT("m"))) return Key::M;
	if (String::compare_icase(_value, TXT("n"))) return Key::N;
	if (String::compare_icase(_value, TXT("o"))) return Key::O;
	if (String::compare_icase(_value, TXT("p"))) return Key::P;
	if (String::compare_icase(_value, TXT("q"))) return Key::Q;
	if (String::compare_icase(_value, TXT("r"))) return Key::R;
	if (String::compare_icase(_value, TXT("s"))) return Key::S;
	if (String::compare_icase(_value, TXT("t"))) return Key::T;
	if (String::compare_icase(_value, TXT("u"))) return Key::U;
	if (String::compare_icase(_value, TXT("v"))) return Key::V;
	if (String::compare_icase(_value, TXT("w"))) return Key::W;
	if (String::compare_icase(_value, TXT("x"))) return Key::X;
	if (String::compare_icase(_value, TXT("y"))) return Key::Y;
	if (String::compare_icase(_value, TXT("z"))) return Key::Z;
	if (String::compare_icase(_value, TXT("f1"))) return Key::F1;
	if (String::compare_icase(_value, TXT("f2"))) return Key::F2;
	if (String::compare_icase(_value, TXT("f3"))) return Key::F3;
	if (String::compare_icase(_value, TXT("f4"))) return Key::F4;
	if (String::compare_icase(_value, TXT("f5"))) return Key::F5;
	if (String::compare_icase(_value, TXT("f6"))) return Key::F6;
	if (String::compare_icase(_value, TXT("f7"))) return Key::F7;
	if (String::compare_icase(_value, TXT("f8"))) return Key::F8;
	if (String::compare_icase(_value, TXT("f9"))) return Key::F9;
	if (String::compare_icase(_value, TXT("f10"))) return Key::F10;
	if (String::compare_icase(_value, TXT("f11"))) return Key::F11;
	if (String::compare_icase(_value, TXT("f12"))) return Key::F12;
	if (String::compare_icase(_value, TXT("leftShift"))) return Key::LeftShift;
	if (String::compare_icase(_value, TXT("leftCtrl"))) return Key::LeftCtrl;
	if (String::compare_icase(_value, TXT("leftAlt"))) return Key::LeftAlt;
	if (String::compare_icase(_value, TXT("rightShift"))) return Key::RightShift;
	if (String::compare_icase(_value, TXT("rightCtrl"))) return Key::RightCtrl;
	if (String::compare_icase(_value, TXT("rightAlt"))) return Key::RightAlt;
	if (String::compare_icase(_value, TXT("space"))) return Key::Space;
	if (String::compare_icase(_value, TXT("enter")) ||
		String::compare_icase(_value, TXT("return"))) return Key::Return;
	if (String::compare_icase(_value, TXT("leftArrow"))) return Key::LeftArrow;
	if (String::compare_icase(_value, TXT("rightArrow"))) return Key::RightArrow;
	if (String::compare_icase(_value, TXT("downArrow"))) return Key::DownArrow;
	if (String::compare_icase(_value, TXT("upArrow"))) return Key::UpArrow;
	if (String::compare_icase(_value, TXT("esc")) ||
		String::compare_icase(_value, TXT("escape"))) return Key::Esc;
	if (String::compare_icase(_value, TXT("tab"))) return Key::Tab;
	if (String::compare_icase(_value, TXT("backspace"))) return Key::Backspace;
	if (String::compare_icase(_value, TXT("delete"))) return Key::Delete;
	if (String::compare_icase(_value, TXT("tilde"))) return Key::Tilde;
	if (String::compare_icase(_value, TXT("pageDown"))) return Key::PageDown;
	if (String::compare_icase(_value, TXT("pageUp"))) return Key::PageUp;
	if (String::compare_icase(_value, TXT("home"))) return Key::Home;
	if (String::compare_icase(_value, TXT("end"))) return Key::End;
	if (String::compare_icase(_value, TXT("minus"))) return Key::Minus;
	if (String::compare_icase(_value, TXT("equals"))) return Key::Equals;
	if (String::compare_icase(_value, TXT("comma"))) return Key::Comma;
	if (String::compare_icase(_value, TXT("period"))) return Key::Period;
	if (String::compare_icase(_value, TXT("semicolon"))) return Key::Semicolon;
	if (String::compare_icase(_value, TXT("1"))) return Key::N1;
	if (String::compare_icase(_value, TXT("2"))) return Key::N2;
	if (String::compare_icase(_value, TXT("3"))) return Key::N3;
	if (String::compare_icase(_value, TXT("4"))) return Key::N4;
	if (String::compare_icase(_value, TXT("5"))) return Key::N5;
	if (String::compare_icase(_value, TXT("6"))) return Key::N6;
	if (String::compare_icase(_value, TXT("7"))) return Key::N7;
	if (String::compare_icase(_value, TXT("8"))) return Key::N8;
	if (String::compare_icase(_value, TXT("9"))) return Key::N9;
	if (String::compare_icase(_value, TXT("0"))) return Key::N0;
	if (String::compare_icase(_value, TXT("kp_1"))) return Key::KP_N1;
	if (String::compare_icase(_value, TXT("kp_2"))) return Key::KP_N2;
	if (String::compare_icase(_value, TXT("kp_3"))) return Key::KP_N3;
	if (String::compare_icase(_value, TXT("kp_4"))) return Key::KP_N4;
	if (String::compare_icase(_value, TXT("kp_5"))) return Key::KP_N5;
	if (String::compare_icase(_value, TXT("kp_6"))) return Key::KP_N6;
	if (String::compare_icase(_value, TXT("kp_7"))) return Key::KP_N7;
	if (String::compare_icase(_value, TXT("kp_8"))) return Key::KP_N8;
	if (String::compare_icase(_value, TXT("kp_9"))) return Key::KP_N9;
	if (String::compare_icase(_value, TXT("kp_0"))) return Key::KP_N0;
	if (String::compare_icase(_value, TXT("kp_period"))) return Key::KP_Period;
	if (String::compare_icase(_value, TXT("kp_enter"))) return Key::KP_Enter;
	if (String::compare_icase(_value, TXT("kp_minus"))) return Key::KP_Minus;
	if (String::compare_icase(_value, TXT("kp_plus"))) return Key::KP_Plus;
	if (String::compare_icase(_value, TXT("mouseWheelLeft"))) return Key::MouseWheelLeft;
	if (String::compare_icase(_value, TXT("mouseWheelRight"))) return Key::MouseWheelRight;
	if (String::compare_icase(_value, TXT("mouseWheelDown"))) return Key::MouseWheelDown;
	if (String::compare_icase(_value, TXT("mouseWheelUp"))) return Key::MouseWheelUp;
	if (String::compare_icase(_value, TXT("inUse"))) return Key::InUse;
	if (_value != TXT(""))
	{
		todo_important(TXT("more keys! key \"%S\" is undefined"), _value.to_char());
	}
#endif
	return _default;
}

tchar const * const Key::to_char(Key::Type _key)
{
#ifdef AN_STANDARD_INPUT
	if (_key == Key::A) return TXT("a");
	if (_key == Key::B) return TXT("b");
	if (_key == Key::C) return TXT("c");
	if (_key == Key::D) return TXT("d");
	if (_key == Key::E) return TXT("e");
	if (_key == Key::F) return TXT("f");
	if (_key == Key::G) return TXT("g");
	if (_key == Key::H) return TXT("h");
	if (_key == Key::I) return TXT("i");
	if (_key == Key::J) return TXT("j");
	if (_key == Key::K) return TXT("k");
	if (_key == Key::L) return TXT("l");
	if (_key == Key::M) return TXT("m");
	if (_key == Key::N) return TXT("n");
	if (_key == Key::O) return TXT("o");
	if (_key == Key::P) return TXT("p");
	if (_key == Key::Q) return TXT("q");
	if (_key == Key::R) return TXT("r");
	if (_key == Key::S) return TXT("s");
	if (_key == Key::T) return TXT("t");
	if (_key == Key::U) return TXT("u");
	if (_key == Key::V) return TXT("v");
	if (_key == Key::W) return TXT("w");
	if (_key == Key::X) return TXT("x");
	if (_key == Key::Y) return TXT("y");
	if (_key == Key::Z) return TXT("z");
	if (_key == Key::F1) return TXT("f1");
	if (_key == Key::F2) return TXT("f2");
	if (_key == Key::F3) return TXT("f3");
	if (_key == Key::F4) return TXT("f4");
	if (_key == Key::F5) return TXT("f5");
	if (_key == Key::F6) return TXT("f6");
	if (_key == Key::F7) return TXT("f7");
	if (_key == Key::F8) return TXT("f8");
	if (_key == Key::F9) return TXT("f9");
	if (_key == Key::F10) return TXT("f10");
	if (_key == Key::F11) return TXT("f11");
	if (_key == Key::F12) return TXT("f12");
	if (_key == Key::LeftShift) return TXT("leftShift");
	if (_key == Key::LeftCtrl) return TXT("leftCtrl");
	if (_key == Key::LeftAlt) return TXT("leftAlt");
	if (_key == Key::RightShift) return TXT("rightShift");
	if (_key == Key::RightCtrl) return TXT("rightCtrl");
	if (_key == Key::RightAlt) return TXT("rightAlt");
	if (_key == Key::Space) return TXT("space");
	if (_key == Key::Return) return TXT("return");
	if (_key == Key::LeftArrow) return TXT("leftArrow");
	if (_key == Key::RightArrow) return TXT("rightArrow");
	if (_key == Key::DownArrow) return TXT("downArrow");
	if (_key == Key::UpArrow) return TXT("upArrow");
	if (_key == Key::Esc) return TXT("escape");
	if (_key == Key::Tab) return TXT("tab");
	if (_key == Key::Backspace) return TXT("backspace");
	if (_key == Key::Delete) return TXT("delete");
	if (_key == Key::Tilde) return TXT("tilde");
	if (_key == Key::PageDown) return TXT("pageDown");
	if (_key == Key::PageUp) return TXT("pageUp");
	if (_key == Key::Home) return TXT("home");
	if (_key == Key::End) return TXT("end");
	if (_key == Key::Minus) return TXT("minus");
	if (_key == Key::Equals) return TXT("equals");
	if (_key == Key::Comma) return TXT("comma");
	if (_key == Key::Period) return TXT("period");
	if (_key == Key::Semicolon) return TXT("semicolon");
	if (_key == Key::N1) return TXT("1");
	if (_key == Key::N2) return TXT("2");
	if (_key == Key::N3) return TXT("3");
	if (_key == Key::N4) return TXT("4");
	if (_key == Key::N5) return TXT("5");
	if (_key == Key::N6) return TXT("6");
	if (_key == Key::N7) return TXT("7");
	if (_key == Key::N8) return TXT("8");
	if (_key == Key::N9) return TXT("9");
	if (_key == Key::N0) return TXT("0");
	if (_key == Key::KP_N1) return TXT("kp_1");
	if (_key == Key::KP_N2) return TXT("kp_2");
	if (_key == Key::KP_N3) return TXT("kp_3");
	if (_key == Key::KP_N4) return TXT("kp_4");
	if (_key == Key::KP_N5) return TXT("kp_5");
	if (_key == Key::KP_N6) return TXT("kp_6");
	if (_key == Key::KP_N7) return TXT("kp_7");
	if (_key == Key::KP_N8) return TXT("kp_8");
	if (_key == Key::KP_N9) return TXT("kp_9");
	if (_key == Key::KP_N0) return TXT("kp_0");
	if (_key == Key::KP_Period) return TXT("kp_period");
	if (_key == Key::KP_Enter) return TXT("kp_enter");
	if (_key == Key::KP_Minus) return TXT("kp_minus");
	if (_key == Key::KP_Plus) return TXT("kp_plus");
	if (_key == Key::MouseWheelLeft) return TXT("mouseWheelLeft");
	if (_key == Key::MouseWheelRight) return TXT("mouseWheelRight");
	if (_key == Key::MouseWheelDown) return TXT("mouseWheelDown");
	if (_key == Key::MouseWheelUp) return TXT("mouseWheelUp");
	if (_key == Key::InUse) return TXT("inUse");
	if (_key != Key::None)
	{
		todo_important(TXT("more keys! key is undefined"));
	}
#endif
	return TXT("");
}

tchar const * const Key::to_ui_char(Key::Type _key)
{
#ifdef AN_STANDARD_INPUT
	if (_key == Key::A) return TXT("a");
	if (_key == Key::B) return TXT("b");
	if (_key == Key::C) return TXT("c");
	if (_key == Key::D) return TXT("d");
	if (_key == Key::E) return TXT("e");
	if (_key == Key::F) return TXT("f");
	if (_key == Key::G) return TXT("g");
	if (_key == Key::H) return TXT("h");
	if (_key == Key::I) return TXT("i");
	if (_key == Key::J) return TXT("j");
	if (_key == Key::K) return TXT("k");
	if (_key == Key::L) return TXT("l");
	if (_key == Key::M) return TXT("m");
	if (_key == Key::N) return TXT("n");
	if (_key == Key::O) return TXT("o");
	if (_key == Key::P) return TXT("p");
	if (_key == Key::Q) return TXT("q");
	if (_key == Key::R) return TXT("r");
	if (_key == Key::S) return TXT("s");
	if (_key == Key::T) return TXT("t");
	if (_key == Key::U) return TXT("u");
	if (_key == Key::V) return TXT("v");
	if (_key == Key::W) return TXT("w");
	if (_key == Key::X) return TXT("x");
	if (_key == Key::Y) return TXT("y");
	if (_key == Key::Z) return TXT("z");
	if (_key == Key::F1) return TXT("f1");
	if (_key == Key::F2) return TXT("f2");
	if (_key == Key::F3) return TXT("f3");
	if (_key == Key::F4) return TXT("f4");
	if (_key == Key::F5) return TXT("f5");
	if (_key == Key::F6) return TXT("f6");
	if (_key == Key::F7) return TXT("f7");
	if (_key == Key::F8) return TXT("f8");
	if (_key == Key::F9) return TXT("f9");
	if (_key == Key::F10) return TXT("f10");
	if (_key == Key::F11) return TXT("f11");
	if (_key == Key::F12) return TXT("f12");
	if (_key == Key::LeftShift) return TXT("l shift");
	if (_key == Key::LeftCtrl) return TXT("l ctrl");
	if (_key == Key::RightShift) return TXT("r shift");
	if (_key == Key::RightCtrl) return TXT("r ctrl");
	if (_key == Key::Space) return TXT("space");
	if (_key == Key::Return) return TXT("return");
	if (_key == Key::LeftArrow) return TXT("left");
	if (_key == Key::RightArrow) return TXT("right");
	if (_key == Key::DownArrow) return TXT("down");
	if (_key == Key::UpArrow) return TXT("up");
	if (_key == Key::Esc) return TXT("esc");
	if (_key == Key::Tab) return TXT("tab");
	if (_key == Key::Backspace) return TXT("backspace");
	if (_key == Key::Delete) return TXT("delete");
	if (_key == Key::Tilde) return TXT("~");
	if (_key == Key::PageDown) return TXT("pg dn");
	if (_key == Key::PageUp) return TXT("pg up");
	if (_key == Key::Home) return TXT("home");
	if (_key == Key::End) return TXT("end");
	if (_key == Key::Minus) return TXT("-");
	if (_key == Key::Equals) return TXT("=");
	if (_key == Key::Comma) return TXT(",");
	if (_key == Key::Period) return TXT(".");
	if (_key == Key::Semicolon) return TXT(";");
	if (_key == Key::N1) return TXT("1");
	if (_key == Key::N2) return TXT("2");
	if (_key == Key::N3) return TXT("3");
	if (_key == Key::N4) return TXT("4");
	if (_key == Key::N5) return TXT("5");
	if (_key == Key::N6) return TXT("6");
	if (_key == Key::N7) return TXT("7");
	if (_key == Key::N8) return TXT("8");
	if (_key == Key::N9) return TXT("9");
	if (_key == Key::N0) return TXT("0");
	if (_key == Key::KP_N1) return TXT("kp 1");
	if (_key == Key::KP_N2) return TXT("kp 2");
	if (_key == Key::KP_N3) return TXT("kp 3");
	if (_key == Key::KP_N4) return TXT("kp 4");
	if (_key == Key::KP_N5) return TXT("kp 5");
	if (_key == Key::KP_N6) return TXT("kp 6");
	if (_key == Key::KP_N7) return TXT("kp 7");
	if (_key == Key::KP_N8) return TXT("kp 8");
	if (_key == Key::KP_N9) return TXT("kp 9");
	if (_key == Key::KP_N0) return TXT("kp 0");
	if (_key == Key::KP_Period) return TXT("kp .");
	if (_key == Key::KP_Enter) return TXT("kp enter");
	if (_key == Key::KP_Minus) return TXT("kp -");
	if (_key == Key::KP_Plus) return TXT("kp +");
	if (_key == Key::MouseWheelLeft) return TXT("mw left");
	if (_key == Key::MouseWheelRight) return TXT("mw right");
	if (_key == Key::MouseWheelDown) return TXT("mw down");
	if (_key == Key::MouseWheelUp) return TXT("mw up");
	if (_key == Key::InUse) return TXT("inUse");
	if (_key != Key::None)
	{
		todo_important(TXT("more keys! key is undefined"));
	}
#endif
	return TXT("");
}

//

void Input::create()
{
	an_assert(s_current == nullptr);
	s_current = new Input();
}

void Input::terminate()
{
	delete_and_clear(s_current);
}

Input::Input()
: inputIsGrabbed(false)
, cursorIsVisible(true)
, mouseWindowLocation(Vector2::zero)
, lastMouseMotion(Vector2::zero)
, lastMouseWheelMotion(VectorInt2::zero)
, keyboardIsActive(false)
, mouseIsActive(false)
, mouseButtonIsActive(false)
, gamepadIsActive(false)
{
	for (int i = 0; i < 3; ++ i)
	{
		mouseButtons[i] = false;
		prevMouseButtons[i] = false;
	}
	init_cursor();
	grab(inputIsGrabbed);
	initialise_gamepads();
}

Input::~Input()
{
#ifdef AN_STANDARD_INPUT
	if (s_cursor)
	{
#ifdef AN_SDL
		SDL_FreeCursor(s_cursor);
#else
		#error implement for non sdl
#endif
		s_cursor = nullptr;
	}
#endif
	while (!gamepads.is_empty())
	{
		delete gamepads.get_first();
	}
}

void Input::init_cursor()
{
#ifdef AN_STANDARD_INPUT
	if (s_cursor)
	{
#ifdef AN_SDL
		SDL_SetCursor(s_cursor);
#else
		#error implement for non sdl
#endif
		return;
	}

	/**
	 *	NOTE: taken from https://wiki.libsdl.org/SDL_SetCursor
	 */

	static const tchar *arrow[] = {
		/* width height num_colors chars_per_pixel */
		TXT("    32    32        3            1"),
		/* colors */
		TXT("X c #000000"),
		TXT(". c #ffffff"),
		TXT("  c None"),
		/* pixels */
		TXT("X                               "),
		TXT("XX                              "),
		TXT("X.X                             "),
		TXT("X..X                            "),
		TXT("X...X                           "),
		TXT("X....X                          "),
		TXT("X.....X                         "),
		TXT("X......X                        "),
		TXT("X.......X                       "),
		TXT("X........X                      "),
		TXT("X.....XXXXX                     "),
		TXT("X..X..X                         "),
		TXT("X.X X..X                        "),
		TXT("XX  X..X                        "),
		TXT("X    X..X                       "),
		TXT("     X..X                       "),
		TXT("      X..X                      "),
		TXT("      X..X                      "),
		TXT("       XX                       "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("                                "),
		TXT("0,0"),
	};

	int i, row, col;
	Uint8 data[4 * 32];
	Uint8 mask[4 * 32];
	int hot_x, hot_y;

	i = -1;
	for (row = 0; row<32; ++row)
	{
		for (col = 0; col<32; ++col)
		{
			if (col % 8)
			{
				data[i] <<= 1;
				mask[i] <<= 1;
			}
			else
			{
				++i;
				data[i] = mask[i] = 0;
			}
			switch (arrow[4 + row][col])
			{
			case 'X':
				data[i] |= 0x01;
				mask[i] |= 0x01;
				break;
			case '.':
				mask[i] |= 0x01;
				break;
			case ' ':
				break;
			}
		}
	}
	tscanf_s(arrow[4 + row], TXT("%d,%d"), &hot_x, &hot_y);
#ifdef AN_SDL
	s_cursor = SDL_CreateCursor(data, mask, 32, 32, hot_x, hot_y);
	SDL_SetCursor(s_cursor);
#else
	#error implement for non sdl
#endif
#endif
}

void Input::show_cursor(bool _show)
{
	cursorIsVisible = _show;
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	SDL_ShowCursor(_show? 1 : 0);
#else
	#error implement for non sdl
#endif
#endif
	if (_show)
	{
		init_cursor();
	}
}
		
void Input::grab(bool _grab)
{
	inputIsGrabbed = _grab;
	update_grab();
}

void Input::update_grab()
{
	if (Core::has_focus())
	{
		if (inputIsGrabbed)
		{
			show_cursor(false);
		}
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
		SDL_SetRelativeMouseMode(inputIsGrabbed ? SDL_TRUE : SDL_FALSE);
#else
		#error implement for non sdl
#endif
#endif
		if (Video2D::get())
		{
			Video2D::get()->update_grab_input(inputIsGrabbed);
		}
		if (Video3D::get())
		{
			Video3D::get()->update_grab_input(inputIsGrabbed);
		}
		if (!inputIsGrabbed)
		{
			show_cursor(true);
		}
	}
	else
	{
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
		SDL_SetRelativeMouseMode(SDL_FALSE);
#else
		#error implement for non sdl
#endif
#endif
		if (Video2D::get())
		{
			Video2D::get()->update_grab_input(false);
		}
		if (Video3D::get())
		{
			Video3D::get()->update_grab_input(false);
		}
		show_cursor(true);
	}
}

void Input::prepare_for_events()
{
	lastMouseMotion = Vector2::zero;
	lastMouseWheelMotion = VectorInt2::zero;
	for (int32 i = 0; i < c_mouseButtonNum; ++i)
	{
		prevMouseButtons[i] = mouseButtons[i];
	}
}

void Input::advance(float _deltaTime)
{
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	int sdlKeyNum;
	uint8 const * inputKeys = SDL_GetKeyboardState(&sdlKeyNum);

	uint32 keyNum = min<uint32>(Key::SPECIAL, min<uint32>(sdlKeyNum, c_keyNum));
#else
	#error implement for non sdl
#endif
#endif

	keyboardIsActive = false;
	mouseIsActive = false;
	mouseButtonIsActive = false;
	gamepadIsActive = false;

#ifdef AN_STANDARD_INPUT
	{	// standard keys
		bool* prevKey = prevKeys;
		float* prevKeyPressedTime = prevKeysPressedTime;
		bool* currentKey = keys;
		float* currentKeyPressedTime = keysPressedTime;
		uint8 const * inputKey = inputKeys;
		for (uint32 i = 0; i < keyNum; ++i, ++prevKey, ++currentKey, ++inputKey, ++prevKeyPressedTime, ++currentKeyPressedTime)
		{
			*prevKey = *currentKey;
			*prevKeyPressedTime = *currentKeyPressedTime;
			*currentKey = *inputKey != 0;
			*currentKeyPressedTime = !*prevKey ? 0.0f : *currentKeyPressedTime + _deltaTime;
			keyboardIsActive |= *inputKey != 0;
		}
	}

	{	// mouse buttons
		bool const * mouseButton = mouseButtons;
		float * mouseButtonPressedTime = mouseButtonPressedTimes;
		for (int32 i = 0; i < c_mouseButtonNum; ++i, ++mouseButton, ++mouseButtonPressedTime)
		{
			if (*mouseButton)
			{
				*mouseButtonPressedTime += _deltaTime;
			}
		}
	}

#ifdef AN_SDL
	SDL_JoystickUpdate();
	for_every_ptr(gamepad, gamepads)
	{
		gamepad->advance(_deltaTime);
	}
#else
	#error implement for non sdl
#endif

	update_gamepad_is_active();
#endif
}

void Input::post_events()
{
#ifdef AN_STANDARD_INPUT
	{	// special keys
		const int specialKeysNum = Key::NUM - Key::SPECIAL;
		ArrayStatic<uint8, specialKeysNum> specialInputKeys;
		specialInputKeys.set_size(specialKeysNum);
		int offset = Key::SPECIAL + 1;
		specialInputKeys[Key::MouseWheelLeft - offset] = lastMouseWheelMotion.x < 0;
		specialInputKeys[Key::MouseWheelRight - offset] = lastMouseWheelMotion.x > 0;
		specialInputKeys[Key::MouseWheelDown - offset] = lastMouseWheelMotion.y < 0;
		specialInputKeys[Key::MouseWheelUp - offset] = lastMouseWheelMotion.y > 0;
		specialInputKeys[Key::InUse - offset] = true; // always true
		bool* prevKey = &prevKeys[offset];
		bool* currentKey = &keys[offset];
		uint8 const * inputKey = specialInputKeys.get_data();
		for (uint32 i = 0; i < specialKeysNum; ++i, ++prevKey, ++currentKey, ++inputKey)
		{
			*prevKey = *currentKey;
			*currentKey = *inputKey != 0;
			//keyboardIsActive |= *inputKey != 0;
		}
	}
#endif
}

void Input::update_gamepad_is_active()
{
	gamepadIsActive = false;
	for_every_ptr(gamepad, gamepads)
	{
		gamepadIsActive |= gamepad->is_active();
	}
}

void Input::store_mouse_window_location(Vector2 const & _loc)
{
	mouseWindowLocation = _loc;
}

void Input::accumulate_mouse_motion(Vector2 _add)
{
	lastMouseMotion += _add;
	mouseIsActive |= ! _add.is_zero();
}

void Input::accumulate_mouse_wheel_motion(VectorInt2 _add)
{
	lastMouseWheelMotion += _add;
	mouseIsActive |= !_add.is_zero();
}

void Input::store_mouse_button(int32 _button, bool _state)
{
	if (! is_mouse_button_valid(_button)) { return; }
	mouseButtons[_button] = _state;
	mouseIsActive |= _state;
	mouseButtonIsActive |= _state;
	if (_state)
	{
		mouseButtonPressedTimes[_button] = 0.0f;
	}
}

void Input::initialise_gamepads()
{
#ifdef AN_STANDARD_INPUT
#ifdef AN_SDL
	SDL_JoystickEventState(SDL_ENABLE);
	for_count(int, idx, SDL_NumJoysticks())
	{
		if (Gamepad::can_create_gamepad(idx, this))
		{
			// will add itself to gamepads
			Gamepad::create(idx, this);
		}
	}
#else
	#error implement for non sdl
#endif
#endif
}

Gamepad* Input::get_gamepad_by_system_id(int _systemID) const
{
	for_every_ptr(gamepad, gamepads)
	{
		if (gamepad->get_system_id() == _systemID)
		{
			return gamepad;
		}
	}

	return nullptr;
}

Gamepad* Input::get_gamepad(int _index) const
{
	for_every_ptr(gamepad, gamepads)
	{
		if (gamepad->get_index() == _index)
		{
			return gamepad;
		}
	}

	return nullptr;
}

bool Input::is_any_button_or_key_pressed() const
{
	for_every_ptr(gamepad, gamepads)
	{
		if (gamepad->is_any_button_pressed())
		{
			return true;
		}
	}

	bool const * key = keys;
	for_count(int, i, c_keyNum)
	{
		if (*key)
		{
			return true;
		}
		++key;
	}

	bool const * mouseButton = mouseButtons;
	for_count(int, i, c_mouseButtonNum)
	{
		if (*mouseButton)
		{
			return true;
		}
		++mouseButton;
	}

	return false;
}

Tags const& Input::get_usage_tags(Optional<Hand::Type> const& _hand) const
{
	if (_hand.is_set())
	{
		return usageTagsPerHand[_hand.get()];
	}
	else
	{
		return usageTagsGeneral;
	}
}

void Input::set_usage_tag(Name const& _tag, Optional<Hand::Type> const& _hand)
{
	if (_hand.is_set())
	{
		usageTagsPerHand[_hand.get()].set_tag(_tag);
	}
	else
	{
		usageTagsGeneral.set_tag(_tag);
		usageTagsPerHand[Hand::Left].set_tag(_tag);
		usageTagsPerHand[Hand::Right].set_tag(_tag);
	}
}

void Input::remove_usage_tag(Name const& _tag, Optional<Hand::Type> const& _hand)
{
	if (_hand.is_set())
	{
		usageTagsPerHand[_hand.get()].remove_tag(_tag);
	}
	else
	{
		usageTagsGeneral.remove_tag(_tag);
		usageTagsPerHand[Hand::Left].remove_tag(_tag);
		usageTagsPerHand[Hand::Right].remove_tag(_tag);
	}
}

void Input::set_usage_tags_from(Tags const& _tags, Optional<Hand::Type> const& _hand)
{
	if (_tags.is_empty())
	{
		return;
	}
	if (_hand.is_set())
	{
		usageTagsPerHand[_hand.get()].set_tags_from(_tags);
	}
	else
	{
		usageTagsGeneral.set_tags_from(_tags);
		usageTagsPerHand[Hand::Left].set_tags_from(_tags);
		usageTagsPerHand[Hand::Right].set_tags_from(_tags);
	}
}

void Input::remove_usage_tags(Tags const& _tags, Optional<Hand::Type> const& _hand)
{
	if (_tags.is_empty())
	{
		return;
	}
	if (_hand.is_set())
	{
		usageTagsPerHand[_hand.get()].remove_tags(_tags);
	}
	else
	{
		usageTagsGeneral.remove_tags(_tags);
		usageTagsPerHand[Hand::Left].remove_tags(_tags);
		usageTagsPerHand[Hand::Right].remove_tags(_tags);
	}
}

bool Input::has_key_been_hold_pressed(Key::Type _key) const
{
	an_assert(s_current != nullptr);
	if (s_current->keys[_key])
	{
		float time = s_current->keysPressedTime[_key];
		if (time == 0.0f) // just started
		{
			return !s_current->prevKeys[_key];
		}
		float prevTime = s_current->prevKeysPressedTime[_key];
		if (prevTime < time)
		{
			struct CheckMod
			{
				static bool check(float _startAt, float _mod, float _time, float _prevTime)
				{
					if (_time >= _startAt)
					{
						float t = mod(_time - _startAt, _mod);
						float p = mod(_prevTime - _startAt, _mod);
						if (t < p)
						{
							return true;
						}
					}
					return false;
				}
			};
			if (CheckMod::check(1.0f, 0.50f, time, prevTime)) return true;
			if (CheckMod::check(2.0f, 0.20f, time, prevTime)) return true;
			if (CheckMod::check(5.0f, 0.10f, time, prevTime)) return true;
		}
		return false;
	}
	return false;
}

