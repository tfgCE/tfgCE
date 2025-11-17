#include "lhw_text.h"

#include "..\loaderHub.h" 
#include "..\loaderHubScreen.h" 
#include "..\screens\lhc_messageSetBrowser.h"

#include "..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\video\font.h"

#include "..\..\..\..\core\other\typeConversions.h"
#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

#ifdef AN_DEVELOPMENT
#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define AN_DEBUG_READY_AND_MEASURE_TEXT
#endif
#endif

//

using namespace Loader;

//

/*
 *	Special lines
 *		img#libraryname			shows an image
 *		msb#section/name		for MessageSetBrowser screens jumps to a given section
 *		colour#colourname		changes colour for following lines, has to be only thing in a line
 */
//

REGISTER_FOR_FAST_CAST(HubWidgets::Text);

float HubWidgets::Text::calculate_vertical_size()
{
	Range2 useAt = at;
	if (scroll.scrollType != InnerScroll::NoScroll)
	{
		useAt = scroll.viewWindow;
	}

	ready_text(useAt.x.length());

	float fontHeight = HubScreen::s_fontSizeInPixels.y;
	return (float)linesToDraw.get_size() * fontHeight * scale.y;
}

void HubWidgets::Text::render_to_display(Framework::Display* _display)
{
	calculate_auto_values();

	if (auto* font = _display->get_font())
	{
		Colour border = Colour::lerp(active, HubColours::border()/*.with_alpha(0.8f)*/, HubColours::text());
		Colour inside = Colour::lerp(active, HubColours::widget_background()/*.with_alpha(0.3f)*/, HubColours::highlight()/*.with_alpha(0.8f)*/);

		border = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(border, screen, this);
		inside = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(inside, screen, this);

		Range2 useAt = at;
		if (scroll.scrollType != InnerScroll::NoScroll)
		{
			useAt = scroll.viewWindow;
		}

		scroll.render_to_display(_display, at, border, inside);

		scroll.push_clip_plane_stack();

		ready_text(useAt.x.length());

		float fontHeight = HubScreen::s_fontSizeInPixels.y;
		Vector2 offset = Vector2::zero;
		Vector2 useAlignPt = alignPt;
		if (scroll.scrollType != InnerScroll::NoScroll && scroll.scrollRequired)
		{
			useAlignPt = Vector2(alignPt.x, 1.0f);
		}
		else
		{
			offset.y += (float)(linesToDraw.get_size() - 1) * fontHeight * (1.0f - alignPt.y) * scale.y;
		}
		if (scroll.scrollType != InnerScroll::NoScroll)
		{
			offset -= scroll.scroll;
		}
		linesOnScreenY.clear();
		linesOnScreenY.make_space_for(linesToDraw.get_size());
		Colour textColour = colour.get(HubColours::text());
		textColour = Colour::lerp(overrideColour.a, textColour, overrideColour.with_alpha(textColour.a));
		textColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(textColour, screen, this);

		if (specialHighlight)
		{
			border = HubColours::special_highlight().with_set_alpha(1.0f);
			inside = HubColours::special_highlight_background_for(inside);
			textColour = HubColours::special_highlight().with_set_alpha(1.0f);
		}

		lastUsedColours.border = border;
		lastUsedColours.inside = inside;
		lastUsedColours.textColour = textColour;

		for_every(line, linesToDraw)
		{
			Range yRange;
			Vector2 lineAlignPt = useAlignPt;
			tchar const * text = line->to_char();
			Colour highlightColour = HubColours::selected_highlighted();

			if (line->has_prefix(TXT("img#")))
			{
				String img = line->get_sub(4);
				if (auto* tp = Framework::Library::get_current()->get_texture_parts().find(Framework::LibraryName(img)))
				{
					lineAlignPt = Vector2(0.5f, 1.0f);
					Framework::TexturePartDrawingContext context;
					context.scale = scale;
					context.colour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(context.colour, screen, this);
					Vector2 imgAt = round(useAt.get_at(lineAlignPt) + offset);
					Vector2 tpSize = tp->get_size();
					yRange = Range(imgAt.y - tpSize.y * scale.y, imgAt.y);
					float upSize = ceil(tp->get_size().y / HubScreen::s_fontSizeInPixels.y) * fontHeight * scale.y;
					imgAt.x = Range(useAt.x.min  + tpSize.x * 0.5f * scale.x, useAt.x.max - tpSize.x * 0.5f * scale.x).get_at(lineAlignPt.x) - tpSize.x * 0.5f  * scale.x - tp->get_left_bottom_offset().x * scale.x;
					imgAt.y = imgAt.y - tpSize.y * scale.y - tp->get_left_bottom_offset().y * scale.y - round((upSize - tpSize.y * scale.y) / 2.0f);
					tp->draw_at(::System::Video3D::get(), imgAt, context);

					offset.y -= upSize;
					text = nullptr;
				}
			}
			else if (line->has_prefix(TXT("colour#")))
			{
				String colourName = line->get_sub(7);
				textColour = HubColours::text();
				if (!colourName.is_empty())
				{
					Colour newTextColour;
					if (newTextColour.parse_from_string(colourName))
					{
						textColour = newTextColour;
					}
				}
				textColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(textColour, screen, this);
				continue;
			}
			else if (line->has_prefix(TXT("msb#")))
			{
				text = &line->get_data()[4];
				textColour = HubColours::ink();
				textColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(textColour, screen, this);
				highlightColour = HubColours::selected_highlighted();
				lineAlignPt.x = 0.5f;
			}

			highlightColour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(highlightColour, screen, this);

			if (text)
			{
				Vector2 textAt = round(useAt.get_at(lineAlignPt) + offset);
				System::FontDrawingContext fdContext;
				fdContext.forceFixedSize = forceFixedSize;
				fdContext.smallerDecimals = smallerDecimals;
				font->draw_text_at(::System::Video3D::get(), text, highlighted ? highlightColour : textColour, textAt, scale, lineAlignPt, NP, fdContext);
				yRange = Range(textAt.y - fontHeight * scale.y * lineAlignPt.y, textAt.y + fontHeight * scale.y * (1.0f - lineAlignPt.y));
				offset.y -= fontHeight * scale.y;
			}
			linesOnScreenY.push_back(yRange);
		}

		scroll.pop_clip_plane_stack();
	}

	base::render_to_display(_display);
}

