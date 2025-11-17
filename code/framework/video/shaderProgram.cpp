#include "shaderProgram.h"

#include "texture.h"

#include "..\library\library.h"
#include "..\library\libraryLoadingContext.h"
#include "..\library\usedLibraryStored.inl"

#include "..\game\game.h"
#include "..\jobSystem\jobSystem.h"

#include "..\..\core\concurrency\asynchronousJob.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

REGISTER_FOR_FAST_CAST(ShaderProgram);
LIBRARY_STORED_DEFINE_TYPE(ShaderProgram, shaderProgram);

bool ShaderParam::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);

	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for shader param"));
		result = false;
	}

	if (_node->get_name() == TXT("float"))
	{
		param.set_uniform(_node->get_float());
	}
	else if (_node->get_name() == TXT("invertedFloat"))
	{
		param.set_uniform(1.0f / _node->get_float());
	}
	else if (_node->get_name() == TXT("floatArray"))
	{
		int size = _node->get_int_attribute(TXT("size"), 1);
		float defVal = _node->get_float_attribute(TXT("default"), 0);
		Array<float> a;
		while (a.get_size() < size)
		{
			a.push_back(defVal);
		}
		for_every(n, _node->children_named(TXT("val")))
		{
			int at = n->get_int_attribute(TXT("idx"), 0);
			while (a.get_size() <= at)
			{
				a.push_back(defVal);
			}
			a[at] = n->get_float();
		}
		param.set_uniform(a);
	}
	else if (_node->get_name() == TXT("integer"))
	{
		param.set_uniform((int32)_node->get_int());
	}
	else if (_node->get_name() == TXT("intArray"))
	{
		int size = _node->get_int_attribute(TXT("size"), 1);
		int defVal = _node->get_int_attribute(TXT("default"), 0);
		Array<int> a;
		while (a.get_size() < size)
		{
			a.push_back(defVal);
		}
		for_every(n, _node->children_named(TXT("val")))
		{
			int at = n->get_int_attribute(TXT("idx"), 0);
			while (a.get_size() <= at)
			{
				a.push_back(defVal);
			}
			a[at] = n->get_int();
		}
		param.set_uniform(a);
	}
	else if (_node->get_name() == TXT("vector2"))
	{
		Vector2 v = Vector2::zero;
		v.load_from_xml(_node);
		param.set_uniform(v);
	}
	else if (_node->get_name() == TXT("vector3"))
	{
		Vector3 v = Vector3::zero;
		v.load_from_xml(_node);
		param.set_uniform(v);
	}
	else if (_node->get_name() == TXT("vector4"))
	{
		Vector4 v = Vector4::zero;
		v.load_from_xml(_node);
		param.set_uniform(v);
	}
	else if (_node->get_name() == TXT("vector4Array"))
	{
		int size = _node->get_int_attribute(TXT("size"), 1);
		Vector4 defVal = Vector4::zero;
		defVal.load_from_xml_child_node(_node, TXT("default"));
		Array<Vector4> a;
		while (a.get_size() < size)
		{
			a.push_back(defVal);
		}
		for_every(n, _node->children_named(TXT("val")))
		{
			int at = n->get_int_attribute(TXT("idx"), 0);
			while (a.get_size() <= at)
			{
				a.push_back(defVal);
			}
			a[at].load_from_xml(n);
		}
		param.set_uniform(a);
	}
	else if (_node->get_name() == TXT("colour") ||
			 _node->get_name() == TXT("color"))
	{
		Colour c = Colour(0.0f, 0.0f, 0.0f, 1.0f);
		c.load_from_xml(_node);
		param.set_uniform(c.to_vector4());
	}
	else if (_node->get_name() == TXT("matrix") ||
			 _node->get_name() == TXT("matrix44"))
	{
		Matrix44 m = Matrix44::identity;
		m.load_from_xml(_node);
		param.set_uniform(m);
	}
	else if (_node->get_name() == TXT("texture"))
	{
		textureName.load_from_xml(_node, TXT("use"), _lc);
		param.set_uniform(::System::texture_id_none());
	}
	else
	{
		error(TXT("shader param not recognised"));
		result = false;
	}

	return result;
}

