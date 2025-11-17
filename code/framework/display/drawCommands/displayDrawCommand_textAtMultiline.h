#pragma once

#include "displayDrawCommand_textAtBase.h"

namespace Framework
{
	struct DisplayRegion;

	namespace DisplayDrawCommands
	{
		/**
		 *	Divides into lines automatically
		 *	Breaks text on ~ character (new line)
		 */
		class TextAtMultiline
		: public PooledObject<TextAtMultiline>
		, public TextAtBase
		{
			typedef TextAtBase base;
		public:
			TextAtMultiline();

			int get_lines_count() const { assert_prepared(); return max(1, textParamLines); }
			int get_max_line_length() const;
			int get_vertical_size() const { assert_prepared(); return verticalSize; }

			// both are safe to set after prepare/add draw command
			TextAtMultiline* start_at_line(int _startAtLine) { startAtLine = _startAtLine; return this; }
			TextAtMultiline* end_at_line(int _endAtLine) { endAtLine = _endAtLine; return this; }

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;
			implement_ void prepare(Display* _display);

		private:
			static int const MAX_LINES = 16;
			struct LineProp
			{
				int startsAt = 0;
				int length = 0;
				LineProp() {}
				LineProp(int _startsAt, int _length) : startsAt(_startsAt), length(_length) {}
			};
			LineProp textParamMultiline[MAX_LINES];
			Array<LineProp> textParamMultilineExt; // if normal doesn't handle
			int textParamLines;
			int startAtLine = 0;
			int endAtLine = -INF_INT; // if lesser than start, there is no end
			int verticalSize;

			int get_line_length(int _idx) const;
		};
	};

};