void HubWidgets::Text::measure(Framework::Font const* _font, Optional<Vector2> const& _scale, Optional<bool> const & _forceFixedSize, String const & _text, OUT_ int & _lineCount, OUT_ int & _maxWidth, Optional<int> const & _maxWidthAvailable, ::List<String> * _outputLines)
{
	Vector2 scale = _scale.get(Vector2::one);
#ifdef AN_DEBUG_READY_AND_MEASURE_TEXT
	output(TXT("measure text"));
	int debugLineIdx = 0;
#endif
	if (_maxWidthAvailable.is_set() && _maxWidthAvailable.get() <= _font->calculate_char_size().x * scale.x)
	{
		// special case
		_maxWidth = TypeConversions::Normal::f_i_closest(_font->calculate_char_size().x * scale.x);
		_lineCount = _text.get_length();
		if (_outputLines)
		{
			for_count(int, i, _text.get_length())
			{
				_outputLines->push_back(_text.get_sub(i, 1));
				an_assert(_outputLines->get_size() < 10000, TXT("it's sure a lot of text!"));
			}
		}
		return;
	}

	_maxWidth = 0;
	float width = 0;
	int m = 0;
	int l = 0;
	int lAt = 0;
	bool normalLine = true;
	int chIdxCount = _text.get_length();
	// get rid of last empty lines
	while (chIdxCount > 1 &&
		_text.get_data()[chIdxCount - 1] == '~')
	{
		--chIdxCount;
	}
	Optional<int> lastSpaceAt;
	int chIdx = 0;
	while (chIdx < chIdxCount)
	{
		tchar const ch = _text.get_data()[chIdx];
		Optional<int> startNewLineAt;
		if (m == 0)
		{
			// check if it is a normal line
			int at = chIdx;
			normalLine = true;
			if (at < _text.get_length() - 5)
			{
				if (_text.get_data()[at + 0] == 'i' &&
					_text.get_data()[at + 1] == 'm' &&
					_text.get_data()[at + 2] == 'g' &&
					_text.get_data()[at + 3] == '#')
				{
					normalLine = false;
				}
			}
			if (at < _text.get_length() - 8)
			{
				if (_text.get_data()[at + 0] == 'c' &&
					_text.get_data()[at + 1] == 'o' &&
					_text.get_data()[at + 2] == 'l' &&
					_text.get_data()[at + 3] == 'o' &&
					_text.get_data()[at + 4] == 'u' &&
					_text.get_data()[at + 5] == 'r' &&
					_text.get_data()[at + 6] == '#')
				{
					normalLine = false;
					// find space after that and jump to next char
					chIdx += 7;
					while (chIdx < _text.get_length() &&
						   _text.get_data()[chIdx] != '~')
					{
						++chIdx;
					}
					if (_text.get_data()[chIdx] == '~')
					{
						startNewLineAt = chIdx + 1;
					}
					else
					{
						startNewLineAt = _text.get_length();
					}
					m = startNewLineAt.get() - lAt - 2 /* we don't want ~ and first character of a new line */;
				}
			}
		}
		if (ch == '~')
		{
			int at = chIdx;
			startNewLineAt = at + 1;
			if (at > lAt && lAt < _text.get_length() - 5)
			{
				if (_text.get_data()[lAt + 0] == 'i' &&
					_text.get_data()[lAt + 1] == 'm' &&
					_text.get_data()[lAt + 2] == 'g' &&
					_text.get_data()[lAt + 3] == '#')
				{
					int imgAt = lAt + 4;
					String img = _text.get_sub(imgAt, at - imgAt);
					if (auto* tp = Framework::Library::get_current()->get_texture_parts().find(Framework::LibraryName(img)))
					{
						l += TypeConversions::Normal::f_i_closest(ceil(tp->get_size().y / HubScreen::s_fontSizeInPixels.y) + 0.01f);
					}
				}
			}
		}
		else
		{
			float newWidth = width + (float)_font->get_font()->get_width(ch, _forceFixedSize.get(false)) * scale.x;
			++m;
			bool isOk = true;
			if (normalLine)
			{
				if (ch == ' ')
				{
					lastSpaceAt = chIdx;
				}
				if (_maxWidthAvailable.is_set() && _maxWidthAvailable.get() != 0)
				{
					if (TypeConversions::Normal::f_i_closest(newWidth) > _maxWidthAvailable.get())
					{
						isOk = false;
						int at = chIdx;
						if (lastSpaceAt.is_set())
						{
							width = 0.0f;
							m = lastSpaceAt.get() - lAt;
							for_range(int, i, lAt, lastSpaceAt.get() - 1)
							{
								width += (float)_font->get_font()->get_width(_text.get_data()[i], _forceFixedSize.get(false)) * scale.x;
							}
							startNewLineAt = lastSpaceAt.get() + 1;
						}
						else
						{
							--m;
							startNewLineAt = at;
						}
					}
				}
			}
			if (isOk)
			{
				width = newWidth;
			}
		}
		if (startNewLineAt.is_set())
		{
			if (_outputLines)
			{
				_outputLines->push_back(_text.get_sub(lAt, m));
				an_assert(_outputLines->get_size() < 10000, TXT("it's sure a lot of text!"));
			}
#ifdef AN_DEBUG_READY_AND_MEASURE_TEXT
			String line = _text.get_sub(lAt, m);
			output(TXT(" :%03i|%03i: >%S<"), debugLineIdx, l, line.to_char());
			++ debugLineIdx;
#endif
			if (normalLine)
			{
				_maxWidth = max(_maxWidth, TypeConversions::Normal::f_i_closest(width));
				++l;
			}
			lAt = startNewLineAt.get();
			m = 0;
			width = 0;
			lastSpaceAt.clear();
			chIdx = startNewLineAt.get();
		}
		else
		{
			++chIdx;
		}
	}
	if (m > 0)
	{
		if (_outputLines)
		{
			_outputLines->push_back(_text.get_sub(lAt, m));
		}
#ifdef AN_DEBUG_READY_AND_MEASURE_TEXT
		String line = _text.get_sub(lAt, m);
		output(TXT(" :%03i|%03i: >%S<"), debugLineIdx, l, line.to_char());
		++debugLineIdx;
#endif
		if (normalLine)
		{
			_maxWidth = max(_maxWidth, TypeConversions::Normal::f_i_closest(width));
			++l; // last line
		}
	}
	_lineCount = l;
}

