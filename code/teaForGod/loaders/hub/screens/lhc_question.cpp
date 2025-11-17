#include "lhc_question.h"

#include "..\loaderHub.h"
#include "..\loaderHubWidget.h"

#include "..\widgets\lhw_text.h"

#include "..\..\..\..\framework\video\font.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsYes, TXT("hub; common; yes"));
DEFINE_STATIC_NAME_STR(lsNo, TXT("hub; common; no"));

//

using namespace Loader;
using namespace HubScreens;

REGISTER_FOR_FAST_CAST(Question);

#define USE_MARGIN (1.0f * max(fs.x, fs.y))

void Question::ask(Hub* _hub, String const & _question, std::function<void()> _yes, std::function<void()> _no, Optional<int> const & _secondsLimit, int _flags)
{
	int width;
	int lineCount;
	HubWidgets::Text::measure(_hub->get_font(), NP, NP, _question, lineCount, width);
	width = max((10 * (int)_hub->get_font()->get_font()->calculate_char_size().x), width);
	if (_secondsLimit.is_set())
	{
		lineCount += 2;
	}
	lineCount += 4; // yes/no
	Vector2 ppa = HubScreen::s_pixelsPerAngle;
	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float margin = USE_MARGIN;
	Vector2 size = Vector2(((float)(width + 2) + margin * 2.0f) / ppa.x,
		(HubScreen::s_fontSizeInPixels.y * (float)(lineCount) + margin * 2.0f) / ppa.y);
	Question* newQuestion = new Question(_hub, _question, _yes, _no, size, _hub->get_last_view_rotation() * Rotator3(0.0f, 1.0f, 0.0f), _hub->get_radius() * 0.5f, _secondsLimit, _flags);

	newQuestion->activate();
	_hub->add_screen(newQuestion);
}

void Question::ask(Hub* _hub, Name const & _locStrId, std::function<void()> _yes, std::function<void()> _no, Optional<int> const & _secondsLimit, int _flags)
{
	ask(_hub, LOC_STR(_locStrId), _yes, _no, _secondsLimit, _flags);
}

Question::Question(Hub* _hub, String const & _question, std::function<void()> _yes, std::function<void()> _no, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<int> const & _secondsLimit, int _flags)
: HubScreen(_hub, Name::invalid(), _size, _at, _radius)
, question(_question)
, onYes(_yes)
, onNo(_no)
, secondsLimit(_secondsLimit)
, flags(_flags)
{
	be_modal();

	Vector2 fs = HubScreen::s_fontSizeInPixels;

	{
		auto* t = new HubWidgets::Text(Name::invalid(), Range2(Range(mainResolutionInPixels.x.centre() - _size.x * 0.5f * HubScreen::s_fontSizeInPixels.x,
																	 mainResolutionInPixels.x.centre() + _size.x * 0.5f * HubScreen::s_fontSizeInPixels.x),
															   Range(mainResolutionInPixels.y.min, mainResolutionInPixels.y.max - USE_MARGIN)), question);
		t->alignPt.y = 1.0f;
		add_widget(t);
	}

	String yes = LOC_STR(NAME(lsYes));
	String no = LOC_STR(NAME(lsNo));

	Array<HubScreenButtonInfo> buttons;
	buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsYes), [this]() { if (onYes) onYes(); deactivate(); }).activate_on_trigger_hold(flags & YesOnTrigger));
	buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsNo), [this]() { if (onNo) onNo(); deactivate(); }));
		
	float spacing = 2.0f;
	float length = (float)max(yes.get_length(), no.get_length()) + 4.0f + spacing * 0.5f;
	add_button_widgets(buttons,
		Vector2(max(mainResolutionInPixels.x.centre() - fs.x * length, mainResolutionInPixels.x.min + fs.x), mainResolutionInPixels.y.min + USE_MARGIN),
		Vector2(min(mainResolutionInPixels.x.centre() + fs.x * length, mainResolutionInPixels.x.max - fs.x), mainResolutionInPixels.y.min + USE_MARGIN + fs.y * 2.5f),
		fs.x * spacing);

	if (secondsLimit.is_set())
	{
		timeToNextSecond = 1.0f;
		secondsLeft = secondsLimit.get();

		secondsWidget = new HubWidgets::Text(Name::invalid(), Range2(Vector2(mainResolutionInPixels.x.centre(), mainResolutionInPixels.y.min + USE_MARGIN + fs.y * 4.5f)), String::printf(TXT("(%i)"), secondsLeft));
		add_widget(secondsWidget);
	}

	followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
}

void Question::advance(float _deltaTime, bool _beyondModal)
{
	if (secondsLeft > 0 && ! _beyondModal)
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
				if (onNo) onNo();
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
