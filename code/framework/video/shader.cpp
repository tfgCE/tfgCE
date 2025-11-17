#include "shader.h"

#include "..\..\core\types\names.h"
#include "..\..\core\concurrency\asynchronousJob.h"
#include "..\..\core\containers\arrayStack.h"

#include "..\library\library.h"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#include "..\pipelines\postProcessPipeline.h"
#include "..\pipelines\renderingPipeline.h"
#include "..\pipelines\uiPipeline.h"

#include "..\..\core\system\video\primitivesPipeline.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"

#include "shaderFunctionLib.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

REGISTER_FOR_FAST_CAST(Shader);
LIBRARY_STORED_DEFINE_TYPE(Shader, shader);

Shader::Shader(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, useRenderingPipeline(false)
, useWithRenderingPipeline(false)
, usePrimitivesPipeline(false)
, useWithPrimitivesPipeline(false)
, usePostProcessPipeline(false)
, useWithPostProcessPipeline(false)
, useUIPipeline(false)
, useWithUIPipeline(false)
, loadData(nullptr)
{
}

bool Shader::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	if (!loadData)
	{
		loadData = new LoadData();
	}

	if (IO::XML::Attribute const * attr = _node->get_attribute(TXT("file")))
	{
		loadData->shaderSetup = ::System::ShaderSetup::from_file(get_shader_type(), _lc.get_path_in_dir(attr->get_value()));
	}
	else if (IO::XML::Node const * source = _node->first_child_named(TXT("rawSource")))
	{
		loadData->shaderSetup = ::System::ShaderSetup::from_string(get_shader_type(), source->get_text());
	}
	else if (IO::XML::Node const * source = _node->first_child_named(TXT("source")))
	{
		loadData->shaderSetup = ::System::ShaderSetup::will_use_source(get_shader_type());
		loadData->shaderSetup->shaderSource.load_from_xml(source);
	}
	loadData->shaderSetup->fill_version(RenderingPipeline::get_version());
	
	if (IO::XML::Node const * dependsOnNode = _node->first_child_named(TXT("dependsOn")))
	{
		for_every(shaderFunctionLibNode, dependsOnNode->children_named(TXT("shaderFunctionLib")))
		{
			LibraryName shaderFunctionLib;
			if (shaderFunctionLib.load_from_xml(shaderFunctionLibNode, TXT("name"), _lc))
			{
				dependsOnShaderFunctionLibs.push_back(shaderFunctionLib);
			}
			else
			{
				error_loading_xml(_node, TXT("problem loading shader lib reference"));
			}
		}
	}

	extendsShaderName.load_from_xml(_node, TXT("extends"), _lc);

	useRenderingPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("useRenderingPipeline"), useRenderingPipeline);
	useWithRenderingPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("useWithRenderingPipeline"), useWithRenderingPipeline);
	usePrimitivesPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("usePrimitivesPipeline"), usePrimitivesPipeline);
	useWithPrimitivesPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("useWithPrimitivesPipeline"), useWithPrimitivesPipeline);
	usePostProcessPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("usePostProcessPipeline"), usePostProcessPipeline);
	useWithPostProcessPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("useWithPostProcessPipeline"), useWithPostProcessPipeline);
	useUIPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("useUIPipeline"), useUIPipeline);
	useWithUIPipeline = _node->get_bool_attribute_or_from_child_presence(TXT("useWithUIPipeline"), useWithUIPipeline);
	if ((is_to_be_used_with_rendering()? 1 : 0) +
		(is_to_be_used_with_primitives()? 1 : 0) +
		(is_to_be_used_with_post_process()? 1 : 0) +
		(is_to_be_used_with_ui() ? 1 : 0) > 1)
	{
		error_loading_xml(_node, TXT("can't have shader that will be used with more than one pipeline"));
		result = false;
	}
	if (useRenderingPipeline && useWithRenderingPipeline)
	{
		error_loading_xml(_node, TXT("can't have shader that will use rendering pipeline and will be used with rendering pipeline - decide on one"));
		result = false;
	}
	if (usePrimitivesPipeline && useWithPrimitivesPipeline)
	{
		error_loading_xml(_node, TXT("can't have shader that will use primitives pipeline and will be used with primitives pipeline - decide on one"));
		result = false;
	}
	if (usePostProcessPipeline && useWithPostProcessPipeline)
	{
		error_loading_xml(_node, TXT("can't have shader that will use post process pipeline and will be used with post process pipeline - decide on one"));
		result = false;
	}
	if (useUIPipeline && useWithUIPipeline)
	{
		error_loading_xml(_node, TXT("can't have shader that will use ui pipeline and will be used with ui pipeline - decide on one"));
		result = false;
	}

	if (get_shader_type() == ::System::ShaderType::Fragment)
	{
		// add default outputs for each pipeline (only for fragment shaders)
		if (useRenderingPipeline || useWithRenderingPipeline)
		{
			loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(RenderingPipeline::get_default_output_definition());
		}
		if (usePrimitivesPipeline || useWithPrimitivesPipeline)
		{
			loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(::System::PrimitivesPipeline::get_default_output_definition());
		}
		if (usePostProcessPipeline && !useWithPostProcessPipeline)
		{
			loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(PostProcessPipeline::get_default_output_definition());
		}
		if (useUIPipeline && !useWithUIPipeline)
		{
			loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(UIPipeline::get_default_output_definition());
		}
	}

	::System::OutputTextureDefinition defaultOutputTextureDefinition;
	if (is_to_be_used_with_rendering())
	{
		if (RenderingPipeline::get_default_output_definition().get_output_texture_count() > 0)
		{
			defaultOutputTextureDefinition = RenderingPipeline::get_default_output_definition().get_output_texture_definition(0);
		}
	}
	if (is_to_be_used_with_primitives())
	{
		if (::System::PrimitivesPipeline::get_default_output_definition().get_output_texture_count() > 0)
		{
			defaultOutputTextureDefinition = ::System::PrimitivesPipeline::get_default_output_definition().get_output_texture_definition(0);
		}
	}
	if (is_to_be_used_with_post_process())
	{
		if (PostProcessPipeline::get_default_output_definition().get_output_texture_count() > 0)
		{
			defaultOutputTextureDefinition = PostProcessPipeline::get_default_output_definition().get_output_texture_definition(0);
		}
	}
	if (is_to_be_used_with_ui())
	{
		if (UIPipeline::get_default_output_definition().get_output_texture_count() > 0)
		{
			defaultOutputTextureDefinition = UIPipeline::get_default_output_definition().get_output_texture_definition(0);
		}
	}

	// load additional setup
	result &= loadData->shaderSetup->load_from_xml(_node, defaultOutputTextureDefinition);

	return result;
}

