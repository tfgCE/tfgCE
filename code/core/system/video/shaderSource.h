#pragma once

#include "videoEnums.h"

#include "..\..\containers\list.h"
#include "..\..\other\simpleVariableStorage.h"
#include "..\..\types\name.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace System
{
	struct ShaderSourceFunction
	{
		Name name;
		String header; // optional for overriding
		Array<String> body; // stack to allow calling base

		bool load_from_xml(IO::XML::Node const * _node);

		String get_body() const;
	};

	struct ShaderSourceVersion
	{
		int gl = 0;
		int gles = 0;

		void clear();
		void setup_default();
		bool load_from_xml_child_node(IO::XML::Node const* _node, tchar const* _childName = TXT("version"));
	};

	struct ShaderSourceCustomisation
	{
	public:
		static ShaderSourceCustomisation* get() { an_assert(s_one); return s_one; }
		static void initialise_static();
		static void close_static();

		Array<String> macros;
		Array<String> functions; // built-in
		Array<String> data; // built-in
		Array<String> functionsVertexShaderOnly; // built-in
		Array<String> dataVertexShaderOnly; // built-in
		Array<String> functionsFragmentShaderOnly; // built-in
		Array<String> dataFragmentShaderOnly; // built-in
	private:
		static ShaderSourceCustomisation* s_one;
	};

	struct ShaderSource
	{
		ShaderSourceVersion version;
		String data;
		List<ShaderSourceFunction> functions;
		String mainBody;
		SimpleVariableStorage defaultValues; // if there is a default value, it is also a "forced" uniform (if not actual uniform)

		bool load_from_xml(IO::XML::Node const * _node);

		void clear();

		String get_source(ShaderType::Type _type) const;

		inline ShaderSourceFunction& get_function(Name const & _name) { return get_function(_name, false); }
		void add_function(Name const & _name, String const & _header, String const & _body = String::empty());
		
		bool override_with(ShaderSource const & _extender);
	private:
		ShaderSourceFunction& get_function(Name const & _name, bool _addInFrontIfNotExisting);
		ShaderSourceFunction* get_existing_function(Name const & _name);
	};
};

