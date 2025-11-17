#pragma once

#include "..\..\core\latent\latent.h"

#include "functions\latentFunctions.h"

#define LATENT_WAIT_FOR_TASK(_howLong, _task) \
	LATENT_CALL_FUNCTION_2(Framework::Latent::Functions::wait_for_task, _howLong, _task)
