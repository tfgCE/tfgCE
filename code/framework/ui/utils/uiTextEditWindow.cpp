#include "uiTextEditWindow.h"

#include "..\uiCanvas.h"
#include "..\uiCanvasButton.h"
#include "..\uiCanvasWindow.h"

#include "..\..\..\core\concurrency\scopedSpinLock.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#define CARET '|'

//

using namespace Framework;
using namespace UI;

//

DEFINE_STATIC_NAME(textEdit_window);
DEFINE_STATIC_NAME(textEdit_text);
DEFINE_STATIC_NAME(textEdit_shift);

//

void Utils::edit_text(Canvas* _c, tchar const* _info, REF_ String& _text, std::function<void()> _on_done)
{
	// don't clear

	UI::Utils::TextEditWindow textEdit;

	textEdit.step_1_create_window(_c)->set_title(_info);
	textEdit.step_2_setup(_text,
		[_on_done, &_text](String const& _newText)
		{
			_text = _newText;
			if (_on_done)
			{
				_on_done();
			}
		},
		[_on_done]()
		{
			if (_on_done)
			{
				_on_done();
			}
		});
	textEdit.step_3_finalise();
}

void Utils::edit_text(Canvas* _c, tchar const* _info, String const & _text, std::function<void(String const & _text)> _on_accepted, std::function<void()> _on_canceled)
{
	// don't clear

	UI::Utils::TextEditWindow textEdit;

	textEdit.step_1_create_window(_c)->set_title(_info);
	textEdit.step_2_setup(_text,
		[_on_accepted](String const& _newText)
		{
			if (_on_accepted)
			{
				_on_accepted(_newText);
			}
		},
		[_on_canceled]()
		{
			if (_on_canceled)
			{
				_on_canceled();
			}
		});
	textEdit.step_3_finalise();
}

//

CanvasWindow* Utils::TextEditWindow::step_1_create_window(Canvas * _canvas, Optional<float> const& _textScale, Optional<float> const & _buttonTextScale)
{
	canvas = _canvas;
	textScale = _textScale.get(2.0f);
	buttonTextScale = _buttonTextScale.get(textScale * 1.5f);

	window = new UI::CanvasWindow();
	window->set_id(NAME(textEdit_window));
	window->set_closable(false)->set_movable(false)->set_modal(true);
	canvas->add(window);

	return window;
}

#ifdef AN_STANDARD_INPUT
#define SYSTEM_KEY(key) System::Key::key
#else
#define SYSTEM_KEY(key) System::Key::None
#endif

