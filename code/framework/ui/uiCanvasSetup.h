#pragma once

#include "..\..\core\types\colour.h"

//

namespace Framework
{
	namespace UI
	{
		struct CanvasColours
		{
			Colour ink;
			Colour paper;

			Colour windowPaper;
			Colour windowFrame;

			Colour windowTitleBarInk;
			Colour windowTitleBarPaper;

			Colour buttonInk;
			Colour buttonPaper;
			Colour buttonPaperHoverOn;
			Colour buttonPaperHighlighted;
			Colour buttonFrame;
			Colour buttonFrameHoverOn; // if button colour defined
			Colour buttonFrameHighlighted; // if button colour defined
			Colour buttonShortcutInk;
		};
		
		struct CanvasSetup
		{
			CanvasSetup();

			CanvasColours colours;

			float textHeight;
			Vector2 fontScale;

			float windowTitleBarHeight;
			float windowTitleBarTextHeight;
			float windowFrameThickness;
			float windowScrollThickness;

			float buttonTextHeight;
			float buttonDefaultHeight;
			Vector2 buttonTextMargin;			
		};
	};
};

TYPE_AS_CHAR(Framework::UI::CanvasColours);
TYPE_AS_CHAR(Framework::UI::CanvasSetup);
