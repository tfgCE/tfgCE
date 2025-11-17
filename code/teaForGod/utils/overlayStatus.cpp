#include "overlayStatus.h"

#include "..\overlayInfo\overlayInfoElement.h"
#include "..\overlayInfo\overlayInfoSystem.h"
#include "..\overlayInfo\elements\oie_text.h"

#include "..\..\framework\text\localisedString.h"

//

using namespace TeaForGodEmperor;

//

OverlayStatus::OverlayStatus()
{

}

OverlayStatus::~OverlayStatus()
{
	if (auto* e = statusOIE.get())
	{
		e->deactivate();
	}
}

void OverlayStatus::update(Optional<int> const& _value)
{
	if (isShown)
	{
		if (auto* e = statusOIE.get())
		{
			if (!e->is_active())
			{
				statusOIE.clear();
				statusValue.clear();
			}
		}
		if (!statusOIE.get())
		{
			if (auto* overlayInfoSystem = OverlayInfo::System::get())
			{
				String text;

				auto* e = new OverlayInfo::Elements::Text(text);
				Rotator3 offset = Rotator3(-20.0f, 0.0f, 0.0f);

				e->with_location(OverlayInfo::Element::Relativity::RelativeToCamera);
				e->with_rotation(OverlayInfo::Element::Relativity::RelativeToCamera, offset);
				e->with_distance(1.0f);

				e->with_scale(2.0f);
				e->with_max_line_width(50);

				e->with_vertical_align(1.0f);

				e->set_cant_be_deactivated_by_system(); // will be forced or deactivate here explicitly

				overlayInfoSystem->add(e);

				statusOIE = e;
			}
		}
		if (auto* e = statusOIE.get())
		{
			bool updateColour = forceColourUpdate;
			Optional<int> newValue = _value;
			if (newValue.is_set() &&
				(!statusValue.is_set() ||
					statusValue.get() != newValue.get()))
			{
				statusValue = newValue;
				String text;
				if (prefixLS.is_valid())
				{
					text = String::printf(TXT("%S%i%%"), LOC_STR(prefixLS).to_char(), newValue.get());
				}
				else
				{
					text = String::printf(TXT("%i%%"), newValue.get());
				}

				if (auto* et = fast_cast<OverlayInfo::Elements::Text>(e))
				{
					et->with_text(text);
				}
				updateColour = true;
			}
			if (updateColour)
			{
				if (auto* et = fast_cast<OverlayInfo::Elements::Text>(e))
				{
					Colour useColour = colour.get(Colour::white);
					if (highlightAtValueOrBelow.is_set())
					{
						if (newValue.get() <= highlightAtValueOrBelow.get())
						{
							useColour = highlightColour.get(Colour::red);
						}
					}
					et->with_colour(useColour);

					forceColourUpdate = false;
				}
			}
		}
	}
	else
	{
		if (auto* e = statusOIE.get())
		{
			e->deactivate();
			statusOIE.clear();
			statusValue.clear();
		}
	}
}
