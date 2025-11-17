#pragma once

#include "..\globalDefinitions.h"
#include "..\defaults.h"
#include "..\containers\array.h"
#include "..\memory\refCountObject.h"
#include "..\other\factoryOfNamed.h"
#include "..\other\registeredType.h"
#include "..\types\name.h"

#include <functional>

struct Vector3;

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

#define REGISTER_WHERES_MY_POINT_TOOL(name, create_tool) \
	::WheresMyPoint::RegisteredTools::add(::String(name), create_tool);

#define REGISTER_WHERES_MY_POINT_TOOL_PREFIXED(name, create_tool) \
	::WheresMyPoint::RegisteredPrefixedTools::add(::String(name), create_tool);

#define error_processing_wmp(msg, ...) { ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR in WMP] %S : "), get_source_location().to_char()); report_func(msg, ##__VA_ARGS__); output_colour(); }
#define warn_processing_wmp(msg, ...) { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning in WMP] %S : "), get_source_location().to_char()); report_func(msg, ##__VA_ARGS__); output_colour(); }
#define error_processing_wmp_tool(tool, msg, ...) { ScopedOutputLock outputLock; output_colour(1,0,0,1); trace_func(TXT("[ERROR in WMP] %S : "), tool->get_source_location().to_char()); report_func(msg, ##__VA_ARGS__); output_colour(); }
#define warn_processing_wmp_tool(tool, msg, ...) { ScopedOutputLock outputLock; output_colour(1,1,0,1); trace_func(TXT("[warning in WMP] %S : "), tool->get_source_location().to_char()); report_func(msg, ##__VA_ARGS__); output_colour(); }

namespace WheresMyPoint
{
	interface_class IOwner;
	interface_class ITool;
	struct Context;
	class RegisteredCustomTool;

	// only raw data accepted! (if you want to have some other data, use convertStore)
	struct Value
	{
	public:
		template <typename Class>
		Class get_as() const;

		template <typename Class>
		Class & access_as();

		template <typename Class>
		void set(Class const & _value);

		void const * get_raw() const { return valueRaw; }
		void * access_raw() { return valueRaw; }
		void set_raw(TypeId _id, void const * _raw) { type = _id; an_assert(RegisteredType::get_size_of(_id) <= MAX_SIZE); an_assert(RegisteredType::is_plain_data(_id)); memory_copy(valueRaw, _raw, RegisteredType::get_size_of(_id)); }
		void read_raw(TypeId _id, void * _raw) const { an_assert(type == _id); an_assert(RegisteredType::get_size_of(_id) <= MAX_SIZE); an_assert(RegisteredType::is_plain_data(_id)); memory_copy(_raw, valueRaw, RegisteredType::get_size_of(_id)); }

		bool is_set() const { return type != RegisteredType::none(); }
		TypeId get_type() const { return type; }

	private:
		static int const MAX_SIZE = 64;
		int8 valueRaw[MAX_SIZE];
		TypeId type = RegisteredType::none();
	};

	struct ToolSet
	: public RefCountObject
	{
	public:
		ToolSet(int _limit = 0) : limit(_limit) {}
		virtual ~ToolSet();

		bool load_from_xml(IO::XML::Node const * _node);

		template <typename Class>
		bool update(Class & _value, Context & _context) const;

		bool update(REF_ Value & _value, Context & _context) const;
		bool update(Context & _context) const; // if return value is not important

		bool is_empty() const { return tools.is_empty(); }
		Array<ITool*> const & get_tools() const { return tools; }
		Array<ITool*> & access_tools() { return tools; }

		void clear();

	private:
		int limit;
		Array<ITool*> tools;

		friend class RegisteredCustomTool;
	};

	interface_class ITool
	{
	public:
		virtual ~ITool() {}

		static ITool * create_from_xml(IO::XML::Node const * _node);

		String const & get_source_location() const { return sourceLocation; }

	public:
		virtual bool load_from_xml(IO::XML::Node const * _node);
		virtual bool load_prefixed_from_xml(IO::XML::Node const * _node, String const& _prefix);
		virtual bool update(REF_ Value & _value, Context & _context) const = 0;

		virtual tchar const* development_description() const { return TXT(""); }

	protected:
		template <typename Class>
		static bool process_value(OUT_ Class & _value, Class const & _defaultValue, ToolSet const & _toolSet, Context & _context, tchar const * const _errorOnValueType);

	private:
		String sourceLocation;
	};

	interface_class IPrefixedTool
	: public ITool
	{
	public:
		virtual ~IPrefixedTool() {}
	};

	/**
	 *	Custom tools are just snippets of code - they should use temp variables.
	 *	They use stack by default.
	 *	Input parameters are read into stack from any store.
	 *	Output parameters are read from stack (before popping) into any store.
	 *	"Current value" can be used for both input and output (processing function).
	 *	Registered custom tools are used as base for actual CustomTool that runs them.
	 */
	class RegisteredCustomTool
	{
	public:
		Name name;
		struct Parameter
		{
			Name name;
			TypeId type = RegisteredType::none();
			bool load_from_xml(IO::XML::Node const * _node);
		};
		Array<Parameter> inputParameters;
		Array<Parameter> outputParameters;
		ToolSet tools;

		void reset() { inputParameters.clear(); outputParameters.clear(); tools.clear(); }
		bool load_from_xml(IO::XML::Node const * _node);
	};

	class RegisteredTools
	: public FactoryOfNamed<ITool, String>
	{
	public:
		typedef FactoryOfNamed<ITool, String> base;
	public:
		static void initialise_static();
		static void close_static();

		static ITool* create(String const & _name);
		static RegisteredCustomTool const * find_custom(Name const & _name);

		static bool register_custom_from_xml(IO::XML::Node const * _node);

		static void development_output_all(LogInfoContext& _log);

	private:
		typedef Array<RegisteredCustomTool*> RegisteredCustomToolsArray;
		static Concurrency::MRSWLock registeredCustomToolsLock;
		static RegisteredCustomToolsArray* registeredCustomTools;
	};

	class RegisteredPrefixedTools
	: public FactoryOfNamed<IPrefixedTool, String>
	{
		typedef FactoryOfNamed<IPrefixedTool, String> base;
	};

	#include "iWMPTool.inl"
};
