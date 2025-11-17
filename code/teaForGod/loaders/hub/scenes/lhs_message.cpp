#include "lhs_message.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\screens\lhc_message.h"

//

DEFINE_STATIC_NAME(message);

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(Message);

Message::Message(String const & _message, float _delay)
: message(_message)
, delay(_delay)
{
}

Message::Message(tchar const * _message, float _delay)
: message(_message)
, delay(_delay)
{
}

Message::Message(Name const & _locStrId, float _delay)
: message(LOC_STR(_locStrId))
, delay(_delay)
{
}

void Message::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	output(TXT("HubScenes::Message::on_activate \"%S\""), message.to_char());

	messageAccepted = false;
	timeToShow = delay;

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	if (timeToShow <= 0.0f)
	{
		show();
	}
}

void Message::show()
{
	HubScreens::Message::show(get_hub(), message, [this]() { messageAccepted = true; });
}

void Message::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
	if (timeToShow > 0.0f && !messageAccepted)
	{
		timeToShow -= _deltaTime;
		if (timeToShow <= 0.0f)
		{
			show();
		}
	}
}

void Message::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
	if (! requiresAcceptance)
	{
		messageAccepted = true; // pretend it was accepted
	}
}

void Message::on_loader_deactivate()
{
	base::on_loader_deactivate();
}