struct LoadShaderData
: public Concurrency::AsynchronousJobData
{
	Shader* shader;
	Library* library;
	LoadShaderData(Shader * _shader, Library * _library)
	: shader(_shader)
	, library(_library)
	{}
};

bool Shader::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		result = LibraryStored::prepare_for_game(_library, _pfgContext);
		_pfgContext.request_level(LibraryPrepareLevel::LoadShader);
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadShader)
	{
		if (loadData)
		{
			shader = new ::System::Shader(*loadData->shaderSetup.get());
			delete_and_clear(loadData);
		}
	}

	return result;
}

struct Shader_Release
: public Concurrency::AsynchronousJobData
{
	RefCountObjectPtr<::System::Shader> shader;
	Shader_Release(::System::Shader* _shader)
	: shader(_shader)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Shader_Release* data = (Shader_Release*)_data;
		data->shader = nullptr;
	}
};

Shader::~Shader()
{
	if (shader.get())
	{
		Game::async_system_job(get_library()->get_game(), Shader_Release::perform, new Shader_Release(shader.get()));
	}
	delete_and_clear(loadData);
}

// will add in order (deepest depend will be first)
static void add_depends_on_shader_function_lib(REF_ Array<LibraryName> & _shaderFunctionLibs, Array<LibraryName> const & _toAdd,  Library* _library)
{
	ARRAY_STACK(LibraryName, justAdded, _toAdd.get_size());
	for_every_reverse(toAdd, _toAdd)
	{
		if (!_shaderFunctionLibs.does_contain(*toAdd))
		{
			_shaderFunctionLibs.insert_at(0, *toAdd);
			justAdded.push_back(*toAdd);
		}
	}
	if (! justAdded.is_empty())
	{
		for_every(added, justAdded)
		{
			if (ShaderFunctionLib* shaderFunctionLib = _library->get_shader_function_libs().find(*added))
			{
				add_depends_on_shader_function_lib(REF_ _shaderFunctionLibs, shaderFunctionLib->get_depends_on_shader_libs(), _library);
			}
			else
			{
				error(TXT("could not find shader function lib \"%S\""), added->to_string().to_char());
			}
		}
	}
}

