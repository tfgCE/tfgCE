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
		struct WeaponPartDraw
		{
		public:
			struct Context
			{
				bool preDeactivationTransfer;
				int preDeactivationLevel;
				int activeBoosts;

				// has to be in a separate constructor
				Context()
				: preDeactivationTransfer(false)
				, preDeactivationLevel(0)
				, activeBoosts(0)
				{}

				Context& pre_deactivation_transfer(bool _v) { preDeactivationTransfer = _v; return *this; }
				Context& pre_deactivation_level(int _v) { preDeactivationLevel = _v; return *this; }
				Context& active_boosts(int _v) { activeBoosts = _v; return *this; }
			};
			enum DrawFlags
			{
				Outline = 1,
				WithTransferChance = 2
			};
			static void draw_grid_element(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, HubScreen const* screen, HubWidgets::GridDraggable const* _element, Optional<Colour> const& _overrideColour = NP, Optional<Colour> const& _blendTotalColour = NP);
			static void draw_draggable_at(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, HubWidgets::GridDraggable const* _element, Context const& _context = Context(), int _flags = 0);
			static void draw_draggable_at(Framework::Display* _display, HubWidgets::Grid* w, Range2 const& _at, TeaForGodEmperor::WeaponPart* wp, Context const& _context = Context(), int _flags = 0);
			static void draw_element_at(Framework::Display* _display, Loader::HubWidget * _widget, Loader::HubDraggable const* _draggable, Range2 const& _at, TeaForGodEmperor::WeaponPart const* _wp, Context const& _context = Context(), int _flags = 0);
			static Colour get_transfer_chance_colour(int transferChance);
		};
	};
};
