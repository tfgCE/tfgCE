#include "mggc_bullshotSystem.h"

#include "..\..\game\bullshotSystem.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool GenerationConditions::BullshotSystem::check(ElementInstance & _instance) const
{
	return Framework::BullshotSystem::is_active();
}