bool Shader::fill_source_with_base(Library* _library)
{
	if (! loadData || loadData->sourceFilledWithBase)
	{
		return true;
	}

	bool result = true;

	// try to add in order from most depended (deepest)
	Array<LibraryName> dependsOnShaderFunctionLibsAll;
	add_depends_on_shader_function_lib(REF_ dependsOnShaderFunctionLibsAll, dependsOnShaderFunctionLibs, _library);
	for_every(dependOnShaderFunctionLib, dependsOnShaderFunctionLibsAll)
	{
		if (ShaderFunctionLib* shaderFunctionLib = _library->get_shader_function_libs().find(*dependOnShaderFunctionLib))
		{
			result &= loadData->shaderSetup->fill_source_with_base(shaderFunctionLib->get_shader_setup().shaderSource);
		}
		else
		{
			error(TXT("could not find shader function lib \"%S\""), dependOnShaderFunctionLib->to_string().to_char());
			result = false;
		}
	}

	Shader* toBeExtended = find_one_to_extend(_library);
	if (toBeExtended)
	{
		// mark to avoid circles
		todo_future(TXT("...but maybe we should catch circles?"));
		loadData->sourceFilledWithBase = true;

		result &= toBeExtended->fill_source_with_base(_library);

		// extend actual source, we do it here, because we should start with most important and then fill things that are missing with
		if (toBeExtended->loadData)
		{
			result &= loadData->shaderSetup->fill_source_with_base(toBeExtended->loadData->shaderSetup->shaderSource);
		}
	}

	return result;
}

Shader* Shader::find_one_to_extend(Library* _library)
{
	an_assert(false, TXT("implement_"));
	return nullptr;
}

//

REGISTER_FOR_FAST_CAST(VertexShader);
LIBRARY_STORED_DEFINE_TYPE(VertexShader, vertexShader);

VertexShader::VertexShader(Library * _library, LibraryName const & _name)
: Shader(_library, _name)
{
}

bool VertexShader::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	if (_node->get_bool_attribute_or_from_child_presence(TXT("useWithStatic"), false))
	{
		shaderUsage = ::System::MaterialShaderUsage::Static;
		todo_future(TXT("allow SkinnedToSingleBone too"));
	}
	else if (_node->get_bool_attribute_or_from_child_presence(TXT("useWithSkinned"), false))
	{
		shaderUsage = ::System::MaterialShaderUsage::Skinned;
	}
	else if (_node->get_bool_attribute_or_from_child_presence(TXT("useWithSkinnedToSingleBone"), false))
	{
		shaderUsage = ::System::MaterialShaderUsage::SkinnedToSingleBone;
	}
	else if (useRenderingPipeline || useWithRenderingPipeline ||
		usePrimitivesPipeline || useWithPrimitivesPipeline ||
		usePostProcessPipeline || useWithPostProcessPipeline ||
		useUIPipeline || useWithUIPipeline)
	{
		error_loading_xml(_node, TXT("vertex shader should define whether to be used with static or skinned when using pipelines"));
		result = false;
	}

	loadData->shaderUsage = shaderUsage;

	return result;
}

bool VertexShader::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (!fill_source_with_base(_library))
		{
			result = false;
		}
	}

	return result && base::prepare_for_game(_library, _pfgContext);
}

bool VertexShader::fill_source_with_base(Library* _library)
{
	bool result = true;
	result = base::fill_source_with_base(_library);
	if (!loadData || loadData->sourceFilledWithBase)
	{
		// we could already find base and fill source - or maybe we did that out of order
		return result;
	}
	// mark to avoid circles
	todo_future(TXT("...but maybe we should catch circles?"));
	loadData->sourceFilledWithBase = true;
	if (useRenderingPipeline || useWithRenderingPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(RenderingPipeline::get_vertex_shader_source(loadData->shaderUsage));
	}
	else if (usePrimitivesPipeline || useWithPrimitivesPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(::System::PrimitivesPipeline::get_vertex_shader_for_3D_source());
	}
	else if (usePostProcessPipeline || useWithPostProcessPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(PostProcessPipeline::get_vertex_shader_source());
	}
	else if (useUIPipeline || useWithUIPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(UIPipeline::get_vertex_shader_source());
	}
	return result;
}

