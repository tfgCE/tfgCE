#pragma once

#include "..\..\core\types\colour.h"
#include "..\..\core\wheresMyPoint\wmp.h"

struct DebugRendererFrame;

namespace Framework
{
	class DebugDrawLine
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	protected:
		virtual void draw(DebugRendererFrame* _drf, Colour const & _colour, Vector3 const & _a, Vector3 const & _b) const;

	private:
		// by default takes current value
		WheresMyPoint::ToolSet from;
		WheresMyPoint::ToolSet to;

		Colour colour = Colour::white;
	};
};
