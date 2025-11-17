#include "material.h"

#include "..\..\core\types\names.h"
#include "..\..\core\concurrency\asynchronousJob.h"

#ifdef AN_DEVELOPMENT
#include "..\debug\previewGame.h"
#endif
#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\usedLibraryStored.inl"
#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#ifdef AN_DEVELOPMENT_OR_PROFILER
	#ifdef AN_ALLOW_EXTENSIVE_LOGS
		#ifdef ALLOW_DETAILED_DEBUG
			#define LOG_LOADING_SHADERS
		#endif
	#endif
#endif

#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	#ifndef LOG_LOADING_SHADERS
		#define LOG_LOADING_SHADERS
	#endif
#endif
//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

void MaterialParams::apply_to(::System::Material* _material) const
{
#ifdef AN_ASSERT
	//!@# an_assert(preparedForGame, "maybe some object was not prepared for game or something is missing?");
#endif
	for_every(param, params)
	{
		_material->set_uniform(param->name, param->param);
	}
}

void MaterialParams::apply_to(::System::MaterialInstance* _materialInstance, ::System::MaterialSetting::Type _settingMaterial) const
{
#ifdef AN_ASSERT
	//!@# an_assert(preparedForGame, "maybe some object was not prepared for game or something is missing?");
#endif
	for_every(param, params)
	{
		_materialInstance->set_uniform(param->name, param->param, _settingMaterial);
	}
}

//

MaterialSetup::MaterialSetup()
{
}

void MaterialSetup::resolve_links()
{
	if (auto* lib = Library::get_current())
	{
		material.find_may_fail(lib);
		params.prepare_for_game(lib);
	}
}

bool MaterialSetup::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= material.load_from_xml(_node, TXT("material"), _lc);
	result &= params.load_from_xml(_node, _lc);
	renderingOrderPriorityOffset.load_from_xml(_node, TXT("renderingOrderPriorityOffset"));

	return result;
}

bool MaterialSetup::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext, int _atLevel)
{
	bool result = true;
	if (!material.prepare_for_game_find(_library, _pfgContext, _atLevel))
	{
		error(TXT("couldn't find material \"%S\""), material.get_name().to_string().to_char());
		result = false;
	}
	if (_pfgContext.get_current_level() == _atLevel)
	{
		if (!params.prepare_for_game(_library))
		{
			error(TXT("couldn't prepare params"));
			result = false;
		}
	}
#ifdef AN_DEVELOPMENT
	if (Game::get_as<PreviewGame>())
	{
		result = true;
	}
#endif
	an_assert(result);
	if (_pfgContext.get_current_level() == _atLevel)
	{
		if (!material.get())
		{
			error(TXT("no material \"%S\""), material.get_name().to_string().to_char());
			result = false;
		}
	}
	return result;
}

void MaterialSetup::set_material(Material* _material)
{
	material = _material;
	params.clear();
	if (_material)
	{
		params = _material->get_params();
	}
}

void MaterialSetup::apply_to(Meshes::Mesh3DInstance& _meshInstance, int _materialIndex, ::System::MaterialSetting::Type _settingMaterial) const
{
	if (material.get())
	{
		_meshInstance.set_material(_materialIndex, material->get_material());
		if (::System::MaterialInstance* materialInstance = _meshInstance.get_material_instance(_materialIndex))
		{
			params.apply_to(materialInstance, _settingMaterial);
			materialInstance->set_rendering_order_priority_offset(renderingOrderPriorityOffset);
		}
	}
}

void MaterialSetup::apply_material_setups(Array<MaterialSetup> const & _materialSetups, Meshes::Mesh3DInstance& _meshInstance, ::System::MaterialSetting::Type _settingMaterial)
{
	int materialIdx = 0;
	_meshInstance.requires_at_least_materials(_materialSetups.get_size());
	for_every(materialSetup, _materialSetups)
	{
		materialSetup->apply_to(_meshInstance, materialIdx, _settingMaterial);
		++materialIdx;
	}
}

