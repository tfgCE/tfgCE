#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class Output
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool load_from_xml(IO::XML::Node const * _node);
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("provide an information (variable's value using \"var\" or \"from\")"); }

	protected:
		String info;
		::Name name;
		::Name debug;

		bool asXML = false;

		void log_variable(Context& _context) const;
	};

	class Log
	: public Output
	{
		typedef Output base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("just log an info"); }
	};

	class Warning
	: public Output
	{
		typedef Output base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("issue a warning (variable's value using \"var\" or \"from\")"); }
	};

	class Error
	: public Output
	{
		typedef Output base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("throw an error (variable's value using \"var\" or \"from\")"); }
	};
};
