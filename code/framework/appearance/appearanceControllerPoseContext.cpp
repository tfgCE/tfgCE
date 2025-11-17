#include "appearanceControllerPoseContext.h"

using namespace Framework;

AppearanceControllerPoseContext::AppearanceControllerPoseContext()
{
	reset_processing();
}

void AppearanceControllerPoseContext::reset_processing()
{
	processingOrder = NONE;
	nextProcessingOrder = 0;
}