bool MaterialSetup::apply_renamer(LibraryStoredRenamer const & _renamer, Library * _library)
{
	bool result = true;
	result &= _renamer.apply_to(material, _library);
	return result;
}

//

REGISTER_FOR_FAST_CAST(Material);
LIBRARY_STORED_DEFINE_TYPE(Material, material);

Material::Material(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, useWithStaticMesh(false)
, useWithSkinnedMesh(false)
, useWithSkinnedToSingleBoneMesh(false)
, material(new ::System::Material())
, parentLinked(false)
{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	loadShaderDetailedTime[0] = 0.0f;
	loadShaderDetailedTime[1] = 0.0f;
	loadShaderDetailedTime[2] = 0.0f;
	loadShaderDetailedTime[3] = 0.0f;
#endif
}

struct Material_DeleteMaterial
: public Concurrency::AsynchronousJobData
{
	::System::Material* material;
	Material_DeleteMaterial(::System::Material* _material)
	: material(_material)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		Material_DeleteMaterial* data = (Material_DeleteMaterial*)_data;
		delete data->material;
	}
};

Material::~Material()
{
	Game::async_system_job(get_library()->get_game(), Material_DeleteMaterial::perform, new Material_DeleteMaterial(material));
}

bool Material::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	material->access_tags().set_tags_from(get_tags());

	result &= material->load_from_xml(_node);

	useWithStaticMesh = _node->get_bool_attribute_or_from_child_presence(TXT("useWithStaticMesh"), useWithStaticMesh);
	useWithSkinnedMesh = _node->get_bool_attribute_or_from_child_presence(TXT("useWithSkinnedMesh"), useWithSkinnedMesh);
	useWithSkinnedToSingleBoneMesh = _node->get_bool_attribute_or_from_child_presence(TXT("useWithSkinnedToSingleBoneMesh"), useWithSkinnedToSingleBoneMesh);
	useWithSkinnedToSingleBoneMesh = useWithStaticMesh || useWithSkinnedToSingleBoneMesh;

	result &= vertexShader.load_from_xml(_node, TXT("vertexShader"), _lc);
	result &= vertexShaderStatic.load_from_xml(_node, TXT("vertexShaderStatic"), _lc);
	result &= vertexShaderSkinned.load_from_xml(_node, TXT("vertexShaderSkinned"), _lc);
	result &= vertexShaderSkinnedToSingleBone.load_from_xml(_node, TXT("vertexShaderSkinnedToSingleBone"), _lc);
	result &= fragmentShader.load_from_xml(_node, TXT("fragmentShader"), _lc);
	result &= parentMaterial.load_from_xml(_node, TXT("parent"), _lc);
	if (!(fragmentShader.is_name_valid() || parentMaterial.is_name_valid()))
	{
		error_loading_xml(_node, TXT("either fragmentShader or parentMaterial should be provided"));
		result = false;
	}

	result &= params.load_from_xml_child_node(_node, TXT("params"), _lc);

	result &= physicalMaterialMap.load_from_xml(_node, TXT("physicalMaterialMap"), _lc);

	return result;
}

struct LoadShadersIntoMaterialData
: public Concurrency::AsynchronousJobData
{
	Material* material;
	Library* library;
	Optional<::System::MaterialShaderUsage::Type> usage;
	::System::ShaderProgram* shaderProgram;
	LoadShadersIntoMaterialData(Material * _material, Library* _library, Optional<::System::MaterialShaderUsage::Type> const & _usage = NP, ::System::ShaderProgram* _shaderProgram = nullptr)
	: material(_material)
	, library(_library)
	, usage(_usage)
	, shaderProgram(_shaderProgram)
	{}
};

