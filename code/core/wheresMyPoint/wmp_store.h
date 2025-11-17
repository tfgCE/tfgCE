#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Store
	: public IPrefixedTool
	{
		typedef IPrefixedTool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
	};

	class StoreGlobal
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
	};

	class StoreTemp
	: public IPrefixedTool
	{
		typedef IPrefixedTool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
	};

	class StoreSwap
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("swap \"what\" with \"with\" in a store"); }

	private:
		::Name what, with;
	};
};
