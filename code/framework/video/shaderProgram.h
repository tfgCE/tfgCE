#pragma once

#include "..\..\core\system\video\material.h"
#include "..\..\core\system\video\materialInstance.h"
#include "..\..\core\mesh\mesh3dInstance.h"

#include "..\..\core\system\video\video3d.h"

#include "..\library\libraryStored.h"

#include "..\debugSettings.h"

namespace Concurrency
{
	class JobExecutionContext;
};

namespace System
{
	class ShaderProgram;
};

namespace Framework
{
	class FragmentShader;
	class VertexShader;

	/**
	 *	Single shader parameter.
	 *	Wraps system's ShaderParam and adds framework::textures.
	 */
	struct ShaderParam
	{
		Name name;
		::System::ShaderParam param;
		LibraryName textureName;

		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library);

		bool operator==(ShaderParam const & _other) const { return name == _other.name && param == _other.param && textureName == _other.textureName; }
	};

	/**
	 *	All shader params as used by setup.
	 */
	struct ShaderParams
	{
#ifdef AN_ASSERT
		bool preparedForGame;
#endif
		Array<ShaderParam> params;

		ShaderParams();

		bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc);
		bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		bool prepare_for_game(Library* _library);

		void clear();

		ShaderParam const* get(Name const& _name) const;

		void apply_to(::System::ShaderProgram* _shaderProgram) const;
		void set_to_default_values_of(::System::ShaderProgram* _shaderProgram) const;
		bool operator==(ShaderParams const & _other) const;
		bool operator!=(ShaderParams const & _other) const { return !(operator==(_other)); }
	};

	class ShaderProgram
	: public LibraryStored
	{
		FAST_CAST_DECLARE(ShaderProgram);
		FAST_CAST_BASE(LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef LibraryStored base;
	public:
		ShaderProgram(Library * _library, LibraryName const & _name);

		override_ bool load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc);
		override_ bool prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext);
		override_ void prepare_to_unload();

		::System::ShaderProgram * get_shader_program() const { return shaderProgram.get(); }

	protected:
		virtual ~ShaderProgram();

	private:
		UsedLibraryStored<FragmentShader> fragmentShader;
		UsedLibraryStored<VertexShader> vertexShader;
		RefCountObjectPtr<::System::ShaderProgram> shaderProgram;
		bool postProcessShader;
		bool primitivesShader;
		bool uiShader;

		ShaderParams params;

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		float loadShaderTime = 0.0f;
		float loadShaderDetailedTime[4];
#endif

		static void load_shader_program(Concurrency::JobExecutionContext const * _executionContext, void* _data);
		static void shader_program__setup_attribs(Concurrency::JobExecutionContext const* _executionContext, void* _data);
		static void shader_program__setup_uniforms(Concurrency::JobExecutionContext const* _executionContext, void* _data);
		static void shader_program__finalise(Concurrency::JobExecutionContext const* _executionContext, void* _data);
		static void apply_params_while_loading(Concurrency::JobExecutionContext const * _executionContext, void* _data);
	};
};
