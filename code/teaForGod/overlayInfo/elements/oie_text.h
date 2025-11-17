#pragma once

#include "..\overlayInfoElement.h"

namespace Framework
{
	class Font;
};

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		struct TextColours // translates text to colours
		{
			struct Element
			{
				Name id;
				Colour colour = Colour::none;

				Element() {}
				Element(Name const& _id, Colour const& _colour) : id(_id), colour(_colour) {}
			};
			ArrayStatic<Element, 16> colours;
			void add(Name const& _id, Colour const& _colour) { an_assert(colours.has_place_left()); if (colours.has_place_left()) colours.push_back(Element(_id, _colour)); }
			void add_default(Colour const& _colour) { an_assert(colours.has_place_left()); if (colours.has_place_left()) colours.push_back(Element(Name::invalid(), _colour)); }

			Optional<Colour> process_line(REF_ String& _line) const;

			TextColours()
			{
				SET_EXTRA_DEBUG_INFO(colours, TXT("OverlayInfo::TextColours.colours"));
			}
			static void alter_colour(REF_ Colour& _colour, Optional<Colour> const& _withColour);
		};

		namespace Elements
		{
			struct Text
			: public Element
			{
				FAST_CAST_DECLARE(Text);
				FAST_CAST_BASE(Element);
				FAST_CAST_END();

				typedef Element base;
			public:
				struct Line
				{
					Optional<Colour> colour;
					String line;
				};

			public:
				Text() {}
				Text(String const& _text) { with_text(_text); }

				Text* with_vertical_scroll_down(Range const& _showLines, float _scrollSpeed, float _scrollInitialWait = 0.0f, float _scrollPostWait = 0.0f) { scrollDown.active = true; scrollDown.showLines = _showLines; scrollDown.speed = _scrollSpeed; scrollDown.initialWait = _scrollInitialWait; scrollDown.postWait = _scrollPostWait; return with_vertical_align(1.0f); }
				Text* with_scale(float _scale) { scale = _scale; return this; }
				Text* with_use_additional_scale(Optional<float> const& _scale) { useAdditionalScale = _scale; return this; }
				Text* with_letter_size(float _letterSize) { letterSize = _letterSize; return this; }
				Text* with_text(String const& _text) { text = _text; requiresTextUpdate = true; return this; }
				Text* with_text(tchar const* _text) { if (_text) { text = _text; } else { text = String::empty(); } requiresTextUpdate = true; return this; }
				Text* with_font(Framework::Font* _font) { font = _font; charSize.clear(); return this; }
				Text* with_max_line_width(int _maxLineWidth = 0) { maxLineWidth = _maxLineWidth; requiresTextUpdate = true; return this; }
				Text* with_line_spacing(float _lineSpacing = 0.25f) { lineSpacing = _lineSpacing; return this; }
				Text* with_char_offset(Vector2 const & _charOffset) { charOffset = _charOffset; return this; }
				Text* with_horizontal_align(float _horizontalAlign = 0.5f) { horizontalAlign = _horizontalAlign; return this; }
				Text* with_vertical_align(float _verticalAlign = 0.5f) { verticalAlign = _verticalAlign; return this; }
				Text* with_smaller_decimals(Optional<float> _smallerDecimals = 0.5f) { smallerDecimals = _smallerDecimals; return this; }
				Text* with_background(Optional<Colour> _colour = NP) { backgroundColour = _colour; return this; }
				Text* with_colours(TextColours const & _colours) { colours = _colours; return this; }
				Text* with_force_blend(bool _forceBlend) { forceBlend = _forceBlend; return this; }

				int get_lines_count() const { return requiresTextUpdate ? 0 : lines.get_size(); }

				void update_text();

				List<Line> const& get_lines() { return lines; }

			public:
				implement_ void advance(System const* _system, float _deltaTime);
				implement_ void render(System const* _system, int _renderLayer) const;

			protected:
				String text;
				TextColours colours;
				Framework::Font* font = nullptr;
				mutable Optional<Vector2> charSize;
				bool requiresTextUpdate = false;
				int maxLineWidth = 0;
				Vector2 charOffset = Vector2::zero; // might be better and easier to use than aligns
				float lineSpacing = 0.25f;
				float horizontalAlign = 0.5f;
				float verticalAlign = 0.5f;
				Optional<float> letterSize;
				Optional<float> smallerDecimals;
				float scale = 1.0f;
				Optional<float> useAdditionalScale;
				Optional<Colour> backgroundColour;
				bool forceBlend = false;

				List<Line> lines;
				List<String> rawLines;

				struct ScrollDown
				{
					bool active = false;
					float initialWait = 0.0f;
					float postWait = 0.0f;
					Range showLines = Range::empty;
					float speed = 0.0f;
					float atLine = 0.0f;

					bool scrollFaster = false;
				} scrollDown;
			};
		}
	};
};
