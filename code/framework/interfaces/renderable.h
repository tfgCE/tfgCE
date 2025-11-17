#pragma once

namespace System
{
	class Video3D;
}

namespace Framework
{
	namespace Render
	{
		class Context;
	}

	interface_class IRenderableObject
	{
	public:
		virtual void render(Render::Context & _context) = 0;
	};
};

