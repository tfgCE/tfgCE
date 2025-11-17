#include "loaderHubWidget.h"

#include "loaderHub.h"
#include "loaderHubScreen.h"

#include "..\..\tutorials\tutorialSystem.h"

//

using namespace Loader;

//

REGISTER_FOR_FAST_CAST(HubWidget);

void HubWidget::advance(Hub* _hub, HubScreen* _screen, float _deltaTime, bool _beyondModal)
{
	if (ignoreInput)
	{
		return;
	}
	for_count(int, hIdx, 2)
	{
		auto & hand = _hub->access_hand((::Hand::Type)hIdx);
		if (hand.onScreen == _screen && _screen->is_active())
		{
			if (at.does_contain(hand.onScreenAt))
			{
				if (TeaForGodEmperor::TutorialSystem::does_allow_interaction(_screen, this, hand.dragged.get()) &&
					(TeaForGodEmperor::TutorialSystem::is_hub_input_allowed() || TeaForGodEmperor::TutorialSystem::is_hub_over_widget_allowed()))
				{
					hand.overWidget = this;
				}
			}
		}
	}
}

void HubWidget::play(Framework::UsedLibraryStored<Framework::Sample> const & _sample) const
{
	if (_sample.is_set())
	{
		if (auto* sample = _sample->get_sample())
		{
			sample->play();
		}
	}
}

void HubWidget::clean_up()
{
	on_click = nullptr;
	on_double_click = nullptr;
	on_hold = nullptr;
	on_hold_grip = nullptr;
	on_release = nullptr;

	on_select = nullptr;
	can_hover = nullptr;
	can_drop = nullptr;
	can_drop_discard = nullptr;
	on_drop_discard = nullptr;
	should_stay_on_drag = nullptr;
	on_drop_change = nullptr;
	on_drag_begin = nullptr;
	on_drag_drop = nullptr;
	on_drag_done = nullptr;
}

