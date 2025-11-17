#include "lhc_enterText.h"

#include "..\loaderHub.h"
#include "..\loaderHubWidget.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_text.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsContinue, TXT("hub; common; continue"));
DEFINE_STATIC_NAME_STR(lsOK, TXT("hub; common; ok"));
DEFINE_STATIC_NAME_STR(lsCancel, TXT("hub; common; cancel"));

DEFINE_STATIC_NAME_STR(lsKeyBackspace, TXT("hub; common; key; backspace"));
DEFINE_STATIC_NAME_STR(lsKeyClear, TXT("hub; common; key; clear"));
DEFINE_STATIC_NAME_STR(lsKeyEnter, TXT("hub; common; key; enter"));
DEFINE_STATIC_NAME_STR(lsKeySpace, TXT("hub; common; key; space"));

//

using namespace Loader;
using namespace HubScreens;

REGISTER_FOR_FAST_CAST(EnterText);

void EnterText::show(Hub* _hub, String const& _info, String const& _text, std::function<void(String const& _text)> _onOK, std::function<void()> _onCancel, bool _continueButtonOnly, bool _asSingleScreen, Keyboard _keyboard)
{
	int width = 50;
	int height = 14;
	
	if (_keyboard == NetworkAddress ||
		_keyboard == NetworkPort)
	{
		width = 20;
	}

	Vector2 ppa = Vector2(24.0f, 24.0f);
	float screenWidthInPixels = (float)(width + 2) * (HubScreen::s_fontSizeInPixels.x * 1.2f);
	float screenHeightInPixels = (float)(height + 2) * (HubScreen::s_fontSizeInPixels.y * 1.2f);
	int infoLineCount = 0;
	if (!_info.is_empty())
	{
		int tempWidth;
		HubWidgets::Text::measure(_hub->get_font(), NP, NP, _info, infoLineCount, tempWidth, (int)screenWidthInPixels);
		screenHeightInPixels += ((float)infoLineCount + 0.5f) * HubScreen::s_fontSizeInPixels.y;
	}
	Vector2 size = Vector2(screenWidthInPixels / ppa.x, screenHeightInPixels / ppa.y);
	EnterText* newEE = new EnterText(_hub, _info, infoLineCount, _text, _onOK, _onCancel, _continueButtonOnly, size, _asSingleScreen ? _hub->get_current_forward() : _hub->get_last_view_rotation() * Rotator3(0.0f, 1.0f, 0.0f), _hub->get_radius() * 0.5f, _keyboard);

	newEE->activate();
	_hub->add_screen(newEE);
}

