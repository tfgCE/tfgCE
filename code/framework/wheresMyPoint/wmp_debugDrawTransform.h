#pragma once

#include "..\..\core\types\colour.h"
#include "..\..\core\wheresMyPoint\wmp.h"

struct DebugRendererFrame;

namespace Framework
{
	class DebugDrawTransform
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		// by default takes current value
		WheresMyPoint::ToolSet transform;

		float size = 0.1f;
		Colour colour = Colour::white.with_alpha(0.0f);
	};
};
