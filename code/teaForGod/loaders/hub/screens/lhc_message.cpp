#include "lhc_message.h"

#include "..\loaderHub.h"
#include "..\loaderHubWidget.h"

#include "..\widgets\lhw_text.h"

#include "..\..\..\..\framework\video\font.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;
using namespace HubScreens;

//

// localised strings
DEFINE_STATIC_NAME_STR(lsOk, TXT("hub; common; ok"));

//

REGISTER_FOR_FAST_CAST(Message);

#define USE_MARGIN (1.0f * max(fs.x, fs.y))

Message* Message::show(Hub* _hub, String const & _message, std::function<void()> _ok, MessageSetup const& _messageSetup)
{
	int width;
	int lineCount;
	HubWidgets::Text::measure(_hub->get_font(), NP, NP, _message, lineCount, width);
	width = max((10 * (int)_hub->get_font()->get_font()->calculate_char_size().x), width);
	if (_messageSetup.secondsLimit.is_set())
	{
		lineCount += (1 + 1);
	}
	if (_ok)
	{
		lineCount += (3 + 1);
	}
	Vector2 ppa = HubScreen::s_pixelsPerAngle;
	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float margin = USE_MARGIN;
	Vector2 size = Vector2(((float)(width + 2) + margin * 2.0f) / ppa.x,
		(HubScreen::s_fontSizeInPixels.y * (float)(lineCount) + margin * 2.0f) / ppa.y);
	Message* newMessage = new Message(_hub, _message, _ok, size, _hub->get_last_view_rotation() * Rotator3(0.0f, 1.0f, 0.0f), _hub->get_radius() * 0.5f, _messageSetup);

	newMessage->activate();
	_hub->add_screen(newMessage);
	return newMessage;
}

Message* Message::show(Hub* _hub, Name const & _locStrId, std::function<void()> _ok, MessageSetup const& _messageSetup)
{
	return show(_hub, LOC_STR(_locStrId), _ok, _messageSetup);
}

Message::Message(Hub* _hub, String const & _message, std::function<void()> _ok, Vector2 const & _size, Rotator3 const & _at, float _radius, MessageSetup const& _messageSetup)
: HubScreen(_hub, Name::invalid(), _size, _at, _radius)
, message(_message)
, onOk(_ok)
, secondsLimit(_messageSetup.secondsLimit)
{
	output(TXT("[show hub message]: %S"), _message.to_char());

	be_modal();

	Vector2 fs = HubScreen::s_fontSizeInPixels;

	Range2 available = mainResolutionInPixels;
	available.expand_by(Vector2::one * (-USE_MARGIN));

	if (onOk)
	{
		Range2 at = available;
		at.y.max = at.y.min + fs.y * 2.5f;

		String ok = LOC_STR(NAME(lsOk));

		Array<HubScreenButtonInfo> buttons;
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOk), [this]() { if (onOk) onOk(); deactivate(); }));

		float spacing = 2.0f;
		float length = (float)ok.get_length() + 2.0f + spacing * 0.5f;
		add_button_widgets(buttons,
			Vector2(max(available.x.centre() - fs.x * length, available.x.min), at.y.min),
			Vector2(min(available.x.centre() + fs.x * length, available.x.max), at.y.max),
			fs.x * spacing);

		available.y.min = at.y.max + fs.y;
	}

	if (secondsLimit.is_set())
	{
		Range2 at = available;
		at.y.max = at.y.min + fs.y * 1.0f;

		timeToNextSecond = 1.0f;
		secondsLeft = secondsLimit.get();

		secondsWidget = new HubWidgets::Text(Name::invalid(), at, String::printf(TXT("(%i)"), secondsLeft));
		add_widget(secondsWidget);

		available.y.min = at.y.max + fs.y;
	}

	{
		auto* t = new HubWidgets::Text(Name::invalid(), available, message);
		t->alignPt = Vector2(_messageSetup.alignXPt, 1.0f);
		add_widget(t);
	}

	if (_messageSetup.follow.get(true))
	{
		followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
	}
	else
	{
		dont_follow();
	}
}

void Message::advance(float _deltaTime, bool _beyondModal)
{
	if (secondsLeft > 0 && !_beyondModal)
	{
		timeToNextSecond -= _deltaTime;
		if (timeToNextSecond <= 0.0f)
		{
			while (timeToNextSecond <= 0.0f)
			{
				timeToNextSecond += 1.0f;
				secondsLeft -= 1;
			}
			if (secondsLeft <= 0)
			{
				secondsWidget->set(String::empty());
				if (onOk) onOk();
				deactivate();
				return;
			}
			else
			{
				secondsWidget->set(String::printf(TXT("(%i)"), secondsLeft));
			}
		}
	}
	base::advance(_deltaTime, _beyondModal);
}