Shader* VertexShader::find_one_to_extend(Library* _library)
{
	if (extendsShaderName.is_valid())
	{
		Shader* found = _library->get_vertex_shaders().find(extendsShaderName);
		if (found)
		{
			return found;
		}
		error(TXT("could not find vertex shader \"%S\""), extendsShaderName.to_string().to_char());
	}
	return nullptr;
}

void VertexShader::add_defaults(Library* _library)
{
	if (!_library->get_vertex_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultUI.to_string())))
	{
		VertexShader* shader = _library->get_vertex_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultUI.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = UIPipeline::get_default_vertex_shader_source();
		shader->useUIPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(UIPipeline::get_version());
		shader->loadData->shaderUsage = ::System::MaterialShaderUsage::Static;
	}
	if (!_library->get_vertex_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultPostProcess.to_string())))
	{
		VertexShader* shader = _library->get_vertex_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultPostProcess.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = PostProcessPipeline::get_default_vertex_shader_source();
		shader->usePostProcessPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(PostProcessPipeline::get_version());
		shader->loadData->shaderUsage = ::System::MaterialShaderUsage::Static;
	}
	if (!_library->get_vertex_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultPrimitives.to_string())))
	{
		VertexShader* shader = _library->get_vertex_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultPrimitives.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = ::System::PrimitivesPipeline::get_default_vertex_shader_source();
		shader->usePrimitivesPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(PostProcessPipeline::get_version());
		shader->loadData->shaderUsage = ::System::MaterialShaderUsage::Static;
	}
	if (!_library->get_vertex_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultRenderingStatic.to_string())))
	{
		VertexShader* shader = _library->get_vertex_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultRenderingStatic.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = RenderingPipeline::get_default_vertex_shader_source(::System::MaterialShaderUsage::Static);
		shader->useRenderingPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource; 
		shader->loadData->shaderSetup->fill_version(RenderingPipeline::get_version());
		shader->loadData->shaderUsage = ::System::MaterialShaderUsage::Static;
	}
	if (!_library->get_vertex_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultRenderingSkinned.to_string())))
	{
		VertexShader* shader = _library->get_vertex_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultRenderingSkinned.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = RenderingPipeline::get_default_vertex_shader_source(::System::MaterialShaderUsage::Skinned);
		shader->useRenderingPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource; 
		shader->loadData->shaderSetup->fill_version(RenderingPipeline::get_version());
		shader->loadData->shaderUsage = ::System::MaterialShaderUsage::Skinned;
	}
	if (!_library->get_vertex_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultRenderingSkinnedToSingleBone.to_string())))
	{
		VertexShader* shader = _library->get_vertex_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultRenderingSkinnedToSingleBone.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = RenderingPipeline::get_default_vertex_shader_source(::System::MaterialShaderUsage::SkinnedToSingleBone);
		shader->useRenderingPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(RenderingPipeline::get_version());
		shader->loadData->shaderUsage = ::System::MaterialShaderUsage::SkinnedToSingleBone;
	}
}

VertexShader * VertexShader::get_default_post_process(Library* _library)
{
	return _library->get_vertex_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultPostProcess.to_string()));
}

VertexShader * VertexShader::get_default_primitives(Library* _library)
{
	return _library->get_vertex_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultPrimitives.to_string()));
}

VertexShader * VertexShader::get_default_rendering_static(Library* _library)
{
	return _library->get_vertex_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultRenderingStatic.to_string()));
}

VertexShader * VertexShader::get_default_rendering_skinned(Library* _library)
{
	return _library->get_vertex_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultRenderingSkinned.to_string()));
}

VertexShader * VertexShader::get_default_rendering_skinned_to_one_bone(Library* _library)
{
	return _library->get_vertex_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultRenderingSkinnedToSingleBone.to_string()));
}

VertexShader * VertexShader::get_default_ui(Library* _library)
{
	return _library->get_vertex_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultUI.to_string()));
}

//

REGISTER_FOR_FAST_CAST(FragmentShader);
LIBRARY_STORED_DEFINE_TYPE(FragmentShader, fragmentShader);

FragmentShader::FragmentShader(Library * _library, LibraryName const & _name)
: Shader(_library, _name)
{
}

bool FragmentShader::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		if (!fill_source_with_base(_library))
		{
			result = false;
		}
	}

	return result && base::prepare_for_game(_library, _pfgContext);
}

