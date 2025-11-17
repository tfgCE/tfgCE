#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Restore
	: public IPrefixedTool
	{
		typedef IPrefixedTool base;
	public:
		Restore() {}
		Restore(::Name const & _name) : name(_name) {}

	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool load_prefixed_from_xml(IO::XML::Node const * _node, String const& _prefix);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
		Array<::Name> subfields;
	};

	class RestoreGlobal
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
	};

	class RestoreTemp
	: public IPrefixedTool
	{
		typedef IPrefixedTool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool load_prefixed_from_xml(IO::XML::Node const* _node, String const& _prefix);
		override_ bool update(REF_ Value & _value, Context & _context) const;

	private:
		::Name name;
		Array<::Name> subfields;
	};
};
