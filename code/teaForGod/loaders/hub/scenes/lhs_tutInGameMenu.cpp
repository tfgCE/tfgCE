#include "lhs_tutInGameMenu.h"

#include "..\loaderHub.h"
#include "..\screens\lhc_tutInGameMenu.h"

//

using namespace Loader;
using namespace HubScenes;

//

// screen name
DEFINE_STATIC_NAME(tutInGameMenu);

//

REGISTER_FOR_FAST_CAST(TutInGameMenu);

void TutInGameMenu::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	deactivateHub = false;
	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	show_in_game_menu();
}

void TutInGameMenu::show_in_game_menu()
{
	get_hub()->keep_only_screen(NAME(tutInGameMenu));

	// add buttons: go back to game, exit to main menu, quit game
	HubScreen* screen = get_hub()->get_screen(NAME(tutInGameMenu));
	if (! screen)
	{
		auto* s = new HubScreens::TutInGameMenu(get_hub());
		s->be_stand_alone(this);

		screen = s;

		screen->activate();
		get_hub()->add_screen(screen);
	}
}
