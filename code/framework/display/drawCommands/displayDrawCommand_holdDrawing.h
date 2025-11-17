#pragma once

#include "..\displayDrawCommand.h"

#include "..\..\..\core\types\colour.h"

namespace Framework
{
	namespace DisplayDrawCommands
	{
		class HoldDrawing
		: public PooledObject<HoldDrawing>
		, public DisplayDrawCommand
		{
		public:
			HoldDrawing();

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;
		};
	};

};
