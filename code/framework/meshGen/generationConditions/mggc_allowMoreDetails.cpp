#include "mggc_allowMoreDetails.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\io\xml.h"

#include "..\..\game\game.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

AllowMoreDetails::~AllowMoreDetails()
{
}

bool AllowMoreDetails::check(ElementInstance& _instance) const
{
	if (_instance.get_context().does_force_more_details())
	{
		return true;
	}
	if (auto * g = Game::get())
	{
		return g->should_generate_more_details();
	}
	return true;
}
