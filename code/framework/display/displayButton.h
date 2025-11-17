#pragma once

#include "displayControl.h"

#include "..\..\core\memory\softPooledObject.h"

namespace Framework
{
	struct DisplayColourReplacer;
	class DisplayDrawCommand;

	class DisplayButton
	: public DisplayControl
	, public SoftPooledObject<DisplayButton>
	{
		FAST_CAST_DECLARE(DisplayButton);
		FAST_CAST_BASE(DisplayControl);
		FAST_CAST_END();

		typedef DisplayControl base;
	public:
		static void initialise_static();
		static void close_static();

	public:
		DisplayButton();

		DisplayButton* caption(String const & _caption, bool _fineAlignment = true) { captionParam = _caption; fineAlignment = _fineAlignment;  return this; }

		DisplayButton* at(CharCoords const & _at) { atParam = _at; return this; }
		DisplayButton* at(CharCoord _x, CharCoord _y) { return at(CharCoords(_x, _y)); }

		DisplayButton* size(CharCoords const & _size) { sizeParam = _size; return this; }
		DisplayButton* size(CharCoord _x, CharCoord _y) { return size(CharCoords(_x, _y)); }
		DisplayButton* size(CharCoord _x) { return size(CharCoords(_x, 1)); }
		DisplayButton* horizontal_padding(CharCoord _x) { horizontalPaddingParam = _x; return this; }

		DisplayButton* use_colour_replacer_for_normal(DisplayColourReplacer * _colourReplacer);
		DisplayButton* use_colour_replacer_for_selected(DisplayColourReplacer * _colourReplacer);
		DisplayButton* use_colour_pair_for_normal(Name const & _pair) { colourPairNormal = _pair; return this; }
		DisplayButton* use_colour_pair_for_selected(Name const & _pair) { colourPairSelected = _pair; return this; }

		Array<DisplayDrawCommandPtr> & access_custom_draw_commands() { return customDrawCommands; }
		Array<DisplayDrawCommandPtr> & access_custom_draw_commands_for_selected() { return customDrawCommandsSelected; }
		void add_custom_draw_command(DisplayDrawCommand* _ddc);
		void add_custom_draw_command_for_selected(DisplayDrawCommand* _ddc);
		void provide_background_for_custom_draw_commands(bool _provideBackgroundForCustomDrawCommands) { provideBackgroundForCustomDrawCommands = _provideBackgroundForCustomDrawCommands; }

	public: // DisplayControl
		override_ void redraw(Display* _display, bool _clear = false);

	protected: // RefCountObject
		override_ void destroy_ref_count_object();

	protected: // SoftPooledObject
		override_ void on_release();

	public:
		String const & get_caption() const { return captionParam; }
		CharCoord calc_caption_height(Display* _display, int _buttonWidth) const;
		CharCoord calc_caption_width(Display* _display, int _maxButtonWidth, OUT_ int * _linesCount = nullptr) const;

	protected:
		String captionParam;
		bool fineAlignment = false;
		static RefCountObjectPtr<DisplayColourReplacer> defaultColourReplacerSelected;

		RefCountObjectPtr<DisplayColourReplacer> colourReplacerNormal;
		RefCountObjectPtr<DisplayColourReplacer> colourReplacerSelected;
		Name colourPairNormal;
		Name colourPairSelected;

		bool provideBackgroundForCustomDrawCommands = true; // by default
		Array<DisplayDrawCommandPtr> customDrawCommands; // relative to "at"
		Array<DisplayDrawCommandPtr> customDrawCommandsSelected; // relative to "at"

		Optional<CharCoord> horizontalPaddingParam;

		Optional<Name> inRegionName;
		DisplayRegion const * inRegion = nullptr;
		Optional<CharCoords> fromTopLeftParam;

		void add_draw_commands(Display* _display, bool _selected) const;

	protected: // DisplayControl
		virtual CharCoords calculate_at(Display* _display) const;
		virtual CharCoords calculate_size(Display* _display) const;
	};

};

TYPE_AS_CHAR(Framework::DisplayButton);
