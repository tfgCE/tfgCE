#pragma once

#include "wmp_debugDrawLine.h"

struct DebugRendererFrame;

namespace Framework
{
	class DebugDrawArrow
	: public DebugDrawLine
	{
		typedef ITool base;
	protected: // DebugDrawLine
		virtual void draw(DebugRendererFrame* _drf, Colour const & _colour, Vector3 const & _a, Vector3 const & _b) const;
	};
};
