#pragma once

#include "..\..\core\math\math.h"
#include "..\..\core\memory\refCountObjectPtr.h"

#include <functional>

//

// these can be defined in buildVer_*_definitions.h
#ifndef USE_DISPLAY_SHADOW_MASK
//#define USE_DISPLAY_SHADOW_MASK
#endif
#ifndef USE_DISPLAY_MESH
//#define USE_DISPLAY_MESH
#endif
#ifndef USE_DISPLAY_VR_SCENE
//#define USE_DISPLAY_VR_SCENE
#endif

//

struct String;

namespace Framework
{
	class Display;

	class DisplayDrawCommand;
	typedef RefCountObjectPtr<Display> DisplayPtr;
	typedef RefCountObjectPtr<DisplayDrawCommand> DisplayDrawCommandPtr;

	typedef int CharCoord;
	typedef Vector2 SubCharCoords;
	typedef VectorInt2 CharCoords;
	typedef RangeInt RangeCharCoord;
	typedef RangeInt2 RangeCharCoord2;

	namespace DisplayHAlignment
	{
		enum Type
		{
			Left,
			Centre,
			CentreRight, // if on half, prefer right
			CentreFine, // if on half, be on half
			Right
		};

		DisplayHAlignment::Type parse(String const & _string, DisplayHAlignment::Type _default = DisplayHAlignment::Left);
	};

	namespace DisplayVAlignment
	{
		enum Type
		{
			Top,
			Centre,
			CentreBottom, // if on half, prefer bottom
			CentreFine, // if on half, be on half
			Bottom
		};

		DisplayVAlignment::Type parse(String const & _string, DisplayVAlignment::Type _default = DisplayVAlignment::Top);
	};

	namespace DisplaySelectorDir
	{
		enum Type
		{
			None,
			Up,
			Down,
			Left,
			Right,
			Prev, // try first to go left, if not possible, go up (prev in line above), doesn't loop
			Next, // try first to go right, if not possible, go down (next in line below), doesn't loop
			PrevInLineAbove,
			NextInLineBelow,
			ScrollUp,
			ScrollDown,
			PageUp,
			PageDown,
			Home,
			End,
		};

		Vector2 to_vector2(DisplaySelectorDir::Type _dir);
	};

	typedef std::function< void(Display* _display) > DoOnDisplay;

	namespace DisplayState
	{
		enum Type
		{
			Off,
			On,
		};
	};

	namespace DisplayBackgroundFillMode
	{
		enum Type
		{
			Screen, // fills hole screen
			Whole, // fills everything
			ScreenRect // requires parameters relative to screen to be provided
		};
	};

	namespace DisplayControlDrag
	{
		enum Type
		{
			NotAvailable,
			ActivateOrDrag, // select or drag
			DragImmediately, // drag immediately
		};
	};

	namespace DisplayCursorState
	{
		enum Type
		{
			None,
			WaitForClick, // to decide whether it's a click or nothing, this is specific for touching
			ClickOrDrag, // to decide
			Click,
			Drag,
			ClickOrDoubleClickFirstClick,
			ClickOrDoubleClickBetweenClicks, //
			DoubleClick,
			WaitUntilRelease, // special state to handle situations when we dealt with a control but it was removed
		};
	};

	namespace DisplayCoordinates
	{
		enum Type
		{
			Char,
			Pixel
		};

		Type parse(String const & _value, Type _default = Char);
		tchar const * to_char(Type _value);
	};

	namespace DisplayCursorVisible
	{
		enum Type
		{
			No,
			IfActive,
			Always,
		};
	};
};