void HubWidgets::Text::advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal)
{
	base::advance(_hub, _screen, _deltaTime, _beyondModal);

	float activePrev = active;

	activeTarget = 0.0f;
	for_count(int, hIdx, 2)
	{
		auto & hand = _hub->access_hand((::Hand::Type)hIdx);
		if (hand.overWidget == this)
		{
			activeTarget = hand.prevOverWidget == this ? 1.0f : 0.0f;
		}
	}
	active = blend_to_using_time(active, activeTarget, 0.1f, _deltaTime);

	if (activePrev != active && abs(active - activeTarget) > 0.005f)
	{
		mark_requires_rendering();
	}

	calculate_auto_values();
}

void HubWidgets::Text::calculate_auto_values()
{
	if (scroll.scrollType == InnerScroll::NoScroll)
	{
		// we need to ready the text
		return;
	}

	scroll.calculate_auto_space_available(at);

	float xAvailable = at.x.length();
	if (scroll.scrollType == InnerScroll::VerticalScroll)
	{
		// always assume scroll visible, even with auto hide
		xAvailable -= scroll.scrollWidth;
	}

	int lineCount;
	int maxWidth;
	int maxWidthAvailable = TypeConversions::Normal::f_i_closest(xAvailable);
	measure(hub->get_font(), scale, forceFixedSize, get_text(), lineCount, maxWidth, maxWidthAvailable);
	Vector2 wholeTextSize = Vector2((float)maxWidth, (float)lineCount * HubScreen::s_fontSizeInPixels.y);

	scroll.calculate_other_auto_variables(at, wholeTextSize);
}

