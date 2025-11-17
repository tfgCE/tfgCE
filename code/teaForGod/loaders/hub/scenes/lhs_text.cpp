#include "lhs_text.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_rect.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\..\framework\game\bullshotSystem.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\video\font.h"
#include "..\..\..\..\framework\video\texturePart.h"

//

DEFINE_STATIC_NAME(text);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsSpaceRequiredOnLoad, TXT("space required on load"));

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(Text);

Text::Text(String const & _text, Optional<float> const & _delay, Name const & _screenIDToReuse)
: text(_text)
, screenId(_screenIDToReuse)
, delay(_delay.get(0.0f))
{
}

Text::Text(tchar const * _text, Optional<float> const & _delay, Name const & _screenIDToReuse)
: text(_text)
, screenId(_screenIDToReuse)
, delay(_delay.get(0.0f))
{
}

Text::Text(Name const & _locStrId, Optional<float> const & _delay, Name const & _screenIDToReuse)
: text(LOC_STR(_locStrId))
, screenId(_screenIDToReuse)
, delay(_delay.get(0.0f))
{
}

Text* Text::with_image(Framework::TexturePart* _tp, Optional<Vector2> const& _scaleSpaceRequired, Optional<Vector2> const& _scaleImage, Optional<bool> const& _background)
{
	image = _tp;
	if (_scaleSpaceRequired.is_set())
	{
		imageScaleSpaceRequired = _scaleSpaceRequired.get();
		imageScale = _scaleSpaceRequired.get();
	}
	if (_scaleImage.is_set())
	{
		imageScale = _scaleImage.get();
	}
	if (_background.is_set())
	{
		imageBackground = _background.get();
	}
	return this;
}

void Text::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	loaderDeactivated = false;
	timeToShow = delay;

	if (conditionToClose != nullptr)
	{
		get_hub()->allow_to_deactivate_with_loader_immediately(false);
	}
	else
#ifdef AN_ALLOW_BULLSHOTS
#ifdef AN_STANDARD_INPUT
	if (Framework::BullshotSystem::is_setting_active(NAME(bsSpaceRequiredOnLoad)))
	{
		get_hub()->allow_to_deactivate_with_loader_immediately(false);
	}
	else
#endif
#endif
	{
		get_hub()->allow_to_deactivate_with_loader_immediately(true);
	}
	 
	if (screenId.is_valid())
	{
		get_hub()->keep_only_screen(screenId);
	}
	else
	{
		get_hub()->deactivate_all_screens();
	}

	if (timeToShow <= 0.0f)
	{
		show();
	}
}

void Text::show()
{
	if (text.is_empty() && !image.is_set())
	{
		return;
	}
	HubScreen* screen = get_hub()->get_screen(screenId);

	if (!screen)
	{
		Vector2 usePPA = ppa.get(HubScreen::s_pixelsPerAngle);
		auto const& fs = HubScreen::s_fontSizeInPixels;
		int width;
		int lineCount;
		HubWidgets::Text::measure(get_hub()->get_font(), NP, NP, text, lineCount, width);
		Vector2 requiredSize = Vector2((float)(width) + (float)margin.x * 2.0f * max(fs.x, fs.y), ((float)(lineCount)+ (float)margin.y * 2.0f) * max(fs.x, fs.y));
		requiredSize.y += 4.0f;

		float imageBorder = fs.y;
		float imageTextSeparator = 0.0f;
		if (image.is_set())
		{
			if (imageBackground)
			{
				requiredSize.x = image->get_size().x * imageScaleSpaceRequired.x;
				requiredSize.y = image->get_size().y * imageScaleSpaceRequired.y;
			}
			else
			{
				requiredSize.x = max(requiredSize.x, image->get_size().x * imageScaleSpaceRequired.x + imageBorder * 2.0f);
				requiredSize.y += image->get_size().y * imageScaleSpaceRequired.y + imageBorder + imageTextSeparator;
			}
		}

		requiredSize /= usePPA;
		screen = new HubScreen(get_hub(), screenId.is_valid()? screenId : NAME(text), requiredSize, get_hub()->get_current_forward(), get_hub()->get_radius() * 0.7f, true, verticalOffset, usePPA);
		screen->activate();
		screen->followHead = followHead;
		screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
		get_hub()->add_screen(screen);

		if (extraBackground.is_set())
		{
			auto* w = new HubWidgets::Rect(screen->wholeResolutionInPixels, extraBackground.get());
			w->filled();
			screen->add_widget(w);
		}
		if (image.is_set())
		{
			Range2 imageRange = screen->mainResolutionInPixels;
			if (!imageBackground)
			{
				imageRange.x.min += imageBorder;
				imageRange.x.max -= imageBorder;
				imageRange.y.max -= imageBorder;
				imageRange.y.min = imageRange.y.max - image->get_size().y * imageScaleSpaceRequired.y;
			}
			auto* w = new HubWidgets::Image(Name::invalid(), imageRange, image.get(), imageScale);
			screen->add_widget(w);
		}
		{
			Range2 textRange = screen->mainResolutionInPixels;
			if (image.is_set())
			{
				if (imageBackground)
				{
					textRange.x.min += (float)margin.x * fs.x;
					textRange.x.max -= (float)margin.x * fs.x;
					textRange.y.min += (float)margin.y * fs.y;
					textRange.y.max -= (float)margin.y * fs.y;
				}
				else
				{
					textRange.y.min += (float)margin.y * fs.y;
					textRange.y.max -= (float)margin.y * fs.y + image->get_size().y * imageScaleSpaceRequired.y + imageBorder + imageTextSeparator;
				}
			}
			else if (textAlign != Vector2::half)
			{
				textRange.x.min += (float)margin.x * fs.x;
				textRange.x.max -= (float)margin.x * fs.x;
				textRange.y.min += (float)margin.y * fs.y;
				textRange.y.max -= (float)margin.y * fs.y;
			}
			auto* w = new HubWidgets::Text(Name::invalid(), textRange, text);
			w->alignPt = textAlign;
			if (textColour.is_set())
			{
				w->set_colour(textColour.get());
			}
			screen->add_widget(w);
		}
	}
}

void Text::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
	if (timeToShow > 0.0f && !does_want_to_end())
	{
		timeToShow -= _deltaTime;
		if (timeToShow <= 0.0f)
		{
			show();
		}
	}
}

void Text::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
	loaderDeactivated = true;
}

bool Text::does_want_to_end()
{
	if (conditionToClose)
	{
		return conditionToClose();
	}
#ifdef AN_ALLOW_BULLSHOTS
#ifdef AN_STANDARD_INPUT
	if (Framework::BullshotSystem::is_setting_active(NAME(bsSpaceRequiredOnLoad)))
	{
		if (auto* i = ::System::Input::get())
		{
			if (!i->is_key_pressed(::System::Key::Space))
			{
				return false;
			}
		}
	}
#endif
#endif
	return loaderDeactivated;
}
