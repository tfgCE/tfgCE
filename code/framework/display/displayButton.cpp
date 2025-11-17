#include "displayButton.h"

#include "display.h"
#include "displayDrawCommands.h"

#include "..\video\texture.h"

#include "..\..\core\concurrency\scopedSpinLock.h"

using namespace Framework;

//

DEFINE_STATIC_NAME(button);
DEFINE_STATIC_NAME(cursor);

//

REGISTER_FOR_FAST_CAST(DisplayButton);

RefCountObjectPtr<DisplayColourReplacer> DisplayButton::defaultColourReplacerSelected;

void DisplayButton::destroy_ref_count_object()
{
	release();
}

void DisplayButton::on_release()
{
	*this = DisplayButton();
}

DisplayButton::DisplayButton()
{
	colourPairNormal = NAME(button);
	colourPairSelected = NAME(cursor);
}

void DisplayButton::initialise_static()
{
}

void DisplayButton::close_static()
{
	defaultColourReplacerSelected = nullptr;
}

DisplayButton* DisplayButton::use_colour_replacer_for_normal(DisplayColourReplacer * _colourReplacer)
{
	colourReplacerNormal = _colourReplacer;
	return this;
}

DisplayButton* DisplayButton::use_colour_replacer_for_selected(DisplayColourReplacer * _colourReplacer)
{
	colourReplacerSelected = _colourReplacer;
	return this;
}

void DisplayButton::add_custom_draw_command(DisplayDrawCommand* _ddc)
{
	customDrawCommands.push_back(DisplayDrawCommandPtr(_ddc));
}

void DisplayButton::add_custom_draw_command_for_selected(DisplayDrawCommand* _ddc)
{
	customDrawCommandsSelected.push_back(DisplayDrawCommandPtr(_ddc));
}

void DisplayButton::redraw(Display* _display, bool _clear)
{
	if (insideDisplay == _display && !is_locked())
	{
		if (isVisible && !_clear)
		{
			add_draw_commands(_display, isSelected);
		}
		else
		{
			add_draw_commands_clear(_display);
		}
	}
}

CharCoord DisplayButton::calc_caption_height(Display* _display, int _buttonWidth) const
{
	CharCoords placement = actualAt;
	CharCoords useSize = CharCoords(_buttonWidth, _display->get_char_resolution().y);

	DisplayDrawCommands::TextAtMultiline textCaption;
	textCaption.at(placement + CharCoords(0, 0))->length(useSize.x)
		->text(captionParam.to_char())
		->h_align(hAlignmentParam.is_set() ? hAlignmentParam.get() : (fineAlignment? DisplayHAlignment::CentreFine : DisplayHAlignment::Centre));
	textCaption.start_at_line(0)
		->end_at_line(useSize.y - 1);
	textCaption.prepare(_display);

	return textCaption.get_lines_count();
}

CharCoord DisplayButton::calc_caption_width(Display* _display, int _maxButtonWidth, OUT_ int * _linesCount) const
{
	CharCoords placement = actualAt;
	CharCoords useSize = CharCoords(_maxButtonWidth > 0 ? _maxButtonWidth : _display->get_char_resolution().x, _display->get_char_resolution().y);

	DisplayDrawCommands::TextAtMultiline textCaption;
	textCaption.at(placement + CharCoords(0, 0))->length(useSize.x)
		->text(captionParam.to_char())
		->h_align(hAlignmentParam.is_set() ? hAlignmentParam.get() : (fineAlignment ? DisplayHAlignment::CentreFine : DisplayHAlignment::Centre));
	textCaption.start_at_line(0)
		->end_at_line(useSize.y - 1);
	textCaption.prepare(_display);

	assign_optional_out_param(_linesCount, textCaption.get_lines_count());

	return textCaption.get_max_line_length();
}