void HubWidgets::Text::internal_on_click(HubScreen* _screen, int _handIdx, Vector2 const & _at, HubInputFlags::Flags _flags)
{
	for_every(yRange, linesOnScreenY)
	{
		if (yRange->does_contain(_at.y))
		{
			String const * line = &linesToDraw[for_everys_index(yRange)];
			if (line->has_prefix(TXT("msb#")))
			{
				if (auto* msb = fast_cast<HubScreens::MessageSetBrowser>(_screen))
				{
					msb->show_message(&line->get_data()[4]);
				}
			}
		}
	}
}

bool HubWidgets::Text::internal_on_hold(int _handIdx, bool _beingHeld, Vector2 const & _at)
{
	bool result;
	if (scroll.handle_internal_on_hold(_handIdx, false, _beingHeld, _at, at, HubScreen::s_fontSizeInPixels, OUT_ result))
	{
		return result;
	}

	return base::internal_on_hold(_handIdx, _beingHeld, _at);
}

void HubWidgets::Text::internal_on_release(int _handIdx, Vector2 const & _at)
{
	scroll.handle_internal_on_release(_handIdx, false);
}

bool HubWidgets::Text::internal_on_hold_grip(int _handIdx, bool _beingHeld, Vector2 const & _at)
{
	bool result;
	if (scroll.handle_internal_on_hold(_handIdx, true, _beingHeld, _at, at, HubScreen::s_fontSizeInPixels, OUT_ result))
	{
		return result;
	}

	return base::internal_on_hold(_handIdx, _beingHeld, _at);
}

void HubWidgets::Text::internal_on_release_grip(int _handIdx, Vector2 const & _at)
{
	scroll.handle_internal_on_release(_handIdx, true);
}

void HubWidgets::Text::process_input(int _handIdx, Framework::GameInput const & _input, float _deltaTime)
{
	scroll.handle_process_input(_handIdx, _input, _deltaTime, HubScreen::s_fontSizeInPixels, screen->pixelsPerAngle);
}

void HubWidgets::Text::ready_text(float _xAvailable)
{
	textToDraw = get_text();
	
	if (_xAvailable == 0.0f)
	{
		// this will be better than a single line
		_xAvailable = textToDraw.get_length() * hub->get_font()->get_font()->calculate_char_size().x * scale.x;
	}

	if (textSplitted != textToDraw ||
		textReadiedForX != _xAvailable)
	{
		textSplitted = textToDraw;
		textReadiedForX = _xAvailable;
		linesToDraw.clear();
		int tempLineCount, tempMaxLength;
		measure(hub->get_font(), scale, forceFixedSize, textToDraw, tempLineCount, tempMaxLength, TypeConversions::Normal::f_i_closest(_xAvailable), &linesToDraw);
#ifdef AN_DEBUG_READY_AND_MEASURE_TEXT
		output(TXT("ready text"));
		int lineIdx = 0;
		for_every(line, linesToDraw)
		{
			output(TXT(" :%03i: >%S<"), lineIdx, line->to_char());
			++lineIdx;
		}
#endif
	}
}