bool ShaderParam::prepare_for_game(Library* _library)
{
	bool result = true;

	if (param.type == ::System::ShaderParamType::texture)
	{
		bool textureFound = false;
		if (textureName.is_valid())
		{
			if (Texture* texture = _library->get_textures().find(textureName))
			{
				param.set_uniform(texture->get_texture()->get_texture_id());
				textureFound = true;
			}
			else
			{
				error(TXT("could not find texture \"%S\""), textureName.to_string().to_char());
				result = false;
			}
		}
		if (!textureFound)
		{
			if (Texture* defaultTexture = _library->get_default_texture())
			{
				param.set_uniform(defaultTexture->get_texture()->get_texture_id());
			}
		}
	}

	return result;
}

//

ShaderParams::ShaderParams()
{
	clear();
}

void ShaderParams::clear()
{
	params.clear();
#ifdef AN_ASSERT
	preparedForGame = false;
#endif
}


bool ShaderParams::load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childName, LibraryLoadingContext & _lc)
{
	bool result = true;
	for_every(paramsChild, _node->children_named(_childName))
	{
		result &= load_from_xml(paramsChild, _lc);
	}
	return result;
}

bool ShaderParams::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	for_every(paramChild, _node->all_children())
	{
		ShaderParam param;
		if (param.load_from_xml(paramChild, _lc))
		{
			params.push_back(param);
		}
		else
		{
			result = false;
		}
	}
	return result;
}

bool ShaderParams::prepare_for_game(Library* _library)
{
	bool result = true;
	for_every(param, params)
	{
		result &= param->prepare_for_game(_library);
	}
#ifdef AN_ASSERT
	preparedForGame = true;
#endif
	return result;
}

void ShaderParams::set_to_default_values_of(::System::ShaderProgram* _shaderProgram) const
{
#ifdef AN_ASSERT
	//!@# an_assert(preparedForGame, TXT("maybe some object was not prepared for game or something is missing?"));
#endif
	for_every(param, params)
	{
		_shaderProgram->access_default_values().set_uniform(param->name, param->param);
	}
}

void ShaderParams::apply_to(::System::ShaderProgram* _shaderProgram) const
{
#ifdef AN_ASSERT
	//!@# an_assert(preparedForGame, TXT("maybe some object was not prepared for game or something is missing?"));
#endif
	for_every(param, params)
	{
		_shaderProgram->set_uniform(param->name, param->param);
	}
}

bool ShaderParams::operator==(ShaderParams const & _other) const
{
	if (params.get_size() != _other.params.get_size())
	{
		return false;
	}
	for_every(param, params)
	{
		bool existsAndSame = false;
		for_every(oParam, _other.params)
		{
			if (*param == *oParam)
			{
				existsAndSame = true;
				break;
			}
		}
		if (!existsAndSame)
		{
			return false;
		}
	}
	return true;
}

ShaderParam const* ShaderParams::get(Name const& _name) const
{
	for_every(param, params)
	{
		if (param->name == _name)
		{
			return param;
		}
	}
	return nullptr;
}

//

ShaderProgram::ShaderProgram(Library * _library, LibraryName const & _name)
: LibraryStored(_library, _name)
, postProcessShader(false)
, primitivesShader(false)
, uiShader(false)
{
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	loadShaderDetailedTime[0] = 0.0f;
	loadShaderDetailedTime[1] = 0.0f;
	loadShaderDetailedTime[2] = 0.0f;
	loadShaderDetailedTime[3] = 0.0f;
#endif
}

struct ShaderProgram_ReleaseShaderProgram
: public Concurrency::AsynchronousJobData
{
	RefCountObjectPtr<::System::ShaderProgram> shaderProgram;
	ShaderProgram_ReleaseShaderProgram(::System::ShaderProgram* _shaderProgram)
	: shaderProgram(_shaderProgram)
	{}

	static void perform(Concurrency::JobExecutionContext const * _executionContext, void* _data)
	{
		ShaderProgram_ReleaseShaderProgram* data = (ShaderProgram_ReleaseShaderProgram*)_data;
		data->shaderProgram = nullptr;
	}
};

ShaderProgram::~ShaderProgram()
{
	if (shaderProgram.get())
	{
		Game::async_system_job(get_library()->get_game(), ShaderProgram_ReleaseShaderProgram::perform, new ShaderProgram_ReleaseShaderProgram(shaderProgram.get()));
	}
}

