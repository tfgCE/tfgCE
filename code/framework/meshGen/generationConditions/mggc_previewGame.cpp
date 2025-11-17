#include "mggc_previewGame.h"

#include "..\..\framework.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool PreviewGame::check(ElementInstance & _instance) const
{
	return Framework::is_preview_game();
}
