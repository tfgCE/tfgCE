#pragma once

#include "..\ai\aiLogic.h"

namespace Framework
{
	class PreviewAILogic
	: public AI::Logic
	{
		FAST_CAST_DECLARE(PreviewAILogic);
		FAST_CAST_BASE(AI::Logic);
		FAST_CAST_END();

		typedef AI::Logic base;
	public:
		PreviewAILogic(AI::MindInstance* _mind);
		virtual ~PreviewAILogic();

	public: // AI::Logic
		override_ void advance(float _deltaTime);
	};
};
