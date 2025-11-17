#pragma once

#include "..\framework.h"

#include "..\..\core\concurrency\jobExecutionContext.h"

namespace Framework
{
	class AdvanceContext
	: public Concurrency::JobExecutionContext
	{
	public:
		AdvanceContext();

		void set_delta_time(float _dt) { deltaTime = _dt; }
		float get_delta_time() const { return deltaTime; }

	private:
		float deltaTime;
	};
};
