#pragma once

#include "..\..\..\..\core\globalDefinitions.h"
#include "..\..\..\..\core\types\colour.h"
#include "..\..\..\..\core\types\optional.h"

struct Range2;

namespace Framework
{
	class Display;
};

namespace TeaForGodEmperor
{
	class WeaponPart;
};

namespace Loader
{
	interface_class IHubDraggableData;
	struct HubDraggable;
	struct HubScreen;
	struct HubWidget;

	namespace HubWidgets
	{
		struct Grid;
		struct GridDraggable;
	}

	namespace Utils
	{
		struct Grid
		{
		public:
			static void draw_grid_element(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, HubScreen const* screen, HubWidgets::GridDraggable const* _element, Optional<float> const & _smallerByPt = NP, Optional<Colour> const& _overrideColour = NP, Optional<float> const & _doubleLine = NP, Optional<Colour> const& _blendTotalColour = NP);
		};
	};
};