bool Material::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);
	if (vertexShader.is_name_valid())
	{
		result &= vertexShader.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		if (vertexShader.get())
		{
			useWithStaticMesh = vertexShader.get()->get_shader_usage() == ::System::MaterialShaderUsage::Static;
			useWithSkinnedMesh = vertexShader.get()->get_shader_usage() == ::System::MaterialShaderUsage::Skinned;
			useWithSkinnedToSingleBoneMesh = vertexShader.get()->get_shader_usage() == ::System::MaterialShaderUsage::SkinnedToSingleBone;
		}
	}
	if (vertexShaderStatic.is_name_valid())
	{
		result &= vertexShaderStatic.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		if (vertexShaderStatic.get())
		{
			useWithStaticMesh = vertexShaderStatic.get()->get_shader_usage() == ::System::MaterialShaderUsage::Static;
			if (!useWithStaticMesh)
			{
				error(TXT("invalid shader \"%S\" for static material \"%S\""), vertexShaderStatic->get_name().to_string().to_char(), get_name().to_string().to_char());
			}
		}
	}
	if (vertexShaderSkinned.is_name_valid())
	{
		result &= vertexShaderSkinned.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		if (vertexShaderSkinned.get())
		{
			useWithSkinnedMesh = vertexShaderSkinned.get()->get_shader_usage() == ::System::MaterialShaderUsage::Skinned;
			if (!useWithSkinnedMesh)
			{
				error(TXT("invalid shader \"%S\" for skinned material \"%S\""), vertexShaderSkinned->get_name().to_string().to_char(), get_name().to_string().to_char());
			}
		}
	}
	if (vertexShaderSkinnedToSingleBone.is_name_valid())
	{
		result &= vertexShaderSkinnedToSingleBone.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
		if (vertexShaderSkinnedToSingleBone.get())
		{
			useWithSkinnedToSingleBoneMesh = vertexShaderSkinnedToSingleBone.get()->get_shader_usage() == ::System::MaterialShaderUsage::SkinnedToSingleBone;
			if (!useWithSkinnedToSingleBoneMesh)
			{
				error(TXT("invalid shader \"%S\" for skinned-to-single-bone material \"%S\""), vertexShaderSkinnedToSingleBone->get_name().to_string().to_char(), get_name().to_string().to_char());
			}
		}
	}
	if (fragmentShader.is_name_valid())
	{
		result &= fragmentShader.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (parentMaterial.is_name_valid())
	{
		result &= parentMaterial.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	result &= physicalMaterialMap.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadShaderIntoMaterial)
	{
		if (fragmentShader.get())
		{
			if (!useWithSkinnedMesh && !useWithStaticMesh && !useWithSkinnedToSingleBoneMesh)
			{
				error(TXT("not defined as skinned nor static (vertex shader should provided it on its own)"));
				result = false;
			}
			if (result)
			{
				_library->preparing_asynchronous_job_added();
				Game::async_system_job(get_library()->get_game(), load_shader_into_material, new LoadShadersIntoMaterialData(this, _library));
			}
		}
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LinkMaterialParent)
	{
		if (!parentLinked)
		{
			parentLinked = true;
			if (parentMaterial.is_name_valid())
			{
				if (Material* pm = parentMaterial.get())
				{
					result &= pm->prepare_for_game(_library, _pfgContext);
					material->set_parent(pm->get_material());
					material->allow_individual_instances(pm->material->are_individual_instances_allowed());
				}
				else
				{
					return false;
				}
			}
		}
		// prepare defaults
		result &= params.prepare_for_game(_library);
		// set defaults		
		params.apply_to(material);
	}

	return result;
}

void Material::prepare_to_unload()
{
	base::prepare_to_unload();
	vertexShader.clear();
	vertexShaderStatic.clear();
	vertexShaderSkinned.clear();
	vertexShaderSkinnedToSingleBone.clear();
	fragmentShader.clear();
	parentMaterial.clear();
	physicalMaterialMap.clear();
}

