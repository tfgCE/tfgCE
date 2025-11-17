#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"
#include "..\..\containers\map.h"
#include "..\..\memory\refCountObject.h"

#include "video.h"

#include "shaderSource.h"

#include "..\..\memory\refCountObject.h"

namespace IO
{
	namespace XML
	{
		class Node;
	};
};

namespace System
{
	typedef GLuint ShaderID;

	struct FragmentShaderOutputSetup;
	struct OutputTextureDefinition;

	struct ShaderSetup
	: public RefCountObject
	{
		ShaderType::Type type;
		String shaderFile;
		String shaderString;

		ShaderSource shaderSource;

		FragmentShaderOutputSetup* fragmentOutputSetup = nullptr;

		static ShaderSetup* from_file(ShaderType::Type _type, String const & _shaderFile) { ShaderSetup *ss = new ShaderSetup(); ss->type = _type; ss->shaderFile = _shaderFile; return ss; }
		static ShaderSetup* from_string(ShaderType::Type _type, String const & _shaderString) { ShaderSetup* ss = new ShaderSetup(); ss->type = _type; ss->shaderString = _shaderString; return ss; }
		static ShaderSetup* from_source(ShaderType::Type _type, ShaderSource const & _shaderSource) { ShaderSetup* ss = new ShaderSetup(); ss->type = _type; ss->shaderSource = _shaderSource; return ss; }
		static ShaderSetup* will_use_source(ShaderType::Type _type) { ShaderSetup* ss = new ShaderSetup(); ss->type = _type; return ss; }

		ShaderSetup();
		~ShaderSetup();

		FragmentShaderOutputSetup* get_fragment_output_setup();

		bool load_from_xml(IO::XML::Node const * _node, OutputTextureDefinition const & _defaultOutputTextureDefinition);

		void fill_version(ShaderSourceVersion const& _version);

		bool fill_source_with_base(ShaderSource const & _base);
	};

	/*
	 *	Will auto delete on Video3d closing.
	 */
	class Shader
	: public RefCountObject
	{
	public:
		Shader(ShaderSetup const * _setup);
		Shader(ShaderSetup const & _setup);

		void init(ShaderSetup const & _setup);
		void close();

		String const& get_source();
		void make_sure_shader_is_compiled();
		ShaderID get_shader_id() const { an_assert(isCompiled); return shader; }

		FragmentShaderOutputSetup & access_fragment_shader_output_setup() { an_assert(type == ShaderType::Fragment); return *fragmentOutputSetup; }
		FragmentShaderOutputSetup const & get_fragment_shader_output_setup() const { an_assert(type == ShaderType::Fragment); return *fragmentOutputSetup; }

		SimpleVariableStorage const& get_default_values() const { return defaultValues; }

	protected: friend class Video3D;
		virtual ~Shader();

	private:
		ShaderType::Type type;
		RefCountObjectPtr<ShaderSetup> setup;
		bool isSourceAssembled = false;
		bool isCompiled = false;
		String source; 
		SimpleVariableStorage defaultValues;
		ShaderID shader;

		FragmentShaderOutputSetup* fragmentOutputSetup = nullptr;

		void assemble_source();
	};

};

 