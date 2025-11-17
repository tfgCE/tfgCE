#pragma once

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\memory\pooledObject.h"
#include "display.h"
#include "displayDrawContext.h"

namespace System
{
	class Video3D;
};

namespace Framework
{
	class Display;

	class DisplayDrawCommand
	: public RefCountObject
	{
	public:
		virtual ~DisplayDrawCommand() {}

		DisplayDrawCommand* use_offset(Vector2 const & _offset) { offset = _offset; return this; }
		DisplayDrawCommand* use_offset(VectorInt2 const & _offset) { offset = _offset.to_vector2(); return this; }
		DisplayDrawCommand* use_char_offset(Display* _display, CharCoords const & _offset) { offset = _display->get_char_size() * _offset.to_vector2(); return this; } // use it wisely, considering use_coordinates!
		DisplayDrawCommand* use_colour_replacer(DisplayColourReplacer* _colourReplacer) { colourReplacer = _colourReplacer; return this; }
		DisplayDrawCommand* no_colour_replacer() { colourReplacer = nullptr; return this; }
		DisplayDrawCommand* use_colourise(Optional<bool> _colourise = true) { colourise = _colourise; return this; }
		DisplayDrawCommand* use_colourise_ink(Optional<Colour> const & _colourise = NP) { colouriseInk = _colourise; if (colouriseInk.is_set()) { use_colourise(); } return this; }
		DisplayDrawCommand* use_colourise_paper(Optional<Colour> const & _colourise = NP) { colourisePaper = _colourise; if (colourisePaper.is_set()) { use_colourise(); } return this; }
		DisplayDrawCommand* use_coordinates(Optional<DisplayCoordinates::Type> const & _useCoordinates = NP) { useCoordinates = _useCoordinates; return this; }
		DisplayDrawCommand* limit_to(Range2 const & _limits) { useLimits = _limits; return this; }

	public:
		Vector2 const & get_offset() const { return offset; }
		DisplayColourReplacer* get_colour_replacer() const { return colourReplacer.get(); }
		bool get_use_colourise(Display * _display) const;
		virtual Colour const & get_use_colourise_ink(Display * _display) const;
		virtual Colour const & get_use_colourise_paper(Display * _display) const;
		DisplayCoordinates::Type get_use_coordinates(Display * _display) const;
		Optional<Range2> const & get_use_limits() const { return useLimits; }

	public:
		// should return if it finished (true) or has to wait (false)
		virtual bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const = 0;

		// called when added to display
		virtual void prepare(Display* _display);

	public:
		DisplayDrawCommand* immediate_draw(bool _immediateDraw = true) { immediateDraw = _immediateDraw; return this; }
		bool should_draw_immediately() const { return immediateDraw; }

	protected:
#ifdef AN_DEVELOPMENT
		bool prepared = false;

		inline void assert_prepared() const { an_assert(prepared); }
#else
		inline void assert_prepared() const {}
#endif

		bool immediateDraw = false;

		Vector2 offset = Vector2::zero;
		RefCountObjectPtr<DisplayColourReplacer> colourReplacer; // colour replacer is handled in display and modifies stack

	private:
		Optional<Colour> colouriseInk;
		Optional<Colour> colourisePaper;
		Optional<bool> colourise;
		Optional<DisplayCoordinates::Type> useCoordinates;
		Optional<Range2> useLimits;
	};

};