void DisplayButton::add_draw_commands(Display* _display, bool _selected) const
{
	Name useColourPair = _selected ? colourPairSelected : colourPairNormal;
	DisplayColourReplacer* colourReplacer = colourReplacerNormal.get();
	
	// if no colour pair found, we will use current ink and paper
	Colour ink = _display->get_current_ink();
	Colour paper = _display->get_current_paper();
	if (!_display->get_colour_pair(useColourPair))
	{
		useColourPair = Name::invalid();
		if (_selected)
		{
			swap(ink, paper);
		}
	}

	if (_selected)
	{
		colourReplacer = colourReplacerSelected.get();
		if (!colourReplacer && customDrawCommandsSelected.is_empty() && !customDrawCommands.is_empty()) // use default colour replacer only if no custom drawing nor colour replacer
		{
			if (!defaultColourReplacerSelected.is_set())
			{
				static Concurrency::SpinLock createDefaultColourReplacerSelectedLock = Concurrency::SpinLock(TXT("Framework.DisplayButton.add_draw_commands.createDefaultColourReplacerSelectedLock"));
				Concurrency::ScopedSpinLock lock(createDefaultColourReplacerSelectedLock);
				if (!defaultColourReplacerSelected.is_set())
				{
					defaultColourReplacerSelected = new DisplayColourReplacer();
					defaultColourReplacerSelected->set_default_colour_pair(colourPairSelected);
				}
			}
			colourReplacer = defaultColourReplacerSelected.get();
		}
	}

	CharCoords placement = actualAt;
	CharCoords useSize = actualSize;

	Array<DisplayDrawCommandPtr> const * useCustomDrawCommands = nullptr;
	if (!customDrawCommands.is_empty())
	{
		useCustomDrawCommands = &customDrawCommands;
	}
	if (!customDrawCommandsSelected.is_empty() && _selected)
	{
		useCustomDrawCommands = &customDrawCommandsSelected;
	}
	if (useCustomDrawCommands)
	{
		if (provideBackgroundForCustomDrawCommands)
		{
			CharCoords placement = actualAt;
			CharCoords useSize = actualSize;

			for (int y = 0; y < useSize.y; ++y)
			{
				DisplayDrawCommands::TextAtBase* text = (new DisplayDrawCommands::TextAt())->at(placement + CharCoords(0, y))->length(useSize.x);
				if (useColourPair.is_valid())
				{
					text->use_colour_pair(useColourPair);
				}
				else
				{
					text->ink(ink);
					text->paper(paper);
				}
				text->immediate_draw(drawImmediately);
				_display->add(text);
			}
		}
		for_every(pDrawCommand, *useCustomDrawCommands)
		{
			if (DisplayDrawCommand* drawCommand = pDrawCommand->get())
			{
				drawCommand->use_char_offset(_display, placement);
				drawCommand->use_colour_replacer(colourReplacer);
				_display->add(drawCommand->immediate_draw(drawImmediately));
			}
		}
	}
	else
	{
		if (actualSize.x == 0 || actualSize.y == 0)
		{
			return;
		}

		// clear all lines
		for (int y = 0; y < useSize.y; ++y)
		{
			DisplayDrawCommands::TextAtBase* text = (new DisplayDrawCommands::TextAt())->at(placement + CharCoords(0, y))->length(useSize.x);
			if (useColourPair.is_valid())
			{
				text->use_colour_pair(useColourPair);
			}
			else
			{
				text->ink(ink);
				text->paper(paper);
			}
			text->use_colour_replacer(colourReplacer);
			_display->add(text->immediate_draw(drawImmediately));
		}

		CharCoord usePadding = horizontalPaddingParam.is_set() ? horizontalPaddingParam.get() : 0;
		DisplayDrawCommands::TextAtMultiline* textCaption = new DisplayDrawCommands::TextAtMultiline();
		textCaption
			->length(useSize.x - usePadding * 2)
			->h_align(hAlignmentParam.is_set() ? hAlignmentParam.get() : (fineAlignment ? DisplayHAlignment::CentreFine : DisplayHAlignment::Centre));
		fineAlignment? textCaption->v_align_centre_fine() : textCaption->v_align_centre();
		if (useColourPair.is_valid())
		{
			textCaption->use_colour_pair(useColourPair);
		}
		else
		{
			textCaption->ink(ink);
			textCaption->paper(paper);
		}
		textCaption->text(captionParam.to_char());
		textCaption->prepare(_display);
		textCaption->use_colour_replacer(colourReplacer);

		// place it properly
		textCaption->in_region(RangeCharCoord2((VectorInt2)placement, placement + useSize - VectorInt2::one));
		
		// add actual drawing
		_display->add(textCaption->immediate_draw(drawImmediately));
	}
}

CharCoords DisplayButton::calculate_at(Display* _display) const
{
	CharCoords retVal = CharCoords::zero;

	if (inRegion)
	{
		if (atParam.is_set())
		{
			retVal = atParam.get() + inRegion->get_left_bottom();
		}
		else
		{
			retVal = inRegion->get_left_bottom();
		}
		if (fromTopLeftParam.is_set())
		{
			retVal = CharCoords(inRegion->get_left_bottom().x, inRegion->get_top_right().y) + fromTopLeftParam.get();
		}

		if (hAlignmentInRegionParam.is_set())
		{
			CharCoords useSize = calculate_size(_display);
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::Centre)
			{
				retVal.x = inRegion->get_left_bottom().x + (inRegion->get_length() - useSize.x) / 2;
			}
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::CentreRight)
			{
				retVal.x = inRegion->get_left_bottom().x + (inRegion->get_length() - useSize.x) / 2;
				if ((inRegion->get_length() - useSize.x) % 2 == 1)
				{
					retVal.x += 1;
				}
			}
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::Left)
			{
				retVal.x = inRegion->get_left_bottom().x;
			}
			if (hAlignmentInRegionParam.get() == DisplayHAlignment::Right)
			{
				retVal.x = inRegion->get_top_right().x - useSize.x;
			}
		}
	}
	else
	{
		if (atParam.is_set())
		{
			retVal = atParam.get();
		}
	}

	return retVal;
}

CharCoords DisplayButton::calculate_size(Display* _display) const
{
	CharCoords retVal = CharCoords(8, 1);

	if (sizeParam.is_set())
	{
		retVal = sizeParam.get();
	}

	if (inRegion)
	{
		if (! sizeParam.is_set())
		{
			retVal = inRegion->get_top_right() - inRegion->get_left_bottom() + CharCoords::one;
		}
	}

	return retVal;
}
