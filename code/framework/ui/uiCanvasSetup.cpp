#include "uiCanvasSetup.h"

//

using namespace Framework;
using namespace UI;

//

CanvasSetup::CanvasSetup()
{
	colours.ink = Colour::blackWarm;
	colours.paper = Colour::whiteWarm;

	colours.windowPaper = Colour::whiteWarm;
	colours.windowFrame = Colour::grey;

	colours.windowTitleBarInk = Colour::blackWarm;
	colours.windowTitleBarPaper = Colour::lerp(0.5f, Colour::whiteWarm, Colour::grey);

	colours.buttonInk = Colour::blackWarm;
	colours.buttonPaper = Colour::greyLight;
	colours.buttonPaperHoverOn = Colour::lerp(0.5f, colours.buttonPaper, Colour::c64Yellow);
	colours.buttonPaperHighlighted = Colour::lerp(0.5f, colours.buttonPaper, Colour::c64Orange);
	colours.buttonFrame = Colour::grey;
	colours.buttonFrameHoverOn = Colour::lerp(0.5f, colours.buttonPaper, Colour::c64Yellow);
	colours.buttonFrameHighlighted = Colour::lerp(0.9f, colours.buttonPaper, Colour::zxRedBright);
	colours.buttonShortcutInk = Colour::lerp(0.9f, Colour::blackWarm, Colour::c64Red);;

	textHeight = 26.0f;
	fontScale = Vector2(0.9f, 1.0f);

	windowTitleBarHeight = 30.0f;
	windowTitleBarTextHeight = textHeight;
	windowFrameThickness = 2.0f;
	windowScrollThickness = 20.0f;

	buttonTextHeight = textHeight;
	buttonDefaultHeight = 30.0f;

	buttonTextMargin = Vector2(5.0f, 2.0f);
}
