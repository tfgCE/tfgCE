#include "lhc_debugNote.h"

#include "..\loaderHub.h"
#include "..\loaderHubWidget.h"
#include "lhc_message.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\reportBug.h"

#include "..\..\..\..\framework\jobSystem\jobSystem.h"
#include "..\..\..\..\framework\object\actor.h"

#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\core\debug\debug.h"
#include "..\..\..\..\core\system\core.h"
#include "..\..\..\..\core\system\faultHandler.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsKeyBackspace, TXT("hub; common; key; backspace"));
DEFINE_STATIC_NAME_STR(lsKeyClear, TXT("hub; common; key; clear"));
DEFINE_STATIC_NAME_STR(lsKeyEnter, TXT("hub; common; key; enter"));
DEFINE_STATIC_NAME_STR(lsKeySpace, TXT("hub; common; key; space"));

#define EDIT_LINES 10

//

using namespace Loader;
using namespace HubScreens;

//

REGISTER_FOR_FAST_CAST(DebugNote);

void DebugNote::show(Hub* _hub, String const& _info, String const& _text, bool _quickSend, bool _sendLogAlways)
{
	int width = 50;
	int height = 13 + EDIT_LINES;
	
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
	DebugNote* newEE = new DebugNote(_hub, _info, infoLineCount, _text, size, _hub->get_last_view_rotation() * Rotator3(0.0f, 1.0f, 0.0f), _hub->get_radius() * 0.5f, _sendLogAlways);

	if (_quickSend)
	{
		newEE->send_note(true);
	}
	else
	{
		newEE->activate();
		_hub->add_screen(newEE);
	}
}

DebugNote::DebugNote(Hub* _hub, String const& _info, int _infoLineCount, String const& _text, Vector2 const& _size, Rotator3 const& _at, float _radius, bool _sendLogAlways)
: HubScreen(_hub, Name::invalid(), _size, _at, _radius)
, info(_info)
, text(_text)
{
	be_modal();

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

	Range2 availableSpace = mainResolutionInPixels;
	availableSpace.expand_by(-Vector2::one * fsxy);

	if (! info.is_empty())
	{
		Range2 at = availableSpace;
		at.y.min = at.y.max - fs.y * (float)_infoLineCount;
		auto* t = new HubWidgets::Text(Name::invalid(), at, info);

		t->alignPt = Vector2(0.5f, 1.0f);
		add_widget(t);

		availableSpace.y.max = at.y.min - fsxy;
	}

	{
		Range2 at = availableSpace;
		at.y.min = at.y.max - fs.y * (float)(EDIT_LINES + 1);
		auto* t = new HubWidgets::Text(Name::invalid(), at, text);
		t->with_align_pt(Vector2(0.0f, 0.0f));

		editWidget = t;
		add_widget(t);

		availableSpace.y.max = at.y.min - fsxy;
	}

	{
		Range2 at = availableSpace;
		at.y.max = at.y.min + fs.y * 2.0f;

		String sendNote(TXT("send note"));
		String sendNoteWithLog(TXT("send note+log"));
		String justLog(TXT("just log"));
		String cancel(TXT("cancel"));

		Array<HubScreenButtonInfo> buttons;
		if (! _sendLogAlways) buttons.push_back(HubScreenButtonInfo(Name::invalid(), sendNote, [this]() { send_note(); deactivate(); }));
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), sendNoteWithLog, [this]() { send_note(true); deactivate(); }));
		if (!_sendLogAlways) buttons.push_back(HubScreenButtonInfo(Name::invalid(), justLog, [this]() { write_note();  deactivate(); }));
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), cancel, [this]() { deactivate(); }));

		float spacing = 2.0f;
		add_button_widgets(buttons,
			at.bottom_left(), at.top_right(),
			fs.x * spacing);

		availableSpace.y.min = at.y.max + fsxy;
	}

	update_text();

	Range2 keyboardSpace = availableSpace;

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
				r.keys.push_back(Key(','));
				r.keys.push_back(Key('.'));
				r.keys.push_back(Key(' '));
			}
		}
		float rowMove = round(keyboardSpace.y.length() / 4.0f);
		float spacing = round(rowMove * 0.15f);
		float rowSize = rowMove - spacing;
		float rowOffset = fs.x * 0.7f;
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
				bool actualSpace = false;
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
					actualSpace = true;
				}
				else
				{
					keyString = String::printf(TXT("%c"), key->key);
				}
				auto* b = new HubWidgets::Button(Name::invalid(),
					Range2(Range(atX, atX + thisKeyWidth - 1.0f), Range(atY - rowSize + 1.0f, atY)), keyString);
				Key keyCopy = *key;
				if (actualSpace)
				{
					b->activateOnTriggerHold = true;
				}
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
						text += TXT("~");
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

