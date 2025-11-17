#pragma once

#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\types\optional.h"

namespace TeaForGodEmperor
{
	namespace OverlayInfo
	{
		struct Element;
	};

	struct OverlayStatus
	{
		OverlayStatus();
		~OverlayStatus();
		
		void set_prefix(Name const& _locStr) { prefixLS = _locStr; }
		
		void set_colour(Optional<Colour> const& _colour) { colour = _colour; forceColourUpdate = true; }
		void set_highlight_colour(Optional<Colour> const& _highlightColour) { highlightColour = _highlightColour; forceColourUpdate = true; }
		void set_highlight_at_value_or_below(Optional<int> const& _highlightAtValueOrBelow) { highlightAtValueOrBelow = _highlightAtValueOrBelow; forceColourUpdate = true; }

		void update(Optional<int> const& _value);

		void show(bool _show = true) { isShown = _show; }

		bool is_shown() const { return isShown; }

	private:
		bool forceColourUpdate = false;
		Optional<Colour> colour;
		Optional<Colour> highlightColour;
		Optional<int> highlightAtValueOrBelow;

		bool isShown = false;
		RefCountObjectPtr<OverlayInfo::Element> statusOIE;
		Optional<int> statusValue; // percentage

		Name prefixLS;
	};
};