CanvasWindow* Utils::TextEditWindow::step_2_setup(String const& _textToEdit, std::function<void(String const &)> _on_ok, std::function<void()> _on_back)
{
	float height = canvas->get_setup().buttonDefaultHeight * buttonTextScale;
	float width = height;
	float keyWidth = width;
	spacing = height * 0.1f;

	// check if fits
	{
		Vector2 predictedSize;
		predictedSize.x = 13.0f * (keyWidth + spacing);
		predictedSize.y = 6.0f * (height + spacing) + canvas->get_setup().windowTitleBarHeight;
		float scale = 1.0f;
		scale = min(scale, canvas->get_logical_size().x / predictedSize.x);
		scale = min(scale, canvas->get_logical_size().y / predictedSize.y);

		textScale *= scale;
		buttonTextScale *= scale;
		height *= scale;
		width *= scale;
		keyWidth *= scale;
		spacing *= scale;
	}

	// text box
	// 1-0 backspace
	// q-p return
	// a-l ;
	// z-m , .
	// shift space    arrow keys <- ->

	// six rows
	Vector2 lineAt = Vector2(width * 0.5f, spacing + (height + spacing) * 5.0f + height * 0.5f);

	{
		Vector2 at = lineAt;

		String text = _textToEdit;
		text += CARET;
		auto* b = add_button(REF_ at, text.to_char(), (width + spacing) * 13, textScale);
		b->set_id(NAME(textEdit_text));
		b->set_text_only();
		b->set_alignment(Vector2(0.0f, 0.5f));

		lineAt.y -= height + spacing;
	}

	float widthPerLine = width * 0.5f;
	{
		Vector2 at = lineAt;

		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N1), '1');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N2), '2');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N3), '3');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N4), '4');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N5), '5');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N6), '6');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N7), '7');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N8), '8');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N9), '9');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N0), '0');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Equals), '=');
		add_button(REF_ at, TXT("<<"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Backspace)
#endif
			->set_on_press([](UI::ActContext const& _context)
				{
					remove_char(_context.canvas, -1);
				})
#ifdef AN_STANDARD_INPUT
			->add_shortcut(System::Key::Delete, NP, [](UI::ActContext const& _context)
				{
					remove_char(_context.canvas, 1);
				})
#endif
				;

		lineAt.y -= height + spacing;
	}

	{
		Vector2 at = lineAt;
		at.x += widthPerLine;

		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Q), 'q', 'Q');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(W), 'w', 'W');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(E), 'e', 'E');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(R), 'r', 'R');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(T), 't', 'T');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Y), 'y', 'Y');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(U), 'u', 'U');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(I), 'i', 'I');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(O), 'o', 'O');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(P), 'p', 'P');
		add_button(REF_ at, TXT("clear"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Backspace, UI::ShortcutParams().with_shift())
#endif
			->set_on_press([](UI::ActContext const& _context)
				{
					clear_box(_context.canvas);
				});

		lineAt.y -= height + spacing;
	}

	{
		Vector2 at = lineAt;
		at.x += widthPerLine * 2.0f;

		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(A), 'a', 'A');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(S), 's', 'S');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(D), 'd', 'D');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(F), 'f', 'F');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(G), 'g', 'G');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(H), 'h', 'H');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(J), 'j', 'J');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(K), 'k', 'K');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(L), 'l', 'L');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Semicolon), ';');
		add_button(REF_ at, TXT("enter"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Return)
#endif
			->set_on_press([_on_ok](UI::ActContext const& _context)
				{
					if (auto* b = fast_cast<CanvasButton>(_context.canvas->find_by_id(NAME(textEdit_text))))
					{
						String text = b->get_caption();
						int at = text.find_last_of(CARET);
						if (at != NONE)
						{
							text = text.get_left(at) + text.get_sub(at + 1);
						}
						if (_on_ok)
						{
							_on_ok(text);
						}
						remove_window(_context.canvas);
					}
				});

		lineAt.y -= height + spacing;
	}

	{
		Vector2 at = lineAt;
		at.x += widthPerLine * 3.0f;

		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Z), 'z', 'Z');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(X), 'x', 'X');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(C), 'c', 'C');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(V), 'v', 'V');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(B), 'b', 'B');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(N), 'n', 'N');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(M), 'm', 'M');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Comma), ',');
		add_key_button(REF_ at, keyWidth, SYSTEM_KEY(Period), '.');
		add_button(REF_ at, TXT("<-"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::LeftArrow)
#endif
			->set_on_press([](UI::ActContext const& _context)
				{
					move_caret(_context.canvas, -1);
				})
#ifdef AN_STANDARD_INPUT
			->add_shortcut(System::Key::Home, NP, [](UI::ActContext const& _context)
				{
					move_caret_to_end(_context.canvas, -1);
				})
#endif
				;
		add_button(REF_ at, TXT("->"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::RightArrow)
#endif
			->set_on_press([](UI::ActContext const& _context)
				{
					move_caret(_context.canvas, 1);
				})
#ifdef AN_STANDARD_INPUT
			->add_shortcut(System::Key::End, NP, [](UI::ActContext const& _context)
				{
					move_caret_to_end(_context.canvas, 1);
				})
#endif
				;

		lineAt.y -= height + spacing;
	}

	{
		Vector2 at = lineAt;
		add_button(REF_ at, TXT("shift"), width * 3.0f)
			->set_on_press([](UI::ActContext const& _context)
				{
					if (auto* b = fast_cast<UI::CanvasButton>(_context.element))
					{
						b->set_highlighted(!b->is_highlighted());
					}
				})
#ifdef AN_STANDARD_INPUT
			->add_shortcut(System::Key::LeftShift, UI::ShortcutParams().with_shift(), [](UI::ActContext const& _context)
				{
					if (auto* b = fast_cast<UI::CanvasButton>(cast_to_nonconst(_context.element))) { b->set_highlighted(true); }
				})
			->add_shortcut(System::Key::RightShift, UI::ShortcutParams().with_shift(), [](UI::ActContext const& _context)
				{
					if (auto* b = fast_cast<UI::CanvasButton>(cast_to_nonconst(_context.element))) { b->set_highlighted(true); }
				})
			->add_shortcut(System::Key::LeftShift, UI::ShortcutParams().act_on_release(), [](UI::ActContext const& _context)
				{
					if (auto* b = fast_cast<UI::CanvasButton>(cast_to_nonconst(_context.element))) { b->set_highlighted(false); }
				})
			->add_shortcut(System::Key::RightShift, UI::ShortcutParams().act_on_release(), [](UI::ActContext const& _context)
				{
					if (auto* b = fast_cast<UI::CanvasButton>(cast_to_nonconst(_context.element))) { b->set_highlighted(false); }
				})
#endif
				;
		add_button(REF_ at, TXT(" "), width * 5.0f)
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Space)
#endif
			->set_on_press([](UI::ActContext const& _context)
				{
					enter_char(_context.canvas, ' ');
				});
		add_button(REF_ at, TXT("back"))
#ifdef AN_STANDARD_INPUT
			->set_shortcut(System::Key::Esc)
#endif
			->set_on_press([_on_back](UI::ActContext const& _context)
				{
					remove_window(_context.canvas);
					if (_on_back)
					{
						_on_back();
					}
				});

		lineAt.y -= height + spacing;
	}

	return window;
}

CanvasWindow* Utils::TextEditWindow::step_3_finalise()
{
	window->set_size_to_fit_all(canvas, canvas->get_default_spacing());
	window->set_at_pt(canvas, Vector2::half);

	return window;
}