void DebugNote::update_text()
{
	if (auto * t = fast_cast<HubWidgets::Text>(editWidget.get()))
	{
		t->set(text + TXT("_"));
	}
}

void DebugNote::advance(float _deltaTime, bool _beyondModal)
{
	base::advance(_deltaTime, _beyondModal);
}

void DebugNote::write_note()
{
	lock_output();
	output(TXT("[DEV NOTE]"));
	output(TXT("%S"), text.to_char());
	output(TXT("[DEV NOTE CALL STACK INFO]"));
	output(TXT("%S"), get_call_stack_info().to_char());
	output(TXT("[DEV NOTE DEBUG INFO]"));
	output(TXT("%S"), get_general_debug_info().to_char());
	if (auto* piow = TeaForGodEmperor::PilgrimageInstanceOpenWorld::get())
	{
		if (auto* playerActor = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>()->access_player().get_actor())
		{
			output(TXT("[DEV NOTE] written at location:"));
			List<String> list = piow->build_dev_info(playerActor);
			for_every(line, list)
			{
				output(TXT("%S"), line->to_char());
			}
		}
		else
		{
			output(TXT("[DEV NOTE] location where written is unknown:"));
		}
	}
	unlock_output();
}

void DebugNote::send_note(bool _withLog)
{
	write_note();
	String summary = String::printf(TXT("%S (%S), [dev-note] %S"), get_build_version(), ::System::Core::get_system_precise_name().to_char(), text.get_left(60).to_char());
	String logFile;
	if (_withLog)
	{
		String fileName = copy_log(TXT("dev_note"), true);
		if (!fileName.is_empty())
		{
			logFile = fileName; // relative to main
		}
	}
	String useLogFileName = IO::safe_filename(String::printf(TXT("devNote_%S"), text.get_left(20).to_char()));
	String info = text;
	info += TXT("\n");
	info += TXT("\n");
	if (auto* piow = TeaForGodEmperor::PilgrimageInstanceOpenWorld::get())
	{
		output(TXT("[DEV NOTE] sent from location:"));
		List<String> list = piow->build_dev_info(TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>()->access_player().get_actor());
		for_every(line, list)
		{
			info += *line + TXT("\n");
		}
	}
	else
	{
		info += ::System::FaultHandler::get_custom_info();
	}

	struct SendReportData
	{
		String summary;
		String info;
		String logFile;
		String useLogFileName;
		RefCountObjectPtr<HubScreen> waitScreen;
		RefCountObjectPtr<HubWidget> waitScreenText;

		SendReportData() {}
		SendReportData(String const& _summary, String const& _info, String const& _logFile, String const& _useLogFileName, HubScreen* _waitScreen, HubWidget* _waitScreenText) : summary(_summary), info(_info), logFile(_logFile), useLogFileName(_useLogFileName), waitScreen(_waitScreen), waitScreenText(_waitScreenText) {}

		static void send(void* _data)
		{
			SendReportData* srData = (SendReportData*)_data;
			String summary = srData->summary;
			String info = srData->info;
			String logFile = srData->logFile;
			String useLogFileName = srData->useLogFileName;

			RefCountObjectPtr<HubWidget> waitScreenText = srData->waitScreenText;

			TeaForGodEmperor::ReportBug::send(summary, info, logFile, TXT("dev-note"), TXT("DevNotes"), useLogFileName.to_char(),
				[waitScreenText](String const& _text)
				{
					if (auto* t = fast_cast<HubWidgets::Text>(waitScreenText.get()))
					{
						t->set(_text);
					}
				});

			srData->waitScreen->deactivate();

			delete srData;
		}
	};

	HubScreen* waitScreen;
	HubWidget* waitScreenText;
	{
		int width = 30;
		int lineCount = 4;
		Vector2 fs = HubScreen::s_fontSizeInPixels;
		Vector2 ppa = HubScreen::s_pixelsPerAngle;
		float margin = max(fs.x, fs.y) * 1.0f;
		Vector2 size = Vector2(((float)(width + 2) * fs.x + margin * 2.0f) / ppa.x,
			(HubScreen::s_fontSizeInPixels.y * (float)(lineCount)+margin * 2.0f) / ppa.y);
		waitScreen = new HubScreen(hub, Name::invalid(), size, at, radius, NP, NP, ppa);
		waitScreen->be_modal();

		{
			HubWidgets::Text* t = new HubWidgets::Text();
			t->at = waitScreen->mainResolutionInPixels.expanded_by(-Vector2::one * margin);
			t->with_align_pt(Vector2::half);
			waitScreen->add_widget(t);
			waitScreenText = t;
		}
	}
	
	waitScreen->activate();
	hub->add_screen(waitScreen);

	TeaForGodEmperor::Game::get()->get_job_system()->do_asynchronous_off_system_job(SendReportData::send, new SendReportData(summary, info, logFile, useLogFileName, waitScreen, waitScreenText));
}
