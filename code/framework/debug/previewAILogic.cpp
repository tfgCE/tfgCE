#include "previewAILogic.h"

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(PreviewAILogic);

PreviewAILogic::PreviewAILogic(AI::MindInstance* _mind)
: base(_mind, nullptr)
{

}

PreviewAILogic::~PreviewAILogic()
{
}

void PreviewAILogic::advance(float _deltaTime)
{
	// nothing?
}