CanvasButton* Utils::TextEditWindow::add_button(REF_ Vector2& at, tchar const* _caption, Optional<float> const& _width, Optional<float> const& _overrideScale)
{
	auto* b = new UI::CanvasButton();
	b->set_caption(_caption);
	b->set_scale(_overrideScale.get(buttonTextScale));
	if (_width.is_set())
	{
		b->set_width(_width.get());
	}
	else
	{
		b->set_auto_width(canvas);
	}
	b->set_height(canvas->get_setup().buttonDefaultHeight * _overrideScale.get(buttonTextScale));
	b->set_at(at + b->get_size() * 0.5f);
	at.x += b->get_size().x;
	at.x += spacing;
	window->add(b);
	return b;
}

CanvasButton* Utils::TextEditWindow::add_key_button(REF_ Vector2& at, Optional<float> const& _width, System::Key::Type _key, tchar _add, tchar _addShifted)
{
	String key;
	key += System::Key::to_ui_char(_key);
	key = key.to_upper();
	auto* b = add_button(REF_ at, key.to_char(), _width);
#ifdef AN_STANDARD_INPUT
	b->set_shortcut(_key);
#endif
	if (_addShifted)
	{
		b->add_shortcut(_key, UI::ShortcutParams().with_shift(), [_addShifted](UI::ActContext const& _context)
			{
				enter_char(_context.canvas, _addShifted);
			});
	}
	b->set_on_press([_add, _addShifted](UI::ActContext const & _context)
		{
			enter_char(_context.canvas, _add, _addShifted);
		});
	return b;
}

void Utils::TextEditWindow::enter_char(Canvas* _canvas, tchar _add, tchar _addShifted)
{
	auto* b = fast_cast<CanvasButton>(_canvas->find_by_id(NAME(textEdit_text)));
	if (!b)
	{
		return;
	}

	if (_addShifted != 0)
	{
		if (auto* b = fast_cast<CanvasButton>(_canvas->find_by_id(NAME(textEdit_shift))))
		{
			if (b->is_highlighted())
			{
				swap(_add, _addShifted);
			}
		}
#ifdef AN_STANDARD_INPUT
		if (auto* input = System::Input::get())
		{
			if (input->is_key_pressed(System::Key::LeftShift) ||
				input->is_key_pressed(System::Key::RightShift))
			{
				swap(_add, _addShifted);
			}
		}
#endif
	}

	String text = b->get_caption();
	int at = text.find_last_of(CARET);
	if (at == NONE)
	{
		text = text + _add + CARET;
	}
	else
	{
		text = text.get_left(at) + _add + text.get_sub(at);
	}
	b->set_caption(text);
}

void Utils::TextEditWindow::remove_char(Canvas* _canvas, int _dir)
{
	auto* b = fast_cast<CanvasButton>(_canvas->find_by_id(NAME(textEdit_text)));
	if (!b)
	{
		return;
	}

	String text = b->get_caption();
	int at = text.find_last_of(CARET);
	if (at == NONE)
	{
		text = text + CARET;
	}
	else
	{
		int keepOff = _dir <= 0 ? -1 : 1;
		int rightOff = _dir <= 0 ? 0 : 2;
		text = text.get_left(max(at + keepOff, 0)) + text.get_sub(at + rightOff);
	}
	b->set_caption(text);
}

void Utils::TextEditWindow::clear_box(Canvas* _canvas)
{
	auto* b = fast_cast<CanvasButton>(_canvas->find_by_id(NAME(textEdit_text)));
	if (!b)
	{
		return;
	}

	String text;
	text += CARET;
	b->set_caption(text);
}

void Utils::TextEditWindow::move_caret(Canvas* _canvas, int _by)
{
	auto* b = fast_cast<CanvasButton>(_canvas->find_by_id(NAME(textEdit_text)));
	if (!b)
	{
		return;
	}

	String text = b->get_caption();
	int at = text.find_last_of(CARET);
	if (at == NONE)
	{
		text = text + CARET;
	}
	else
	{
		// move caret in place
		while (_by < 0 && at > 0)
		{
			swap(text.access_data()[at - 1], text.access_data()[at]);
			++_by;
		}
		int lastAt = text.get_length() - 1;
		while (_by > 0 && at < lastAt)
		{
			swap(text.access_data()[at + 1], text.access_data()[at]);
			--_by;
		}
	}
	b->set_caption(text);
}

void Utils::TextEditWindow::move_caret_to_end(Canvas* _canvas, int _end)
{
	auto* b = fast_cast<CanvasButton>(_canvas->find_by_id(NAME(textEdit_text)));
	if (!b)
	{
		return;
	}

	String text = b->get_caption();
	int at = text.find_last_of(CARET);
	if (at == NONE)
	{
		text = text + CARET;
	}
	else
	{
		// remove caret
		text = text.get_left(at) + text.get_sub(at + 1);
	}
	if (_end < 0)
	{
		text = String::empty() + CARET + text;
	}
	else
	{
		text += CARET;
	}
	b->set_caption(text);
}

void Utils::TextEditWindow::remove_window(Canvas* _canvas)
{
	if (auto* e = _canvas->find_by_id(NAME(textEdit_window)))
	{
		e->remove();
	}
}
