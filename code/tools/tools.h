#pragma once

#include "..\framework\framework.h"

#include "fbx\fbx.h"
#include "simpleImporters\simpleImporters.h"

namespace Tools
{
	void initialise_static();
	void close_static();

	void finished_importing();
};
