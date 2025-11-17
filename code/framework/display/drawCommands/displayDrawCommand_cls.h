#pragma once

#include "..\displayDrawCommand.h"

#include "..\..\..\core\types\colour.h"

namespace Framework
{
	namespace DisplayDrawCommands
	{
		class CLS
		: public PooledObject<CLS>
		, public DisplayDrawCommand
		{
			typedef DisplayDrawCommand base;
		public:
			CLS();
			CLS(Colour const & _colour);
			CLS(Name const & _useColourPair);

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;

		private:
			Name useColourPair;
			Optional<Colour> colour;
		};
	};

};
