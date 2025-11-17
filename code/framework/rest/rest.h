#pragma once

#include "..\..\core\io\json.h"
#include "..\..\core\memory\refCountObject.h"
#include "..\..\core\types\optional.h"
#include "..\..\core\types\string.h"

#include <functional>

namespace Framework
{
	namespace REST
	{
		RefCountObjectPtr<IO::JSON::Document> send(tchar const* address, tchar const* command, IO::JSON::Document const * _message, std::function<void(float _progress)> _update_progress = nullptr);
	};
};
