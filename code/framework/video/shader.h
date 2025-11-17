#pragma once

#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\material.h"

#include "..\library\libraryStored.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace Framework
{
	class Shader
	: public LibraryStored
	{
		FAST_CAST_DECLARE(Shader);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		Shader(Library * _library, LibraryName const & _name);

		::System::Shader * get_shader() const { return shader.get(); }

		bool is_to_be_used_with_rendering() const { return useRenderingPipeline || useWithRenderingPipeline; }
		bool is_to_be_used_with_primitives() const { return usePrimitivesPipeline || useWithPrimitivesPipeline; }
		bool is_to_be_used_with_post_process() const { return usePostProcessPipeline || useWithPostProcessPipeline; }
		bool is_to_be_used_with_ui() const { return useUIPipeline || useWithUIPipeline; }

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		Array<LibraryName> dependsOnShaderFunctionLibs; // has to include shader function libs
		LibraryName extendsShaderName; // to allow extending shaders
		bool useRenderingPipeline; // full rendering pipeline
		bool useWithRenderingPipeline; // just use with rendering pipeline but define everything on your own! you don't have to define your own outputs, use provided
		bool usePrimitivesPipeline; // full primitives pipeline
		bool useWithPrimitivesPipeline; // just use with primitives pipeline but define everything on your own! you don't have to define your own outputs, use provided
		bool usePostProcessPipeline; // full post process pipeline
		bool useWithPostProcessPipeline; // just use with post process pipeline but define everything on your own! you have to define your own outputs!
		bool useUIPipeline; // full ui pipeline
		bool useWithUIPipeline; // just use with ui pipeline but define everything on your own! you have to define your own outputs!

		virtual ~Shader();

		virtual ::System::ShaderType::Type get_shader_type() const = 0;

	private:
		RefCountObjectPtr<::System::Shader> shader;

	protected:
		struct LoadData
		{
			bool sourceFilledWithBase;
			RefCountObjectPtr<::System::ShaderSetup> shaderSetup;
			::System::MaterialShaderUsage::Type shaderUsage; // only for vertex, important if used with pipelines

			LoadData() : sourceFilledWithBase(false), shaderSetup(new ::System::ShaderSetup()) {}
		};
		LoadData* loadData;

		virtual bool fill_source_with_base(Library* _library);
		virtual Shader* find_one_to_extend(Library* _library);
	};

	class VertexShader
	: public Shader
	{
		FAST_CAST_DECLARE(VertexShader);
		FAST_CAST_BASE(Shader);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Shader base;
	public:
		VertexShader(Library * _library, LibraryName const & _name);

		::System::MaterialShaderUsage::Type get_shader_usage() const { return shaderUsage; }

		static void add_defaults(Library* _library);
		static VertexShader * get_default_post_process(Library* _library); // shader for post process
		static VertexShader * get_default_primitives(Library* _library); // shader for primitives
		static VertexShader * get_default_rendering_static(Library* _library); // shader for static objects
		static VertexShader * get_default_rendering_skinned(Library* _library); // shader for skinned objects
		static VertexShader * get_default_rendering_skinned_to_one_bone(Library* _library); // shader for skinned-to-single-bone objects
		static VertexShader * get_default_ui(Library* _library); // shader for ui

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		::System::ShaderType::Type get_shader_type() const { return ::System::ShaderType::Vertex; }

		virtual bool fill_source_with_base(Library* _library);
		virtual Shader* find_one_to_extend(Library* _library);

	private:
		::System::MaterialShaderUsage::Type shaderUsage; // only for vertex, important if used with pipelines
	};

	class FragmentShader
	: public Shader
	{
		FAST_CAST_DECLARE(FragmentShader);
		FAST_CAST_BASE(Shader);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Shader base;
	public:
		FragmentShader(Library * _library, LibraryName const & _name);

		static void add_defaults(Library* _library);
		static FragmentShader * get_default_post_process(Library* _library);
		static FragmentShader * get_default_primitives(Library* _library);
		static FragmentShader * get_default_rendering(Library* _library);
		static FragmentShader * get_default_ui(Library* _library);

	public: // LibraryStored
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);

	protected:
		::System::ShaderType::Type get_shader_type() const { return ::System::ShaderType::Fragment; }

		virtual bool fill_source_with_base(Library* _library);
		virtual Shader* find_one_to_extend(Library* _library);
	};
};
