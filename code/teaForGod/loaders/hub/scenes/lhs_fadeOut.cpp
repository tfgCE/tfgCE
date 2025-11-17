#include "lhs_fadeOut.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\..\..\game\game.h"

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(FadeOut);

FadeOut::FadeOut()
{
}

void FadeOut::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();
}

void FadeOut::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	output(TXT("!@# FadeOut::on_update"));
}
