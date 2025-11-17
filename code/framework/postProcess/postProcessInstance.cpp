#include "postProcessInstance.h"

#include "..\..\core\graph\graphImplementation.h"

#include "postProcess.h"

using namespace Framework;

PostProcessInstance::PostProcessInstance(PostProcess* _postProcess)
: postProcess(_postProcess)
{
	graphInstance = postProcess->get_graph()->create_instance();
}

PostProcessInstance::~PostProcessInstance()
{
	delete_and_clear(graphInstance);
}

void PostProcessInstance::initialise()
{
	graphInstance->initialise(this);
}