bool ShaderProgram::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = LibraryStored::load_from_xml(_node, _lc);

	result &= fragmentShader.load_from_xml(_node, TXT("fragmentShader"), _lc);
	result &= vertexShader.load_from_xml(_node, TXT("vertexShader"), _lc);

	result &= params.load_from_xml_child_node(_node, TXT("params"), _lc);

	return result;
}

struct LoadShaderProgramData
: public Concurrency::AsynchronousJobData
{
	ShaderProgram * shaderProgram;
	Library* library;
	VertexShader * vertexShader;
	FragmentShader * fragmentShader;
	LoadShaderProgramData(ShaderProgram * _shaderProgram, Library * _library)
	: shaderProgram(_shaderProgram)
	, library(_library)
	, vertexShader(nullptr)
	, fragmentShader(nullptr)
	{}
};

struct LoadShaderProgramSteppedSetupData
: public Concurrency::AsynchronousJobData
{
	ShaderProgram * shaderProgram;
	Library* library;
	LoadShaderProgramSteppedSetupData(ShaderProgram * _shaderProgram, Library * _library)
	: shaderProgram(_shaderProgram)
	, library(_library)
	{}
};

struct LoadShaderApplyParamsData
: public Concurrency::AsynchronousJobData
{
	ShaderProgram * shaderProgram;
	Library* library;
	LoadShaderApplyParamsData(ShaderProgram * _shaderProgram, Library * _library)
	: shaderProgram(_shaderProgram)
	, library(_library)
	{}
};

bool ShaderProgram::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = LibraryStored::prepare_for_game(_library, _pfgContext);
	if (fragmentShader.is_name_valid())
	{
		result &= fragmentShader.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}
	if (vertexShader.is_name_valid())
	{
		result &= vertexShader.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadShaderProgram)
	{
		LoadShaderProgramData * loadData = new LoadShaderProgramData(this, _library);
		loadData->vertexShader = vertexShader.get();
		loadData->fragmentShader = fragmentShader.get();
		// check usage basing on for what fragment shader is used - we assume that we may not have vertex shader but fragment shader should always be present
		postProcessShader = loadData->fragmentShader && loadData->fragmentShader->is_to_be_used_with_post_process();
		primitivesShader = loadData->fragmentShader && loadData->fragmentShader->is_to_be_used_with_primitives();
		uiShader = loadData->fragmentShader && loadData->fragmentShader->is_to_be_used_with_ui();
		// fill with default
		if (!loadData->vertexShader)
		{
			// decide on vertex shader basing on usage
			if (postProcessShader)
			{
				loadData->vertexShader = VertexShader::get_default_post_process(_library);
			}
			else if (primitivesShader)
			{
				loadData->vertexShader = VertexShader::get_default_primitives(_library);
			}
			else if (uiShader)
			{
				loadData->vertexShader = VertexShader::get_default_ui(_library);
			}
			else
			{
				loadData->vertexShader = VertexShader::get_default_rendering_static(_library);
			}
		}
		if (!loadData->fragmentShader)
		{
			if (postProcessShader)
			{
				loadData->fragmentShader = FragmentShader::get_default_post_process(_library);
			}
			else if (primitivesShader)
			{
				loadData->fragmentShader = FragmentShader::get_default_primitives(_library);
			}
			else if (uiShader)
			{
				loadData->fragmentShader = FragmentShader::get_default_ui(_library);
			}
			else
			{
				loadData->fragmentShader = FragmentShader::get_default_rendering(_library);
			}
		}
		_library->preparing_asynchronous_job_added();
		Game::async_system_job(get_library()->get_game(), load_shader_program, loadData);
	}
	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::LoadParamsIntoShaders)
	{
		result &= params.prepare_for_game(_library);
		LoadShaderApplyParamsData * loadData = new LoadShaderApplyParamsData(this, _library);
		_library->preparing_asynchronous_job_added();
		Game::async_system_job(get_library()->get_game(), apply_params_while_loading, loadData);
	}

	return result;
}

void ShaderProgram::prepare_to_unload()
{
	base::prepare_to_unload();
	vertexShader.clear();
	fragmentShader.clear();
}