bool FragmentShader::fill_source_with_base(Library* _library)
{
	bool result = base::fill_source_with_base(_library);
	if (!loadData || loadData->sourceFilledWithBase)
	{
		// we could already find base and fill source - or maybe we did that out of order
		return result;
	}
	// mark to avoid circles
	todo_future(TXT("...but maybe we should catch circles?"));
	loadData->sourceFilledWithBase = true;
	if (useRenderingPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(RenderingPipeline::get_fragment_shader_source());
	}
	if (!useRenderingPipeline && useWithRenderingPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(RenderingPipeline::get_use_with_rendering_fragment_shader_source());
	}
	if (usePrimitivesPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(::System::PrimitivesPipeline::get_fragment_shader_without_texture_source());
	}
	if (!usePrimitivesPipeline && useWithPrimitivesPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(::System::PrimitivesPipeline::get_use_with_primitives_fragment_shader_source());
	}
	if (usePostProcessPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(PostProcessPipeline::get_fragment_shader_source());
	}
	if (!usePostProcessPipeline && useWithPostProcessPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(PostProcessPipeline::get_use_with_post_process_fragment_shader_source());
	}
	if (useUIPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(UIPipeline::get_fragment_shader_source());
	}
	if (!useUIPipeline && useWithUIPipeline)
	{
		result &= loadData->shaderSetup->fill_source_with_base(UIPipeline::get_use_with_ui_fragment_shader_source());
	}
	return result;
}

Shader* FragmentShader::find_one_to_extend(Library* _library)
{
	if (extendsShaderName.is_valid())
	{
		Shader* found = _library->get_fragment_shaders().find(extendsShaderName);
		if (found)
		{
			return found;
		}
		error(TXT("could not find fragment shader \"%S\""), extendsShaderName.to_string().to_char());
	}
	return nullptr;
}

void FragmentShader::add_defaults(Library* _library)
{
	if (!_library->get_fragment_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultUI.to_string())))
	{
		FragmentShader* shader = _library->get_fragment_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultUI.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = UIPipeline::get_default_fragment_shader_source();
		shader->useUIPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(UIPipeline::get_version());
		shader->loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(UIPipeline::get_default_output_definition());
	}
	if (!_library->get_fragment_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultPostProcess.to_string())))
	{
		FragmentShader* shader = _library->get_fragment_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultPostProcess.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = PostProcessPipeline::get_default_fragment_shader_source();
		shader->usePostProcessPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(PostProcessPipeline::get_version());
		shader->loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(PostProcessPipeline::get_default_output_definition());
	}
	if (!_library->get_fragment_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultRendering.to_string())))
	{
		FragmentShader* shader = _library->get_fragment_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultRendering.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = RenderingPipeline::get_default_fragment_shader_source();
		shader->useRenderingPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(RenderingPipeline::get_version());
		shader->loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(RenderingPipeline::get_default_output_definition());
	}
	if (!_library->get_fragment_shaders().find_may_fail(LibraryName(Names::default_.to_string(), Names::defaultPrimitives.to_string())))
	{
		FragmentShader* shader = _library->get_fragment_shaders().find_or_create(LibraryName(Names::default_.to_string(), Names::defaultPrimitives.to_string()));

		an_assert(shader->get_shader() == nullptr);

		::System::ShaderSource const & shaderSource = ::System::PrimitivesPipeline::get_fragment_shader_without_texture_source();
		shader->usePrimitivesPipeline = true; // process instead of main

		if (!shader->loadData)
		{
			shader->loadData = new LoadData();
		}
		shader->loadData->shaderSetup = ::System::ShaderSetup::will_use_source(shader->get_shader_type());
		shader->loadData->shaderSetup->shaderSource = shaderSource;
		shader->loadData->shaderSetup->fill_version(::System::PrimitivesPipeline::get_version());
		shader->loadData->shaderSetup->get_fragment_output_setup()->copy_output_textures_from(::System::PrimitivesPipeline::get_default_output_definition());
	}
}

FragmentShader * FragmentShader::get_default_rendering(Library* _library)
{
	return _library->get_fragment_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultRendering.to_string()));
}

FragmentShader * FragmentShader::get_default_primitives(Library* _library)
{
	return _library->get_fragment_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultPrimitives.to_string()));
}

FragmentShader * FragmentShader::get_default_post_process(Library* _library)
{
	return _library->get_fragment_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultPostProcess.to_string()));
}

FragmentShader * FragmentShader::get_default_ui(Library* _library)
{
	return _library->get_fragment_shaders().find(LibraryName(Names::default_.to_string(), Names::defaultUI.to_string()));
}
