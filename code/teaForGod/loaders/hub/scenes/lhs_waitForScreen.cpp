#include "lhs_waitForScreen.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(WaitForScreen);

WaitForScreen::WaitForScreen(HubScreen* _screen)
: screen(_screen)
{
	// screen should not be added now
}

WaitForScreen::WaitForScreen(std::function< HubScreen* (Loader::Hub* _hub)> _create_screen)
: create_screen(_create_screen)
{
	// screen should not be added now
}

void WaitForScreen::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	if (!screen.get() && create_screen)
	{
		screen = create_screen(get_hub());
	}
	get_hub()->allow_to_deactivate_with_loader_immediately(false);
	if (screen.get())
	{
		screen->activate();
		get_hub()->add_screen(screen.get());
	}
}

bool WaitForScreen::does_want_to_end()
{
	if (!screen.get() ||
		!screen->is_active())
	{
		return true;
	}

	return false;
}