void ShaderProgram::load_shader_program(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	LoadShaderProgramData * loadData = (LoadShaderProgramData*)_data;
	if (loadData->vertexShader && loadData->vertexShader->get_shader() && 
		loadData->fragmentShader && loadData->fragmentShader->get_shader())
	{
		ShaderProgram* self = loadData->shaderProgram;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		::System::TimeStamp ts;
#endif
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		::System::TimeStamp tsCreate;
#endif
		::System::ShaderProgramSetup shaderSetup = ::System::ShaderProgramSetup::create(loadData->vertexShader->get_shader(), loadData->fragmentShader->get_shader(), self->get_name().to_string());
		self->shaderProgram = ::System::ShaderProgram::create_stepped();
		self->shaderProgram->stepped_creation__create(shaderSetup);
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		self->loadShaderDetailedTime[0] += tsCreate.get_time_since();
#endif
		// break it into separate jobs to avoid stalling
		loadData->library->preparing_asynchronous_job_added();
		loadData->library->preparing_asynchronous_job_added();
		loadData->library->preparing_asynchronous_job_added();
		Game::async_system_job(loadData->library->get_game(), shader_program__setup_attribs, new LoadShaderProgramSteppedSetupData(self, loadData->library));
		Game::async_system_job(loadData->library->get_game(), shader_program__setup_uniforms, new LoadShaderProgramSteppedSetupData(self, loadData->library));
		Game::async_system_job(loadData->library->get_game(), shader_program__finalise, new LoadShaderProgramSteppedSetupData(self, loadData->library));
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
		self->loadShaderTime += ts.get_time_since();
#endif
	}
	loadData->library->preparing_asynchronous_job_done();
}

void ShaderProgram::shader_program__setup_attribs(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	LoadShaderProgramSteppedSetupData* loadData = (LoadShaderProgramSteppedSetupData*)_data;
	ShaderProgram* self = loadData->shaderProgram;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	::System::TimeStamp ts;
#endif
	self->shaderProgram->stepped_creation__load_attribs();
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	float timeTaken = ts.get_time_since();
	self->loadShaderTime += timeTaken;
	self->loadShaderDetailedTime[1] += timeTaken;
#endif
	loadData->library->preparing_asynchronous_job_done();
}

void ShaderProgram::shader_program__setup_uniforms(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	LoadShaderProgramSteppedSetupData* loadData = (LoadShaderProgramSteppedSetupData*)_data;
	ShaderProgram* self = loadData->shaderProgram;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	::System::TimeStamp ts;
#endif
	self->shaderProgram->stepped_creation__load_uniforms();
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	float timeTaken = ts.get_time_since();
	self->loadShaderTime += timeTaken;
	self->loadShaderDetailedTime[2] += timeTaken;
#endif
	loadData->library->preparing_asynchronous_job_done();
}

void ShaderProgram::shader_program__finalise(Concurrency::JobExecutionContext const* _executionContext, void* _data)
{
	LoadShaderProgramSteppedSetupData* loadData = (LoadShaderProgramSteppedSetupData*)_data;
	ShaderProgram* self = loadData->shaderProgram;
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	::System::TimeStamp ts;
#endif
	self->shaderProgram->stepped_creation__finalise();
#ifdef OUTPUT_LOAD_AND_PREPARE_TIMES_DETAILED
	float timeTaken = ts.get_time_since();
	self->loadShaderTime += timeTaken;
	self->loadShaderDetailedTime[3] += timeTaken;
	output(TXT("load shader %8.3fms [%8.3fms > %8.3fms > %8.3fms > %8.3fms] %S"),
		self->loadShaderTime * 1000.0f,
		self->loadShaderDetailedTime[0] * 1000.0f,
		self->loadShaderDetailedTime[1] * 1000.0f,
		self->loadShaderDetailedTime[2] * 1000.0f,
		self->loadShaderDetailedTime[3] * 1000.0f,
		self->get_name().to_string().to_char());
#endif
	loadData->library->preparing_asynchronous_job_done();
}

void ShaderProgram::apply_params_while_loading(Concurrency::JobExecutionContext const * _executionContext, void* _data)
{
	LoadShaderApplyParamsData * applyParamsData = (LoadShaderApplyParamsData*)_data;
	ShaderProgram* self = applyParamsData->shaderProgram;
	self->params.set_to_default_values_of(self->shaderProgram.get());
	self->shaderProgram->bind();
	self->params.apply_to(self->shaderProgram.get());
	applyParamsData->library->preparing_asynchronous_job_done();
	self->shaderProgram->unbind();
	::System::Video3D::get()->requires_send_all();
	// clear all textures now
	::System::Video3D::get()->mark_use_no_textures();
	::System::Video3D::get()->requires_send_use_textures();
}
