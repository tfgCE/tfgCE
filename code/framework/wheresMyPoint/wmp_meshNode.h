#pragma once

#include "..\..\core\wheresMyPoint\wmp.h"

namespace Framework
{
	struct MeshNode;

	class MeshNodeBase
	: public WheresMyPoint::ITool
	{
		typedef ITool base;
	protected:
		MeshNode * access_node(WheresMyPoint::Context & _context) const;
	};

	class MeshNodeName
	: public MeshNodeBase
	{
		typedef MeshNodeBase base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
	};

	class MeshNodePlacement
	: public MeshNodeBase
	{
		typedef MeshNodeBase base;
	public: // ITool
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;
	};

	class MeshNodeStore
	: public MeshNodeBase
	{
		typedef MeshNodeBase base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		::Name name;
	};

	class MeshNodeRestore
	: public MeshNodeBase
	{
		typedef MeshNodeBase base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ WheresMyPoint::Value & _value, WheresMyPoint::Context & _context) const;

	private:
		::Name name;
	};
};
