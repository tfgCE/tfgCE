#pragma once

#include "..\displayDrawCommand.h"

#include "..\..\..\core\types\colour.h"

namespace Framework
{
	namespace DisplayDrawCommands
	{
		class Custom
		: public PooledObject<Custom>
		, public DisplayDrawCommand
		{
			typedef DisplayDrawCommand base;
		public:
			typedef std::function<bool(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context)> DrawCustom;
			typedef std::function<void(Display* _display, ::System::Video3D * _v3d)> DrawCustomSimple;

			Custom();

			Custom* use(DrawCustom _draw_custom);
			Custom* use_simple(DrawCustomSimple _draw_custom_simple);

		public: // DisplayDrawCommand
			implement_ bool draw_onto(Display* _display, ::System::Video3D * _v3d, REF_ int & _drawCyclesUsed, REF_ int & _drawCyclesAvailable, REF_ DisplayDrawContext & _context) const;

		private:
			DrawCustom draw_custom = nullptr;
			DrawCustomSimple draw_custom_simple = nullptr;
		};
	};

};
