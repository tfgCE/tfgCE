#pragma once

#include "..\displayTypes.h"

#include "..\displayDrawCommand.h"

#include "..\..\..\core\types\colour.h"

namespace Framework
{
	class TexturePart;
};

namespace Framework
{
	struct DisplayRegion;

	namespace DisplayDrawCommands
	{
		class TexturePart
		: public PooledObject<TexturePart>
		, public DisplayDrawCommand
		{
			typedef DisplayDrawCommand base;
		public:
			TexturePart();

			TexturePart* at(VectorInt2 const & _at) { atParam = _at; return this; }
			TexturePart* at(CharCoord _x, CharCoord _y) { return at(VectorInt2(_x, _y)); }
			TexturePart* use(Framework::TexturePart const * _texturePart) { texturePart = _texturePart; return this; }

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;
			implement_ void prepare(Display* _display);

		protected:
			Optional<VectorInt2> atParam;
			Framework::TexturePart const * texturePart = nullptr;
		};
	};

};