EnterText::EnterText(Hub* _hub, String const& _info, int _infoLineCount, String const& _text, std::function<void(String const& _text)> _onOK, std::function<void()> _onCancel, bool _continueButtonOnly, Vector2 const& _size, Rotator3 const& _at, float _radius, Keyboard _keyboard)
: HubScreen(_hub, Name::invalid(), _size, _at, _radius)
, info(_info)
, text(_text)
, onOK(_onOK)
, onCancel(_onCancel)
{
	be_modal();

	Vector2 fs = HubScreen::s_fontSizeInPixels;

	Range2 keyboardSpace = mainResolutionInPixels.expanded_by(Vector2(-5.0f, -5.0f));

	float top = mainResolutionInPixels.y.max - fs.y * 0.5f;

	if (! info.is_empty())
	{
		auto* t = new HubWidgets::Text(Name::invalid(), Range2(mainResolutionInPixels.x,
															   Range(top - fs.y * (float)_infoLineCount, top)), info);
		t->alignPt = Vector2(0.5f, 1.0f);
		add_widget(t);

		top -= fs.y * ((float)_infoLineCount + 0.5f);
		keyboardSpace.y.max = top;
	}

	{
		auto* t = new HubWidgets::Text(Name::invalid(), Range2(mainResolutionInPixels.x,
															   Range(top - fs.y * 2.0f, top)), text);
		editWidget = t;
		add_widget(t);

		top -= fs.y * 2.5f;
		keyboardSpace.y.max = top;
	}

	if (_continueButtonOnly)
	{
		String cont = LOC_STR(NAME(lsContinue));
		
		Array<HubScreenButtonInfo> buttons;
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsContinue), [this]() { if (onOK) onOK(text); deactivate(); }).activate_on_trigger_hold());

		float spacing = 2.0f;
		float length = (float)cont.get_length() + 2.0f + spacing * 0.5f;
		add_button_widgets(buttons,
			Vector2(max(mainResolutionInPixels.x.centre() - fs.x * length, mainResolutionInPixels.x.min + fs.x), mainResolutionInPixels.y.min + fs.y * 0.25f),
			Vector2(min(mainResolutionInPixels.x.centre() + fs.x * length, mainResolutionInPixels.x.max - fs.x), mainResolutionInPixels.y.min + fs.y * 2.75f),
			fs.x * spacing);

		keyboardSpace.y.min = mainResolutionInPixels.y.min + fs.y * 3.25f;
	}
	else
	{
		String ok = LOC_STR(NAME(lsOK));
		String cancel = LOC_STR(NAME(lsCancel));

		Array<HubScreenButtonInfo> buttons;
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOK), [this]() { if (onOK) onOK(text); deactivate(); }).activate_on_trigger_hold());
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { if (onCancel) onCancel(); deactivate(); }));

		float spacing = 2.0f;
		float length = (float)max(ok.get_length(), cancel.get_length()) + 4.0f + spacing * 0.5f;
		add_button_widgets(buttons,
			Vector2(max(mainResolutionInPixels.x.centre() - fs.x * length, mainResolutionInPixels.x.min + fs.x), mainResolutionInPixels.y.min + fs.y * 0.25f),
			Vector2(min(mainResolutionInPixels.x.centre() + fs.x * length, mainResolutionInPixels.x.max - fs.x), mainResolutionInPixels.y.min + fs.y * 2.75f),
			fs.x * spacing);

		keyboardSpace.y.min = mainResolutionInPixels.y.min + fs.y * 3.25f;
	}

	update_text();

	{
		struct Key
		{
			tchar key = ' ';
			bool isBackspace = false;
			bool isClear = false;
			bool isEnter = false;
			Key() {}
			Key(tchar _key) : key(_key) {}
			static Key backspace() { Key k; k.isBackspace = true; return k; }
			static Key clear() { Key k; k.isClear = true; return k; }
			static Key enter() { Key k; k.isEnter = true; return k; }
		};

		struct Row
		{
			ArrayStatic<Key,15> keys;

			Row()
			{
				SET_EXTRA_DEBUG_INFO(keys, TXT("EnterText.Row.keys"));
			}
		};

		ArrayStatic<Row, 4> rows; SET_EXTRA_DEBUG_INFO(rows, TXT("EnterText.rows"));
		ArrayStatic<int, 4> rowKeyOffset; SET_EXTRA_DEBUG_INFO(rowKeyOffset, TXT("EnterText.rowKeyOffset"));
		ArrayStatic<int, 4> rowExtendLastKey; SET_EXTRA_DEBUG_INFO(rowExtendLastKey, TXT("EnterText.rowExtendLastKey"));
		int keyNumber = 13;
		if (_keyboard == LettersAndNumbers)
		{
			rows.set_size(4);
			{
				Row& r = rows[0];
				r.keys.push_back(Key('1'));
				r.keys.push_back(Key('2'));
				r.keys.push_back(Key('3'));
				r.keys.push_back(Key('4'));
				r.keys.push_back(Key('5'));
				r.keys.push_back(Key('6'));
				r.keys.push_back(Key('7'));
				r.keys.push_back(Key('8'));
				r.keys.push_back(Key('9'));
				r.keys.push_back(Key('0'));
				r.keys.push_back(Key::backspace());
			}
			{
				Row& r = rows[1];
				r.keys.push_back(Key('q'));
				r.keys.push_back(Key('w'));
				r.keys.push_back(Key('e'));
				r.keys.push_back(Key('r'));
				r.keys.push_back(Key('t'));
				r.keys.push_back(Key('y'));
				r.keys.push_back(Key('u'));
				r.keys.push_back(Key('i'));
				r.keys.push_back(Key('o'));
				r.keys.push_back(Key('p'));
				r.keys.push_back(Key::clear());
			}
			{
				Row& r = rows[2];
				r.keys.push_back(Key('a'));
				r.keys.push_back(Key('s'));
				r.keys.push_back(Key('d'));
				r.keys.push_back(Key('f'));
				r.keys.push_back(Key('g'));
				r.keys.push_back(Key('h'));
				r.keys.push_back(Key('j'));
				r.keys.push_back(Key('k'));
				r.keys.push_back(Key('l'));
				r.keys.push_back(Key::enter());
			}
			{
				Row& r = rows[3];
				r.keys.push_back(Key('z'));
				r.keys.push_back(Key('x'));
				r.keys.push_back(Key('c'));
				r.keys.push_back(Key('v'));
				r.keys.push_back(Key('b'));
				r.keys.push_back(Key('n'));
				r.keys.push_back(Key('m'));
				r.keys.push_back(Key(' '));
			}
		}
		if (_keyboard == NetworkAddress)
		{
			keyNumber = 7;
			rows.set_size(4);
			{
				Row& r = rows[0];
				r.keys.push_back(Key('7'));
				r.keys.push_back(Key('8'));
				r.keys.push_back(Key('9'));
				r.keys.push_back(Key::backspace());
			}
			{
				Row& r = rows[1];
				r.keys.push_back(Key('4'));
				r.keys.push_back(Key('5'));
				r.keys.push_back(Key('6'));
				r.keys.push_back(Key::clear());
			}
			{
				Row& r = rows[2];
				r.keys.push_back(Key('1'));
				r.keys.push_back(Key('2'));
				r.keys.push_back(Key('3'));
				r.keys.push_back(Key::enter());
			}
			{
				Row& r = rows[3];
				r.keys.push_back(Key('.'));
				r.keys.push_back(Key('0'));
			}
			rowExtendLastKey.set_size(4);
			rowExtendLastKey[0] = true;
			rowExtendLastKey[1] = true;
			rowExtendLastKey[2] = true;
			rowExtendLastKey[3] = false;
		}
		if (_keyboard == NetworkPort)
		{
			keyNumber = 7;
			rows.set_size(4);
			{
				Row& r = rows[0];
				r.keys.push_back(Key('7'));
				r.keys.push_back(Key('8'));
				r.keys.push_back(Key('9'));
				r.keys.push_back(Key::backspace());
			}
			{
				Row& r = rows[1];
				r.keys.push_back(Key('4'));
				r.keys.push_back(Key('5'));
				r.keys.push_back(Key('6'));
				r.keys.push_back(Key::clear());
			}
			{
				Row& r = rows[2];
				r.keys.push_back(Key('1'));
				r.keys.push_back(Key('2'));
				r.keys.push_back(Key('3'));
				r.keys.push_back(Key::enter());
			}
			{
				Row& r = rows[3];
				r.keys.push_back(Key('0'));
			}
			rowKeyOffset.set_size(4);
			rowKeyOffset[0] = 0;
			rowKeyOffset[1] = 0;
			rowKeyOffset[2] = 0;
			rowKeyOffset[3] = 1;
			rowExtendLastKey.set_size(4);
			rowExtendLastKey[0] = true;
			rowExtendLastKey[1] = true;
			rowExtendLastKey[2] = true;
			rowExtendLastKey[3] = false;
		}
		float rowMove = round(keyboardSpace.y.length() / 4.0f);
		float spacing = round(rowMove * 0.15f);
		float rowSize = rowMove - spacing;
		float rowOffset = fs.x * 0.7f;
		if (_keyboard != LettersAndNumbers)
		{
			rowOffset = 0.0f;
		}
		float keyMove = round((keyboardSpace.x.length() - 3.0f * rowOffset) / (float)keyNumber);
		float keyWidth = keyMove - spacing;
		for_every(row, rows)
		{
			int rowIdx = for_everys_index(row);
			float atX = keyboardSpace.x.min + rowOffset * (float)(rowIdx);
			float atY = keyboardSpace.y.max - rowMove * (float)(rowIdx);
			if (rowKeyOffset.is_index_valid(rowIdx))
			{
				atX += (float)rowKeyOffset[rowIdx] * keyMove;
			}
			for_every(key, row->keys)
			{
				float thisKeyWidth = keyWidth;
				if (for_everys_index(key) == row->keys.get_size() - 1)
				{
					if (!rowExtendLastKey.is_index_valid(rowIdx) ||
						rowExtendLastKey[rowIdx])
					{
						thisKeyWidth = keyboardSpace.x.max - atX;
					}
				}
				String keyString;
				if (key->isBackspace)
				{
					keyString = LOC_STR(NAME(lsKeyBackspace));
				}
				else if (key->isClear)
				{
					keyString = LOC_STR(NAME(lsKeyClear));
				}
				else if (key->isEnter)
				{
					keyString = LOC_STR(NAME(lsKeyEnter));
				}
				else if (key->key == ' ')
				{
					keyString = LOC_STR(NAME(lsKeySpace));
				}
				else
				{
					keyString = String::printf(TXT("%c"), key->key);
				}
				auto* b = new HubWidgets::Button(Name::invalid(),
					Range2(Range(atX, atX + thisKeyWidth - 1.0f), Range(atY - rowSize + 1.0f, atY)), keyString);
				Key keyCopy = *key;
				b->on_click = [this, keyCopy](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
				{
					if (keyCopy.isBackspace)
					{
						text.cut_right_inline(1);
					}
					else if (keyCopy.isClear)
					{
						text = String::empty();
					}
					else if (keyCopy.isEnter)
					{
						if (onOK)
						{
							onOK(text);
						}
						deactivate();
						return;
					}
					else
					{
						text += keyCopy.key;
					}
					update_text();
				};
				add_widget(b);

				atX += keyMove;
			}
		}
	}
}

void EnterText::update_text()
{
	if (auto * t = fast_cast<HubWidgets::Text>(editWidget.get()))
	{
		if (blinkNow)
		{
			t->set(text + TXT("_"));
		}
		else
		{
			t->set(text + TXT(" "));
		}
	}
}

void EnterText::advance(float _deltaTime, bool _beyondModal)
{
	timeToBlink -= _deltaTime;
	if (timeToBlink <= 0.0f)
	{
		timeToBlink = 1.0f;
		blinkNow = !blinkNow;
		update_text();
	}
	base::advance(_deltaTime, _beyondModal);
}
