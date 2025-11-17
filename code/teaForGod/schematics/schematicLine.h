#pragma once

#include "schematicElement.h"

#include "..\..\core\containers\array.h"
#include "..\..\core\math\math.h"

namespace TeaForGodEmperor
{
	class Schematic;

	struct SchematicLine
	: public SchematicElement
	{
		FAST_CAST_DECLARE(SchematicLine);
		FAST_CAST_BASE(SchematicElement);
		FAST_CAST_END();

		typedef SchematicElement base;
	public:
		void add(Vector2 const& _point) { points.push_back(_point); }
		void add_from(SchematicLine* _other) { points.add_from(_other->points); }
		void be_companion_of(SchematicLine const & _other, float _dist);
		void set_looped(bool _looped = true) { looped = _looped; }
		void set_filled(Optional<Colour> const & _filled = NP) { filled = _filled; }
		void set_outline(bool _outline) { outline = _outline; }
		void set_line_width(float _lineWidth) { lineWidth = _lineWidth; }
		void set_colour(Colour const& _colour, Optional<Colour> const& _halfColour = NP) { colour = _colour; halfColour = _halfColour; }

		void cut_outer_lines_with(SchematicLine const & _convexLine, Schematic * _addNewLinesTo) { cut_lines_with(_convexLine, _addNewLinesTo, true); }
		void cut_inner_lines_with(SchematicLine const & _convexLine, Schematic * _addNewLinesTo) { cut_lines_with(_convexLine, _addNewLinesTo, false); }

	public:
		implement_ void build(Meshes::Builders::IPU& _ipu, bool _outline) const;
		override_ bool load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);
		implement_ void grow_size(REF_ Range2& _size) const;

	private:
		bool looped = false;
		bool outline = true;
		Optional<Colour> filled;
		float lineWidth = 0.001f;
		Colour colour = Colour::white;
		Colour colourAsOutline = Colour::white;
		Optional<Colour> halfColour;
		Array<Vector2> points;

		void cut_lines_with(SchematicLine const & _convexLine, Schematic* _addNewLinesTo, bool _removeOutside);
	};
}

