#pragma once

#include "iWMPTool.h"

namespace WheresMyPoint
{
	class MathOpSimpleOnVar
	: public ITool
	{
		typedef ITool base;
	public:
		override_ bool load_from_xml(IO::XML::Node const* _node);
	protected:
		bool read_var(REF_ Value& _value, Context& _context) const;
		bool write_var(REF_ Value& _value, Context& _context) const;
	private:
		Name varName;
	};

	//

	class Double
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Half
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Sqr
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Sqrt
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Abs
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Negate
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Invert
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Sign
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Increase
	: public MathOpSimpleOnVar
	{
		typedef MathOpSimpleOnVar base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("increase current value or var's value (var=\"name\")"); }
	};

	class Decrease
	: public MathOpSimpleOnVar
	{
		typedef MathOpSimpleOnVar base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
		override_ tchar const* development_description() const { return TXT("decrease current value or var's value (var=\"name\")"); }
	};

	class IsZero
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value& _value, Context& _context) const;
	};

	class Rotate90Right
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Floor
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

	class Ceil
	: public ITool
	{
		typedef ITool base;
	public: // ITool
		override_ bool update(REF_ Value & _value, Context & _context) const;
	};

};
