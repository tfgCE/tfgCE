#include "uiCanvasUpdateWorktable.h"

#include "uiCanvas.h"
#include "uiCanvasElement.h"

#include "..\..\core\system\input.h"
#include "..\..\core\system\video\video3d.h"

//

using namespace Framework;
using namespace UI;

//

CanvasUpdateWorktable::CanvasUpdateWorktable(Canvas* _canvas)
: canvas(_canvas)
{
}

void CanvasUpdateWorktable::hovers_on(int _cursorIdx, ICanvasElement const* _element)
{
	while (hoversOn.get_size() <= _cursorIdx)
	{
		hoversOn.push_back();
	}

	if (!hoversOn[_cursorIdx].is_pointing_at_something() && _element)
	{
		hoversOn[_cursorIdx] = _element;
	}
}

void CanvasUpdateWorktable::set_shortcut_to_activate(ICanvasElement const* _element, int _shortcutIndexToActivate)
{
	shortcutToActivate = _element;
	shortcutIndexToActivate = _shortcutIndexToActivate;
}

ICanvasElement const* CanvasUpdateWorktable::get_shortcut_to_activate() const
{
	return shortcutToActivate.get();
}

int CanvasUpdateWorktable::get_shortcut_index_to_activate() const
{
	return shortcutIndexToActivate;
}
