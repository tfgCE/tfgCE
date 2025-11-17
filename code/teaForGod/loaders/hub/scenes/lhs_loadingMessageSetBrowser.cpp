#include "lhs_loadingMessageSetBrowser.h"

#include "..\loaderHub.h"

#include "..\screens\lhc_messageSetBrowser.h"

#include "..\..\..\library\library.h"

#include "..\..\..\..\framework\game\bullshotSystem.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;
using namespace HubScenes;

//

// ids
DEFINE_STATIC_NAME(idLoadingMessageSetBrowser);
DEFINE_STATIC_NAME(idClose);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsSpaceRequiredOnLoad, TXT("space required on load"));

//

REGISTER_FOR_FAST_CAST(LoadingMessageSetBrowser);

LoadingMessageSetBrowser::LoadingMessageSetBrowser(TagCondition const& _messageSetsTagged, Optional<String> const & _title, Optional<float> const& _delay, Optional<Name> const & _startWithMessage, Optional<bool> const & _randomOrder, Optional<bool> const& _startWithRandomMessage)
: messageSetsTagged(_messageSetsTagged)
, title(_title.get(String::empty()))
, startWithMessage(_startWithMessage)
, randomOrder(_randomOrder)
, startWithRandomMessage(_startWithRandomMessage)
, delay(_delay.get(0.0f))
{
}

void LoadingMessageSetBrowser::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	loaderDeactivated = false;
	timeToShow = delay;

	// always disallow
	get_hub()->allow_to_deactivate_with_loader_immediately(false);
	 
	get_hub()->deactivate_all_screens();

	if (timeToShow <= 0.0f)
	{
		show();
	}
}

void LoadingMessageSetBrowser::show()
{
	get_hub()->deactivate_all_screens();
	
	HubScreen* screen = nullptr;

	if (!screen)
	{
		Array<TeaForGodEmperor::MessageSet*> messageSets;
		auto* library = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>();
		for_every_ptr(ms, library->get_message_sets().get_tagged(messageSetsTagged))
		{
			messageSets.push_back(ms);
		}

		Vector2 ppa(24.0f, 24.0f);
		Vector2 size(40.0f, 30.0f);

		HubScreens::MessageSetBrowser::BrowseParams params;
		params.for_loading();
		params.in_random_order(randomOrder);
		params.start_with_message(startWithMessage);
		params.start_with_random_message(startWithRandomMessage);
		params.with_title(title);
		auto* msb = HubScreens::MessageSetBrowser::browse(messageSets, params, get_hub(), NAME(idLoadingMessageSetBrowser), true, size, NP, NP, NP, NP, ppa, 1.0f, Vector2(0.0f, 1.0f));
		msb->alwaysProcessInput = true;
		if (msb && allowEarlyClose)
		{
			msb->may_close();
		}
	}
}

void LoadingMessageSetBrowser::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
	if (timeToShow > 0.0f && !does_want_to_end())
	{
		timeToShow -= _deltaTime;
		if (timeToShow <= 0.0f)
		{
			show();
		}
	}

	if (blinkCloseTimeLeft.is_set())
	{
		blinkCloseTimeLeft = blinkCloseTimeLeft.get() - _deltaTime;

		if (blinkCloseTimeLeft.get() <= 0.0f)
		{
			blinkCloseTimeLeft = 5.0f;

			get_hub()->reset_highlights();
			get_hub()->add_highlight_blink(NAME(idLoadingMessageSetBrowser), NAME(idClose));
		}
	}
}

void LoadingMessageSetBrowser::on_deactivate(HubScene* _next)
{
	if (does_want_to_end())
	{
		base::on_deactivate(_next);
	}
}

void LoadingMessageSetBrowser::on_loader_deactivate()
{
	base::on_loader_deactivate();
	loaderDeactivated = true;

	if (auto* msb = fast_cast<HubScreens::MessageSetBrowser>(get_hub()->get_screen(NAME(idLoadingMessageSetBrowser))))
	{
		msb->may_close();

		if (!blinkCloseTimeLeft.is_set())
		{
			blinkCloseTimeLeft = 0.0f;
		}
	}
}

bool LoadingMessageSetBrowser::does_want_to_end()
{
	if (! allowEarlyClose && !loaderDeactivated)
	{
		return false;
	}
	if (conditionToClose)
	{
		if (!conditionToClose())
		{
			return false;
		}
	}
#ifdef AN_ALLOW_BULLSHOTS
#ifdef AN_STANDARD_INPUT
	if (Framework::BullshotSystem::is_setting_active(NAME(bsSpaceRequiredOnLoad)))
	{
		if (auto* i = ::System::Input::get())
		{
			if (!i->is_key_pressed(::System::Key::Space))
			{
				return false;
			}
		}
	}
#endif
#endif
	if (auto* msb = fast_cast<HubScreens::MessageSetBrowser>(get_hub()->get_screen(NAME(idLoadingMessageSetBrowser))))
	{
		if (msb->is_active())
		{
			return false;
		}
	}
	return loaderDeactivated;
}
