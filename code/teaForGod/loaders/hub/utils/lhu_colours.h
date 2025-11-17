#pragma once

#include "..\..\..\..\core\types\optional.h"

struct Colour;

namespace Loader
{
	struct HubScreen;
	struct HubWidget;

	namespace HubWidgets
	{
		struct Grid;
		struct GridDraggable;
	};

	namespace Utils
	{
		namespace ForGridDraggable
		{
			enum Flags
			{
				HighlightSelected = 1
			};
			Optional<Colour> get_colourise(HubWidgets::Grid* w, HubScreen const* screen, HubWidgets::GridDraggable const* _element, int _flags = 0);
			Colour get_border(Optional<Colour> const& _colourise);
			void adjust_colours(REF_ Optional<Colour>& colourise, REF_ Colour& border, HubWidget const* w, HubWidgets::GridDraggable const* _element);
		};
	};
};