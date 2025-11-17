#pragma once

#include "..\displayDrawCommand.h"

#include "..\..\..\core\types\colour.h"

namespace Framework
{
	namespace DisplayDrawCommands
	{
		class ClearRegion
		: public PooledObject<ClearRegion>
		, public DisplayDrawCommand
		{
			typedef DisplayDrawCommand base;
		public:
			ClearRegion() {}
			
			ClearRegion* region(Name const & _regionName) { regionName = _regionName; return this; }
			ClearRegion* paper(Colour const & _colour) { colour = _colour; return this; }
			ClearRegion* use_default_colour_pair() { useDefaultColourPairParam = true; return this; }
			ClearRegion* use_colour_pair(Name const & _colourPair) { useColourPair = _colourPair; return this; }
			ClearRegion* rect(RangeCharCoord2 const & _regionRect) { regionRect = _regionRect; return this; }
			ClearRegion* rect(CharCoords const & _leftBottom, CharCoords const & _size) { regionRect = RangeCharCoord2(RangeCharCoord(_leftBottom.x, _leftBottom.x + _size.x - 1), RangeCharCoord(_leftBottom.y, _leftBottom.y + _size.y - 1)); return this; }
			ClearRegion* rect(CharCoord _x0, CharCoord _y0, CharCoord _x1, CharCoord _y1) { regionRect = RangeCharCoord2(RangeCharCoord(_x0, _x1), RangeCharCoord(_y0, _y1)); return this; }
			ClearRegion* rect_within_region_from_top_left(Name const & _regionName, CharCoord _x0, CharCoord _y0, CharCoord _x1, CharCoord _y1) { regionName = _regionName;  withinRegionRectFromTopLeft = RangeCharCoord2(RangeCharCoord(_x0, _x1), RangeCharCoord(_y0, _y1)); return this; }

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;

		private:
			Name regionName;
			Optional<bool> useDefaultColourPairParam;
			Name useColourPair;
			Optional<Colour> colour;
			Optional<RangeCharCoord2> regionRect;
			Optional<RangeCharCoord2> withinRegionRectFromTopLeft;
		};
	};

};
