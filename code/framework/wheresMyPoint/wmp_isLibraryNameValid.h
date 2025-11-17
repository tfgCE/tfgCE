#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	struct MeshNode;

	class IsLibraryNameValid
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	public:
		static bool is_valid(WheresMyPoint::Value const& _value, REF_ WheresMyPoint::Value& _resultValue);
	};

	class IfLibraryNameEmpty
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const;

	private:
		WheresMyPoint::ToolSet doIfEmptyToolSet;
	};

	class IfLibraryNameValid
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const* _node);
		override_ bool update(REF_ WheresMyPoint::Value& _value, WheresMyPoint::Context& _context) const;

	private:
		WheresMyPoint::ToolSet doIfValidToolSet;
	};
};
