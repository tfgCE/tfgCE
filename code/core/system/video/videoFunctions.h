#pragma once

#include <functional>

namespace System
{
	class ShaderProgram;

	typedef std::function<void(ShaderProgram*)> SetupShaderProgram;
};
