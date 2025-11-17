#pragma once

#include "..\displayTypes.h"

#include "..\displayDrawCommand.h"

#include "..\..\..\core\types\colour.h"

namespace Framework
{
	struct DisplayRegion;

	namespace DisplayDrawCommands
	{
		class TextAtBase
		: public DisplayDrawCommand
		{
			typedef DisplayDrawCommand base;
		public:
			TextAtBase();

			TextAtBase* alphabet(Name const & _alphabet) { alphabetParam = _alphabet; return this; }
			TextAtBase* at(CharCoords const & _at) { atParam = _at; return this; }
			TextAtBase* at(CharCoord _x, CharCoord _y) { return at(CharCoords(_x, _y)); }
			TextAtBase* scale(VectorInt2 const & _at) { scaleParam = _at; return this; }
			TextAtBase* scale(int _x, int _y) { return scale(VectorInt2(_x, _y)); }
			TextAtBase* pixel_offset(VectorInt2 const & _pixelOffset) { pixelOffsetParam = _pixelOffset; return this; }
			TextAtBase* pixel_offset(int _x, int _y) { return pixel_offset(VectorInt2(_x, _y)); }
			TextAtBase* in_region(Name const & _region) { inRegionName = _region; return this; }
			TextAtBase* in_region(DisplayRegion const * _region) { inRegion = _region; return this; }
			TextAtBase* in_region(RangeCharCoord2 const & _rect) { inRect = _rect; return this; }
			TextAtBase* in_whole_display(Display* _display);
			TextAtBase* in_region_from_top_left(Name const & _region, CharCoords const & _fromTopLeft) { inRegionName = _region; fromTopLeftParam = _fromTopLeft; return this; }
			TextAtBase* in_region_from_top_left(Name const & _region, CharCoord _x, CharCoord _y) { inRegionName = _region; fromTopLeftParam = CharCoords(_x, _y); return this; }
			TextAtBase* in_region_from_top_left(DisplayRegion const * _region, CharCoords const & _fromTopLeft) { inRegion = _region; fromTopLeftParam = _fromTopLeft; return this; }
			TextAtBase* in_region_from_top_left(DisplayRegion const * _region, CharCoord _x, CharCoord _y) { inRegion = _region; fromTopLeftParam = CharCoords(_x, _y); return this; }
			TextAtBase* text(String const & _text) { return text(_text.get_data().get_data()); }
			TextAtBase* text(tchar const * _text);
			TextAtBase* text_ext(String const & _text) { return text_ext(_text.get_data().get_data()); }
			TextAtBase* text_ext(tchar const * _text);
			TextAtBase* use_font(Font* _font) { useFont = _font; return this; }
			TextAtBase* use_default_colour_pair() { useDefaultColourPairParam = true; return this; }
			TextAtBase* use_colour_pair(Name const & _colourPairName) { useColourPair = _colourPairName; return this; }
			TextAtBase* ink(Colour const & _colour) { inkParam = _colour; return this; }
			TextAtBase* paper(Colour const & _colour) { paperParam = _colour; return this; }
			TextAtBase* ink(Optional<Colour> const & _colour) { inkParam = _colour; return this; }
			TextAtBase* paper(Optional<Colour> const & _colour) { paperParam = _colour; return this; }
			TextAtBase* limit(int _limit) { limitParam = _limit; return this; }
			TextAtBase* length(int _length) { return limit(_length); }
			TextAtBase* h_align(DisplayHAlignment::Type _alignment) { hAlignmentParam = _alignment; return this; }
			TextAtBase* h_align_right() { return h_align(DisplayHAlignment::Right); }
			TextAtBase* h_align_centre() { return h_align(DisplayHAlignment::Centre); }
			TextAtBase* h_align_centre_right() { return h_align(DisplayHAlignment::CentreRight); }
			TextAtBase* h_align_centre_fine() { return h_align(DisplayHAlignment::CentreFine); }
			TextAtBase* h_align_left() { return h_align(DisplayHAlignment::Left); }
			TextAtBase* v_align(DisplayVAlignment::Type _alignment) { vAlignmentParam = _alignment; return this; }
			TextAtBase* v_align_top() { return v_align(DisplayVAlignment::Top); }
			TextAtBase* v_align_centre() { return v_align(DisplayVAlignment::Centre); }
			TextAtBase* v_align_centre_bottom() { return v_align(DisplayVAlignment::CentreBottom); }
			TextAtBase* v_align_centre_fine() { return v_align(DisplayVAlignment::CentreFine); }
			TextAtBase* v_align_bottom() { return v_align(DisplayVAlignment::Bottom); }
			TextAtBase* compact() { compactParam = true; return this; }
			TextAtBase* without_background() { backgroundParam = false; return this; }
			
			TextAtBase* rotated(int _rotated) { rotatedParam = _rotated; return this; }

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;
			implement_ void prepare(Display* _display);

			implement_ Colour const & get_use_colourise_ink(Display * _display) const;
			implement_ Colour const & get_use_colourise_paper(Display * _display) const;

		protected:
			bool draw_text_onto(tchar const * _text, int _textLength, CharCoords const & _at, VectorInt2 const & _fineOffset, VectorInt2 const & _scale, Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ int & _drawCyclesRequiredForWholeText, REF_ DisplayDrawContext & _context) const;
			VectorInt2 const & get_scale() const;

			CharCoord calculate_field_length(int _textLength = 0, int _scaleX = 1) const;

		protected:
			Name alphabetParam;
			CharCoords atFinal = CharCoords::zero;
			Font* useFont = nullptr;
			Optional<CharCoords> atParam;
			Optional<VectorInt2> scaleParam;
			Optional<VectorInt2> pixelOffsetParam;
			Optional<bool> useDefaultColourPairParam;
			Name useColourPair;
			Optional<Colour> inkParam;
			Optional<Colour> paperParam;
			Optional<int> limitParam;
			Optional<DisplayHAlignment::Type> hAlignmentParam;
			Optional<DisplayVAlignment::Type> vAlignmentParam;
			Optional<Name> inRegionName;
			DisplayRegion const * inRegion;
			Optional<RangeCharCoord2> inRect;
			Optional<CharCoords> fromTopLeftParam;
			Optional<bool> compactParam;
			Optional<bool> backgroundParam;

			static const int MAX_LENGTH = 512;
			tchar textStoredParam[MAX_LENGTH + 1];
			tchar const * textExtParam = nullptr; // for longer texts held outside - use with extreme care!
			tchar const * textParam = nullptr;
			int textParamLength;

			int rotatedParam = 0;

			Vector2 calculate_char_size(Display* _display) const;
		};
	};

};
