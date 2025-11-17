#include "pipelines.h"

#include "colourClashPipeline.h"
#include "displayPipeline.h"
#include "postProcessPipeline.h"
#include "renderingPipeline.h"
#include "uiPipeline.h"

using namespace Framework;

void Pipelines::initialise_static()
{
	ColourClashPipeline::initialise_static();
	DisplayPipeline::initialise_static();
	PostProcessPipeline::initialise_static();
	RenderingPipeline::initialise_static();
	UIPipeline::initialise_static();
}

void Pipelines::setup_static()
{
	ColourClashPipeline::setup_static();
	DisplayPipeline::setup_static();
	PostProcessPipeline::setup_static();
	RenderingPipeline::setup_static();
	UIPipeline::setup_static();
}

void Pipelines::close_static()
{
	ColourClashPipeline::close_static();
	DisplayPipeline::close_static();
	PostProcessPipeline::close_static();
	RenderingPipeline::close_static();
	UIPipeline::close_static();
}

