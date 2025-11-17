#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

#include "..\library\libraryName.h"

namespace Framework
{
	struct MeshNode;

	class UsedEnvironmentFromRegion
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	};

	class UseEnvironmentFromRegion
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		Name useFromRegion;
		WheresMyPoint::ToolSet toolSet;
	};

	class AddEnvironmentOverlay
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		LibraryName environmentOverlay;
		WheresMyPoint::ToolSet toolSet;
	};

	class ClearEnvironmentOverlays
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
	};

};