void Material::load_shader_into_material(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	LoadShadersIntoMaterialData * loadData = (LoadShadersIntoMaterialData*)_data;
	Material* self = loadData->material;
	if (!loadData->usage.is_set())
	{
#ifdef LOG_LOADING_SHADERS
		output(TXT("load shaders into material \"%S\""), self->get_name().to_string().to_char());
#endif
		if (self->useWithStaticMesh)
		{
			loadData->library->preparing_asynchronous_job_added();
			Game::async_system_job(loadData->library->get_game(), load_shader_into_material, new LoadShadersIntoMaterialData(self, loadData->library, ::System::MaterialShaderUsage::Static));
		}
		if (self->useWithSkinnedMesh)
		{
			loadData->library->preparing_asynchronous_job_added();
			Game::async_system_job(loadData->library->get_game(), load_shader_into_material, new LoadShadersIntoMaterialData(self, loadData->library, ::System::MaterialShaderUsage::Skinned));
		}
		if (self->useWithSkinnedToSingleBoneMesh)
		{
			loadData->library->preparing_asynchronous_job_added();
			Game::async_system_job(loadData->library->get_game(), load_shader_into_material, new LoadShadersIntoMaterialData(self, loadData->library, ::System::MaterialShaderUsage::SkinnedToSingleBone));
		}
	}
	else
	{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		::System::TimeStamp ts;
		self->loadShaderUsagesIssued.increase();
#endif
		::System::Shader * vertexShader = self->vertexShader.get()? self->vertexShader.get()->get_shader() : nullptr;
		tchar const* suffix = TXT("");
		if (loadData->usage.get() == ::System::MaterialShaderUsage::Static)
		{
#ifdef LOG_LOADING_SHADERS
			output(TXT("  load for static"));
#endif
			suffix = TXT(" /static");
			if (self->vertexShaderStatic.get()) vertexShader = self->vertexShaderStatic.get()->get_shader();
			if (!vertexShader) vertexShader = VertexShader::get_default_rendering_static(loadData->library)->get_shader();
		}
		if (loadData->usage.get() == ::System::MaterialShaderUsage::Skinned)
		{
#ifdef LOG_LOADING_SHADERS
			output(TXT("  load for skinned"));
#endif
			suffix = TXT(" /skinned");
			if (self->vertexShaderSkinned.get()) vertexShader = self->vertexShaderSkinned.get()->get_shader();
			if (!vertexShader) vertexShader = VertexShader::get_default_rendering_skinned(loadData->library)->get_shader();
		}
		if (loadData->usage.get() == ::System::MaterialShaderUsage::SkinnedToSingleBone)
		{
#ifdef LOG_LOADING_SHADERS
			output(TXT("  load for skinned to single bone mesh"));
#endif
			suffix = TXT(" /skinned to single bone");
			if (self->vertexShaderSkinnedToSingleBone.get()) vertexShader = self->vertexShaderSkinnedToSingleBone.get()->get_shader();
			if (!vertexShader) vertexShader = VertexShader::get_default_rendering_skinned_to_one_bone(loadData->library)->get_shader();
		}
		an_assert(vertexShader);
		FragmentShader const* fragmentShader = self->fragmentShader.get();
		if (fragmentShader && fragmentShader->get_shader())
		{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
			::System::TimeStamp tsCreate;
#endif
			::System::ShaderProgramSetup shaderSetup = ::System::ShaderProgramSetup::create(vertexShader, fragmentShader->get_shader(), self->get_name().to_string() + suffix);
			auto* shaderProgram = ::System::ShaderProgram::create_stepped();
			shaderProgram->stepped_creation__create(shaderSetup);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
			self->loadShaderDetailedTime[0] += tsCreate.get_time_since();
#endif
			if (shaderProgram->has_shader_compilation_or_linking_failed())
			{
				error(TXT("error compiling/linking shader for material \"%S\" (for vertex shader%S)"), self->get_name().to_string().to_char(), suffix);
			}
			// break it into separate jobs to avoid stalling
			loadData->library->preparing_asynchronous_job_added();
			loadData->library->preparing_asynchronous_job_added();
			loadData->library->preparing_asynchronous_job_added();
			Game::async_system_job(loadData->library->get_game(), load_shader_into_material__shader_program__setup_attribs, new LoadShadersIntoMaterialData(self, loadData->library, loadData->usage, shaderProgram));
			Game::async_system_job(loadData->library->get_game(), load_shader_into_material__shader_program__setup_uniforms, new LoadShadersIntoMaterialData(self, loadData->library, loadData->usage, shaderProgram));
			Game::async_system_job(loadData->library->get_game(), load_shader_into_material__shader_program__finalise, new LoadShadersIntoMaterialData(self, loadData->library, loadData->usage, shaderProgram));
		}
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		self->loadShaderTime += ts.get_time_since();
#endif
	}
	loadData->library->preparing_asynchronous_job_done();
}

void Material::load_shader_into_material__shader_program__setup_attribs(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	LoadShadersIntoMaterialData* loadData = (LoadShadersIntoMaterialData*)_data;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	::System::TimeStamp ts;
#endif
	loadData->shaderProgram->stepped_creation__load_attribs();
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	float timeTaken = ts.get_time_since();
	loadData->material->loadShaderTime += timeTaken;
	loadData->material->loadShaderDetailedTime[1] += timeTaken;
#endif
	loadData->library->preparing_asynchronous_job_done();
}

void Material::load_shader_into_material__shader_program__setup_uniforms(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	LoadShadersIntoMaterialData* loadData = (LoadShadersIntoMaterialData*)_data;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	::System::TimeStamp ts;
#endif
	loadData->shaderProgram->stepped_creation__load_uniforms();
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	float timeTaken = ts.get_time_since();
	loadData->material->loadShaderTime += timeTaken;
	loadData->material->loadShaderDetailedTime[2] += timeTaken;
#endif
	loadData->library->preparing_asynchronous_job_done();
}

void Material::load_shader_into_material__shader_program__finalise(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	LoadShadersIntoMaterialData* loadData = (LoadShadersIntoMaterialData*)_data;
	Material* self = loadData->material;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	::System::TimeStamp ts;
#endif
	loadData->shaderProgram->stepped_creation__finalise();
	// and load into the material which is what we wanted to do in the first place
	self->material->set_shader(loadData->usage.get(), loadData->shaderProgram);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	float timeTaken = ts.get_time_since();
	self->loadShaderTime += timeTaken;
	self->loadShaderDetailedTime[3] += timeTaken;
	self->loadShaderUsagesFinished.increase();
	if (self->loadShaderUsagesFinished == self->loadShaderUsagesIssued)
	{
		output(TXT("load shader %8.3fms [%8.3fms > %8.3fms > %8.3fms > %8.3fms] %S"),
			self->loadShaderTime * 1000.0f,
			self->loadShaderDetailedTime[0] * 1000.0f,
			self->loadShaderDetailedTime[1] * 1000.0f,
			self->loadShaderDetailedTime[2] * 1000.0f,
			self->loadShaderDetailedTime[3] * 1000.0f,
			self->get_name().to_string().to_char());
	}
#endif
	loadData->library->preparing_asynchronous_job_done();
}

void Material::add_defaults(Library* _library)
{
	if (! _library->get_materials().find(LibraryName(Names::default_.to_string(), Names::default_.to_string())))
	{
		Material* material = _library->get_materials().find_or_create(LibraryName(Names::default_.to_string(), Names::default_.to_string()));

		material->fragmentShader.set_name(LibraryName(Names::default_.to_string(), Names::defaultRendering.to_string()));
		material->useWithStaticMesh = true;
		material->useWithSkinnedMesh = true;
		material->useWithSkinnedToSingleBoneMesh = true;
	}
}

Material * Material::get_default(Library* _library)
{
	return _library->get_materials().find(LibraryName(Names::default_.to_string(), Names::default_.to_string()));
}

PhysicalMaterialMap * Material::get_physical_material_map() const
{
	return physicalMaterialMap.get();
}
