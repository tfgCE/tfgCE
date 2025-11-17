#include "shaderProgram.h"
#include "shaderProgramCache.h"
#include "video3d.h"

#include "viewFrustum.h"
#include "videoClipPlaneStackImpl.inl"

#include "shaderProgramInstance.h"
#include "fragmentShaderOutputSetup.h"
#include "shaderProgramBindingContext.h"

#include "..\core.h"

#include "..\..\io\file.h"

#include "..\..\mainSettings.h"

#include "..\..\containers\arrayStack.h"

#include "..\..\performance\performanceUtils.h"

#ifdef AN_DEVELOPMENT
#include "..\input.h"
#include "..\..\debugKeys.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
#define WARN_ABOUT_MISSING_PARAMS
#endif

#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

//

using namespace System;

//

DEFINE_STATIC_NAME(colour);
DEFINE_STATIC_NAME(inTexture);
DEFINE_STATIC_NAME(inTextureTexelSize);
DEFINE_STATIC_NAME(inTextureSize);
DEFINE_STATIC_NAME(inTextureAspectRatio);
DEFINE_STATIC_NAME(inOutputTexelSize);
DEFINE_STATIC_NAME(inOutputSize);
DEFINE_STATIC_NAME(inOutputTexelSizeFoved);
DEFINE_STATIC_NAME(inOutputSizeFoved);
DEFINE_STATIC_NAME(inOutputAspectRatio);
DEFINE_STATIC_NAME(inSkinningBones);
DEFINE_STATIC_NAME(eyeOffset);
DEFINE_STATIC_NAME(random);
DEFINE_STATIC_NAME(time);
DEFINE_STATIC_NAME(timePendulum);
DEFINE_STATIC_NAME(individual);
DEFINE_STATIC_NAME(normalMatrix); // model+view 3x3 only
DEFINE_STATIC_NAME(normalVRMatrix); // vr 3x3 only
DEFINE_STATIC_NAME(modelViewMatrix);
DEFINE_STATIC_NAME(projectionMatrix);
DEFINE_STATIC_NAME(modelViewClipPlanes);
DEFINE_STATIC_NAME(modelViewClipPlanesCount);
DEFINE_STATIC_NAME(inAPosition);
DEFINE_STATIC_NAME(inONormal);
DEFINE_STATIC_NAME(inOColour);
DEFINE_STATIC_NAME(inOUV);
DEFINE_STATIC_NAME(inOSkinningWeights);
DEFINE_STATIC_NAME(inOSkinningIndices);
DEFINE_STATIC_NAME(inOSkinningIndex);
DEFINE_STATIC_NAME(inOViewRay);

//

ShaderParamInfo::ShaderParamInfo(Name const & _name, ShaderParamID _id)
: name(_name)
, id(_id)
, textureSlot(NONE)
{
}

//

ShaderTextureSlot::ShaderTextureSlot(int _textureSampler, Name const & _uniformName, int _uniformIndex)
: textureSampler(_textureSampler)
, uniformName(_uniformName)
, uniformIndex(_uniformIndex)
{
}

//

String get_program_log(ShaderProgramID _id)
{
	assert_rendering_thread();
	String log;
	GLint infoLen = 0;
	DIRECT_GL_CALL_ glGetProgramiv(_id, GL_INFO_LOG_LENGTH, &infoLen); AN_GL_CHECK_FOR_ERROR

	if (infoLen > 1)
	{
		char8* infoLog = new char8[infoLen];
		DIRECT_GL_CALL_ glGetProgramInfoLog(_id, infoLen, NULL, infoLog); AN_GL_CHECK_FOR_ERROR
		log = String::from_char8(infoLog);
		delete[] infoLog;
	}
	return log;
}

ShaderProgramID link_program(Shader * _vertexShader, Shader * _fragmentShader, OUT_ bool & _retrievable)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ ShaderProgramID program = glCreateProgram(); AN_GL_CHECK_FOR_ERROR
	an_assert(program != 0);
	// mark that we may want to retrieve binary
	_retrievable = false;
#ifndef AN_CLANG
	if (glProgramParameteri)
#endif
	{
		glProgramParameteri(program, GL_PROGRAM_BINARY_RETRIEVABLE_HINT, GL_TRUE);
		_retrievable = true;
	}
	if (_vertexShader)
	{
		DIRECT_GL_CALL_ glAttachShader(program, _vertexShader->get_shader_id()); AN_GL_CHECK_FOR_ERROR
	}
	if (_fragmentShader)
	{
		DIRECT_GL_CALL_ glAttachShader(program, _fragmentShader->get_shader_id()); AN_GL_CHECK_FOR_ERROR
	}
	// bind frag output
#ifdef AN_GL
	FragmentShaderOutputSetup const & fragmentOutputSetup = _fragmentShader->get_fragment_shader_output_setup();
	for_count(int, outputIndex, fragmentOutputSetup.get_output_texture_count())
	{
#ifdef AN_WIDE_CHAR
		DIRECT_GL_CALL_ glBindFragDataLocation(program, outputIndex, fragmentOutputSetup.get_output_texture_definition(outputIndex).get_name().to_string().to_char8_array().get_data()); AN_GL_CHECK_FOR_ERROR
#else
		DIRECT_GL_CALL_ glBindFragDataLocation(program, outputIndex, fragmentOutputSetup.get_output_texture_definition(outputIndex).get_name().to_char()); AN_GL_CHECK_FOR_ERROR
#endif
	}
#endif
#ifdef AN_GLES
	todo_note(TXT("check or force to use gl_FragColor / gl_FragData[x]?"))
#endif
	// link program
	DIRECT_GL_CALL_ glLinkProgram(program); AN_GL_CHECK_FOR_ERROR
	// check the link status
	GLint result;
	DIRECT_GL_CALL_ glGetProgramiv(program, GL_LINK_STATUS, &result); AN_GL_CHECK_FOR_ERROR
	if (result == 0)
	{
		output(TXT("program compilation error"));
		output(get_program_log(program).to_char());
		if (_vertexShader)
		{
			output(TXT("program, vertex"));
			output_break_into_lines(_vertexShader->get_source());
		}
		if (_fragmentShader)
		{
			output(TXT("program, fragment"));
			output_break_into_lines(_fragmentShader->get_source());
		}
		program = 0;
	}
	an_assert(result != 0, TXT("shader program compilation error"));
	return program;
}

ShaderProgram::ShaderProgram(Shader * _vertexShader, Shader * _fragmentShader)
: video3d(Video3D::get())
{
	an_assert(video3d);
	video3d->add(this);

	// before we init because we need it to be connected to our shader program
	defaultValues.set_shader_program_being_part_of_shader_program(this);
	currentValues.set_shader_program_being_part_of_shader_program(this);

	init(ShaderProgramSetup::create(_vertexShader, _fragmentShader));
}

ShaderProgram::ShaderProgram(ShaderProgramSetup const & _setup)
: video3d(Video3D::get())
{
	an_assert(video3d);
	video3d->add(this);

	// before we init because we need it to be connected to our shader program
	defaultValues.set_shader_program_being_part_of_shader_program(this);
	currentValues.set_shader_program_being_part_of_shader_program(this);

	init(_setup);
}

ShaderProgram::ShaderProgram() // for stepped creation
: video3d(Video3D::get())
{
	an_assert(video3d);
	video3d->add(this);

	// before we init because we need it to be connected to our shader program
	defaultValues.set_shader_program_being_part_of_shader_program(this);
	currentValues.set_shader_program_being_part_of_shader_program(this);
}

ShaderProgram* ShaderProgram::create_stepped()
{
	return new ShaderProgram();
}

ShaderProgram::~ShaderProgram()
{
	close();
	an_assert(! video3d->boundBuffersAndVertexFormat.isBound || video3d->boundBuffersAndVertexFormat.shaderProgram != this);

	an_assert(video3d);
	video3d->remove(this);
}

#define GET_UNIFORM_ONE(_dataType, _glExpectedType, _gl_get_uniform) \
	if (size == 1) \
	{ \
		_dataType val; \
		DIRECT_GL_CALL_ _gl_get_uniform(program, index, (_glExpectedType*)&val); AN_GL_CHECK_FOR_ERROR \
		value = ShaderParam(val); \
	} \
	else \
	{ \
		todo_important(TXT("not implemented!")); \
	}

#define GET_UNIFORM_ONE_OR_ARRAY(_dataType, _glExpectedType, _gl_get_uniform) \
	if (size == 1) \
	{ \
		_dataType val; \
		DIRECT_GL_CALL_ _gl_get_uniform(program, index, (_glExpectedType*)&val); AN_GL_CHECK_FOR_ERROR \
		value = ShaderParam(val); \
	} \
	else if (size > 1 ) \
	{ \
		Array<_dataType> val; \
		val.set_size(size); \
		for_count(int, i, size) \
		{ \
			DIRECT_GL_CALL_ _gl_get_uniform(program, index + i, (_glExpectedType*)(&val[i])); AN_GL_CHECK_FOR_ERROR \
		} \
		value = ShaderParam(val); \
	} \
	else \
	{ \
		todo_important(TXT("what now")); \
	}

#define GET_UNIFORM_ONE_OR_ARRAY_MATRICES_TRANSPOSED(_dataType, _glExpectedType, _gl_get_uniform) \
	if (size == 1) \
	{ \
		_dataType val; \
		DIRECT_GL_CALL_ _gl_get_uniform(program, index, (_glExpectedType*)&val); AN_GL_CHECK_FOR_ERROR \
		val = val.transposed(); \
		value = ShaderParam(val); \
	} \
	else if (size > 1 ) \
	{ \
		Array<_dataType> val; \
		val.set_size(size); \
		for_count(int, i, size) \
		{ \
			DIRECT_GL_CALL_ _gl_get_uniform(program, index + i, (_glExpectedType*)(&val[i])); AN_GL_CHECK_FOR_ERROR \
			val[i] = val[i].transposed(); \
		} \
		value = ShaderParam(val); \
	} \
	else \
	{ \
		todo_important(TXT("what now")); \
	}

void ShaderProgram::init(ShaderProgramSetup const & _setup)
{
	assert_rendering_thread();

	stepped_creation__create(_setup);
	stepped_creation__load_attribs();
	stepped_creation__load_uniforms();
	stepped_creation__finalise();
}

void ShaderProgram::stepped_creation__create(ShaderProgramSetup const & _setup)
{
	assert_rendering_thread();

	// get default output setup
	if (_setup.fragmentShader->access_fragment_shader_output_setup().get_output_texture_count() == 0)
	{
		if (!_setup.fragmentShader->access_fragment_shader_output_setup().does_allow_default_output())
		{
			warn(TXT("no outputs defined for fragment shader nor \"allowsDefaultOutput\" node provided : <outputs allowsDefaultOutput=\"true\"/>"));
		}
		_setup.fragmentShader->access_fragment_shader_output_setup().have_at_least_one_default_output(OutputTextureDefinition(NAME(colour)));
	}

	shadersAttached = false;

	if (_setup.vertexShader.get() &&
		_setup.fragmentShader.get())
	{
		bool justCompiled = false;
		bool retrievable = false;

		if (program == 0)
		{
			if (!_setup.shaderProgramCacheId.is_empty() && video3d->should_use_shader_program_cache())
			{
				// try to use cache without even accessing sources
				int binaryFormat;
				Array<int8> binary;
				if (video3d->access_shader_program_cache().get_binary(_setup.shaderProgramCacheId, binaryFormat, binary))
				{
					DIRECT_GL_CALL_ program = glCreateProgram(); AN_GL_CHECK_FOR_ERROR
					an_assert(program != 0);
					DIRECT_GL_CALL_ glProgramBinary(program, binaryFormat, binary.get_data(), binary.get_size()); AN_GL_CHECK_FOR_ERROR
					GLint result;
					DIRECT_GL_CALL_ glGetProgramiv(program, GL_LINK_STATUS, &result); AN_GL_CHECK_FOR_ERROR
					if (result == 0)
					{
						// loading failed, we will need to compile it again
						DIRECT_GL_CALL_ glDeleteProgram(program); AN_GL_CHECK_FOR_ERROR
						program = 0;
					}
				}
			}
		}

		if (program == 0)
		{
			if (!_setup.shaderProgramCacheId.is_empty() && video3d->should_use_shader_program_cache())
			{
				// try to use cache
				int binaryFormat;
				Array<int8> binary;
				if (video3d->access_shader_program_cache().get_binary(_setup.shaderProgramCacheId, _setup.vertexShader->get_source(), _setup.fragmentShader->get_source(), binaryFormat, binary))
				{
					DIRECT_GL_CALL_ program = glCreateProgram(); AN_GL_CHECK_FOR_ERROR
					an_assert(program != 0);
					DIRECT_GL_CALL_ glProgramBinary(program, binaryFormat, binary.get_data(), binary.get_size()); AN_GL_CHECK_FOR_ERROR
					GLint result;
					DIRECT_GL_CALL_ glGetProgramiv(program, GL_LINK_STATUS, &result); AN_GL_CHECK_FOR_ERROR
					if (result == 0)
					{
						// loading failed, we will need to compile it again
						DIRECT_GL_CALL_ glDeleteProgram(program); AN_GL_CHECK_FOR_ERROR
						program = 0;
					}
				}
			}
		}

		if (program == 0)
		{
			// make sure shaders are compiled
			_setup.vertexShader->make_sure_shader_is_compiled();
			_setup.fragmentShader->make_sure_shader_is_compiled();
			// link program
			program = link_program(_setup.vertexShader.get(), _setup.fragmentShader.get(), OUT_ retrievable);
			if (program != 0)
			{
				shadersAttached = true;
				justCompiled = true;
			}
			else
			{
				shaderCompilationOrLinkingFailed = true;
			}
		}

		if (program != 0 && justCompiled && retrievable)
		{
			if (!_setup.shaderProgramCacheId.is_empty() && video3d->should_use_shader_program_cache())
			{
				// store in cache
				GLint binarySize = 0;
				DIRECT_GL_CALL_ glGetProgramiv(program, GL_PROGRAM_BINARY_LENGTH, &binarySize); AN_GL_CHECK_FOR_ERROR
				Array<int8> binary;
				binary.set_size(binarySize);
				GLint actualBinarySize;
				GLenum binaryFormat;
				DIRECT_GL_CALL_ glGetProgramBinary(program, binarySize, &actualBinarySize, &binaryFormat, binary.get_data()); AN_GL_CHECK_FOR_ERROR
				an_assert(actualBinarySize == binarySize);
				video3d->access_shader_program_cache().set_binary(_setup.shaderProgramCacheId, _setup.vertexShader->get_source(), _setup.fragmentShader->get_source(), binaryFormat, binary);
			}
		}
	}

	vertexShader = _setup.vertexShader.get() ? _setup.vertexShader.get() : nullptr;
	fragmentShader = _setup.fragmentShader.get() ? _setup.fragmentShader.get() : nullptr;

	vertexShaderID = vertexShader.get() && shadersAttached ? vertexShader->get_shader_id() : 0;
	fragmentShaderID = fragmentShader.get() && shadersAttached ? fragmentShader->get_shader_id() : 0;
}

void ShaderProgram::stepped_creation__load_attribs()
{
	assert_rendering_thread();

	if (program == 0)
	{
		return;
	}

	static GLchar GL_OUTPUT_BUFFER[8192];
	int numVars;

	// load attributes
	attributes.clear();
	DIRECT_GL_CALL_ glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numVars); AN_GL_CHECK_FOR_ERROR
	for (int i = 0; i < numVars; ++i)
	{
		GLsizei bufSize = 8192;
		GLsizei size; GLenum type;
		DIRECT_GL_CALL_ glGetActiveAttrib(program, i, bufSize, &bufSize, &size, &type, GL_OUTPUT_BUFFER); AN_GL_CHECK_FOR_ERROR
#ifdef AN_WIDE_CHAR
		String name = String::from_char8(GL_OUTPUT_BUFFER);
#else
		String name = String(GL_OUTPUT_BUFFER);
#endif
		DIRECT_GL_CALL_ ShaderParamID index = glGetAttribLocation(program, GL_OUTPUT_BUFFER); AN_GL_CHECK_FOR_ERROR
		attributes.push_back(ShaderParamInfo(Name(name), index));
		attributeMap[Name(name)] = attributes.get_size() - 1;
	}
}

void ShaderProgram::stepped_creation__load_uniforms()
{
	assert_rendering_thread();

	if (program == 0)
	{
		return;
	}

	static GLchar GL_OUTPUT_BUFFER[8192];
	int numVars;

	// load uniforms (and texture slots)
	uniforms.clear();
	defaultValues.uniforms.clear();
	textureSlots.clear();
	Video3D::get()->bind_shader_program(program, this); // bind program to allow to set_uniforms
	Video3D::get()->requires_send_shader_program();

	DIRECT_GL_CALL_ glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numVars); AN_GL_CHECK_FOR_ERROR

	uniforms.make_space_for(numVars);
	for (int i = 0; i < numVars; ++i)
	{
		GLsizei bufSize = 8192;
		GLsizei size;
		GLenum type;
		DIRECT_GL_CALL_ glGetActiveUniform(program, i, bufSize, &bufSize, &size, &type, GL_OUTPUT_BUFFER); AN_GL_CHECK_FOR_ERROR
		String name;
#ifdef AN_WIDE_CHAR
		name = String::from_char8(GL_OUTPUT_BUFFER);
#else
		name = String(GL_OUTPUT_BUFFER);
#endif
		GLint index;
		DIRECT_GL_CALL_ index = glGetUniformLocation(program, GL_OUTPUT_BUFFER); AN_GL_CHECK_FOR_ERROR
		if (name.get_right(3) == TXT("[0]"))
		{
			name = name.get_left(name.get_length() - 3);
		}
		Name uniformName;
		uniformName = Name(name);
		int uniformIndex = uniforms.get_size();
		uniforms.push_back(ShaderParamInfo(uniformName, index));
		uniformMap[uniformName] = uniformIndex;

		// values should be always stored, even if they are not set
		ShaderParam value;
		// get current value
		{
			if (
#ifdef AN_GL
				type == GL_SAMPLER_1D ||
#endif
				type == GL_SAMPLER_2D ||
				type == GL_SAMPLER_3D ||
				type == GL_SAMPLER_CUBE)
			{
				int textureSlotIndex = textureSlots.get_size();
				set_uniform(uniformIndex, (int32)textureSlotIndex); // bind sampler
				// textureSampler is same as textureSlot index
				textureSlots.push_back(ShaderTextureSlot(textureSlotIndex, uniformName, uniformIndex));
				textureUniformMap[uniformName] = textureSlotIndex;
				uniforms.get_last().textureSlot = textureSlotIndex;
				// value should remain not set
			}
			else
			{
				// skip build ins as:
				// 1) we will be filling them anyway and we do not want to write them twice
				// 2) and if we store them here, they will be set in shader program instance with filled missing (done somewhere else)
				// 3) also they could overwrite what we wrote in set_build_in_uniforms
				if (!is_build_in_uniform(uniformName) &&
					!video3d->is_custom_build_in_uniform(uniformName))
				{
					bool skip = false;
					// skip those that we have defined
					TypeId vType = type_id_none();
					TypeId fType = type_id_none();
					if (!skip && vertexShader->get_default_values().get_existing_type_id(uniformName, vType))
					{
						todo_note(TXT("we assume the same type"));
						skip = true;
					}
					if (!skip && fragmentShader->get_default_values().get_existing_type_id(uniformName, fType))
					{
						todo_note(TXT("we assume the same type"));
						skip = true;
					}
					if (! skip)
					{
						if (type == GL_INT)
						{
							GET_UNIFORM_ONE_OR_ARRAY(int32, int32, glGetUniformiv);
						}
						else if (type == GL_INT_VEC4)
						{
							GET_UNIFORM_ONE(VectorInt4, int32, glGetUniformiv);
						}
						else if (type == GL_FLOAT)
						{
							GET_UNIFORM_ONE_OR_ARRAY(float, float, glGetUniformfv);
						}
						else if (type == GL_FLOAT_VEC2)
						{
							GET_UNIFORM_ONE_OR_ARRAY(Vector2, float, glGetUniformfv);
						}
						else if (type == GL_FLOAT_VEC3)
						{
							GET_UNIFORM_ONE_OR_ARRAY(Vector3, float, glGetUniformfv);
						}
						else if (type == GL_FLOAT_VEC4)
						{
							GET_UNIFORM_ONE_OR_ARRAY(Vector4, float, glGetUniformfv);
						}
						// let's skip storing matrices as this goes really wrong and
						// 1) I don't know why and have no time to investigate
						// 2) I don't really care as those values should be always provided anyway
						else if (type == GL_FLOAT_MAT3)
						{
							//GET_UNIFORM_ONE_OR_ARRAY_MATRICES_TRANSPOSED(Matrix33, float, glGetUniformfv);
						}
						else if (type == GL_FLOAT_MAT4)
						{
							//GET_UNIFORM_ONE_OR_ARRAY_MATRICES_TRANSPOSED(Matrix44, float, glGetUniformfv);
						}
						else
						{
							// not handling stereos atm
							todo_important(TXT("implement_ type!"));
						}
						// always store value in such a case
					}
				}
			}
		}
		// store value
		defaultValues.uniforms.push_back(value);
	}

	if (vertexShader.is_set())
	{
		add_forced_uniforms_from(vertexShader->get_default_values());
		defaultValues.set_uniforms_from(vertexShader->get_default_values());
	}
	if (fragmentShader.is_set())
	{
		add_forced_uniforms_from(fragmentShader->get_default_values());
		defaultValues.set_uniforms_from(fragmentShader->get_default_values());
	}
	Video3D::get()->unbind_shader_program(program); // unbind now
}

void ShaderProgram::stepped_creation__finalise()
{
	assert_rendering_thread();

	// setup uniforms
	inTextureUniformIndex = find_uniform_index(NAME(inTexture));
	inTextureTexelSizeUniformIndex = find_uniform_index(NAME(inTextureTexelSize));
	inTextureSizeUniformIndex = find_uniform_index(NAME(inTextureSize));
	inTextureAspectRatioUniformIndex = find_uniform_index(NAME(inTextureAspectRatio));
	inOutputTexelSizeUniformIndex = find_uniform_index(NAME(inOutputTexelSize));
	inOutputSizeUniformIndex = find_uniform_index(NAME(inOutputSize));
	inOutputTexelSizeFovedUniformIndex = find_uniform_index(NAME(inOutputTexelSizeFoved));
	inOutputSizeFovedUniformIndex = find_uniform_index(NAME(inOutputSizeFoved));
	inOutputAspectRatioUniformIndex = find_uniform_index(NAME(inOutputAspectRatio));
	inSkinningBonesUniformIndex = find_uniform_index(NAME(inSkinningBones));
	eyeOffsetIndex = find_uniform_index(NAME(eyeOffset));
	randomIndex = find_uniform_index(NAME(random));
	timeIndex = find_uniform_index(NAME(time));
	timePendulumIndex = find_uniform_index(NAME(timePendulum));
	individualIndex = find_uniform_index(NAME(individual));
	normalMatrixUniformIndex = find_uniform_index(NAME(normalMatrix));
	normalVRMatrixUniformIndex = find_uniform_index(NAME(normalVRMatrix));
	modelViewMatrixUniformIndex = find_uniform_index(NAME(modelViewMatrix));
	projectionMatrixUniformIndex = find_uniform_index(NAME(projectionMatrix));
	modelViewClipPlanesUniformIndex = find_uniform_index(NAME(modelViewClipPlanes));
	modelViewClipPlanesCountUniformIndex = find_uniform_index(NAME(modelViewClipPlanesCount));

	// setup attributes
	inPositionAttributeIndex = find_attribute_index(NAME(inAPosition));
	inNormalAttributeIndex = find_attribute_index(NAME(inONormal));
	if (MainSettings::global().rendering_pipeline_should_use_colour_vertex_attributes())
	{
		inColourAttributeIndex = find_attribute_index(NAME(inOColour));
	}
	else
	{
		inColourAttributeIndex = NONE;
	}
	inUVAttributeIndex = find_attribute_index(NAME(inOUV));
	inSkinningWeightsAttributeIndex = find_attribute_index(NAME(inOSkinningWeights));
	inSkinningIndicesAttributeIndex = find_attribute_index(NAME(inOSkinningIndices));
	inSkinningIndexAttributeIndex = find_attribute_index(NAME(inOSkinningIndex));
	inViewRayAttributeIndex = find_attribute_index(NAME(inOViewRay));
}

void ShaderProgram::close()
{
	video3d->on_close(this);
	if (program != 0)
	{
		assert_rendering_thread();
		if (vertexShaderID != 0 && shadersAttached)
		{
			DIRECT_GL_CALL_ glDetachShader(program, vertexShaderID); AN_GL_CHECK_FOR_ERROR
		}
		if (fragmentShaderID != 0 && shadersAttached)
		{
			DIRECT_GL_CALL_ glDetachShader(program, fragmentShaderID); AN_GL_CHECK_FOR_ERROR
		}
		shadersAttached = false;
		vertexShader = nullptr;
		fragmentShader = nullptr;
		DIRECT_GL_CALL_ glDeleteProgram(program); AN_GL_CHECK_FOR_ERROR
		program = 0;
	}
}

void ShaderProgram::bind(bool _setCustomBuildInUniformsToo)
{
	video3d->bind_shader_program(program, this);
	video3d->requires_send_shader_program();
	set_build_in_uniforms(_setCustomBuildInUniformsToo);
	for_every(textureSlot, textureSlots)
	{
		if (currentValues.uniforms.is_index_valid(textureSlot->uniformIndex))
		{
			video3d->mark_use_texture(textureSlot->textureSampler, currentValues.uniforms[textureSlot->uniformIndex].textureID);
		}
		else
		{
			video3d->mark_use_texture(textureSlot->textureSampler, texture_id_none());
		}
	}
}

void ShaderProgram::unbind()
{
	// set textures to none (just in video3d)
	for_every(textureSlot, textureSlots)
	{
		video3d->mark_use_texture(textureSlot->textureSampler, texture_id_none());
	}
	video3d->unbind_shader_program(program);
}

void ShaderProgram::use_default_textures_for_missing(ShaderProgramBindingContext const & _bindingContext)
{
	for_every(textureSlot, textureSlots)
	{
		video3d->mark_use_texture_if_not_set(textureSlot->textureSampler, _bindingContext.get_default_texture_id());
	}
}

void ShaderProgram::use_for_all_textures(TextureID const & _textureID)
{
	for_every(textureSlot, textureSlots)
	{
		video3d->mark_use_texture(textureSlot->textureSampler, _textureID);
	}
}

void ShaderProgram::set_custom_build_in_uniforms()
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(set_custom_build_in_uniforms);
#endif
	video3d->apply_custom_set_build_in_uniforms_for(this);
}

bool ShaderProgram::is_build_in_uniform(Name const & _shaderParam) const
{
	return _shaderParam == NAME(inTexture) ||
		   _shaderParam == NAME(inTextureTexelSize) ||
		   _shaderParam == NAME(inTextureSize) ||
		   _shaderParam == NAME(inTextureAspectRatio) ||
		   _shaderParam == NAME(inOutputTexelSize) ||
		   _shaderParam == NAME(inOutputSize) ||
		   _shaderParam == NAME(inOutputTexelSizeFoved) ||
		   _shaderParam == NAME(inOutputSizeFoved) ||
		   _shaderParam == NAME(inOutputAspectRatio) ||
		   _shaderParam == NAME(inSkinningBones) ||
		   _shaderParam == NAME(eyeOffset) ||
		   _shaderParam == NAME(random) ||
		   _shaderParam == NAME(time) ||
		   _shaderParam == NAME(timePendulum) ||
		   _shaderParam == NAME(individual) ||
		   _shaderParam == NAME(normalMatrix) ||
		   _shaderParam == NAME(normalVRMatrix) ||
		   _shaderParam == NAME(modelViewMatrix) ||
		   _shaderParam == NAME(projectionMatrix) ||
		   _shaderParam == NAME(modelViewClipPlanes) ||
		   _shaderParam == NAME(modelViewClipPlanesCount);
}

bool ShaderProgram::is_automatic_build_in_uniform(Name const & _shaderParam) const
{
	return _shaderParam == NAME(eyeOffset) ||
		   _shaderParam == NAME(random) ||
		   _shaderParam == NAME(time) ||
		   _shaderParam == NAME(timePendulum) ||
		   _shaderParam == NAME(individual) ||
		   _shaderParam == NAME(normalMatrix) ||
		   _shaderParam == NAME(normalVRMatrix) ||
		   _shaderParam == NAME(modelViewMatrix) ||
		   _shaderParam == NAME(projectionMatrix) ||
		   _shaderParam == NAME(modelViewClipPlanes) ||
		   _shaderParam == NAME(modelViewClipPlanesCount);
}

void ShaderProgram::set_build_in_uniforms(bool _setCustomBuildInUniformsToo, bool _eyeRelatedOnly)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(set_build_in_uniforms);
#endif
	if (_setCustomBuildInUniformsToo)
	{
		set_custom_build_in_uniforms();
	}

	if (eyeOffsetIndex != NONE)
	{
		set_uniform(eyeOffsetIndex, ::System::Video3D::get()->get_eye_offset());
	}
	if (randomIndex != NONE)
	{
		set_uniform(randomIndex, Random::get_float(0.0f, 1.0f));
	}
	if (timeIndex != NONE)
	{
		set_uniform(timeIndex, ::System::Core::get_timer());
	}
	if (timePendulumIndex != NONE)
	{
		set_uniform(timePendulumIndex, ::System::Core::get_timer_pendulum());
	}
	if (individualIndex != NONE)
	{
		set_uniform(individualIndex, ::System::Video3D::get()->get_current_individual_offset());
	}
	if (!_eyeRelatedOnly)
	{
		if (normalMatrixUniformIndex != NONE || modelViewMatrixUniformIndex != NONE)
		{
			Matrix44 const & modelViewMatrix = video3d->get_model_view_matrix_stack().get_current_for_rendering();
			if (modelViewMatrixUniformIndex != NONE)
			{
				set_uniform(modelViewMatrixUniformIndex, modelViewMatrix);
			}
			if (normalMatrixUniformIndex != NONE)
			{
				Matrix33 const normalMatrix = modelViewMatrix.to_matrix33();
				set_uniform(normalMatrixUniformIndex, normalMatrix);
			}
		}
		if (normalVRMatrixUniformIndex != NONE)
		{
			auto & vrForRendering = video3d->get_model_view_matrix_stack().get_vr_for_rendering();
			Matrix44 const & vrMatrix = vrForRendering.is_set()? vrForRendering.get() : video3d->get_model_view_matrix_stack().get_current_for_rendering();
			if (normalVRMatrixUniformIndex != NONE)
			{
				Matrix33 const normalVRMatrix = vrMatrix.to_matrix33();
				set_uniform(normalVRMatrixUniformIndex, normalVRMatrix);
			}
		}
	}
	if (!_eyeRelatedOnly)
	{
		if (projectionMatrixUniformIndex != NONE) set_uniform(projectionMatrixUniformIndex, video3d->get_projection_matrix());
	}
	if (modelViewClipPlanesUniformIndex != NONE)
	{
		video3d->access_clip_plane_stack().ready_current(); // we need it to be ready by this point, make sure it is ready
		ARRAY_STATIC(Vector4, clipPlanes, PLANE_SET_PLANE_LIMIT * 2);
		{
			auto const& clipPlaneSet = video3d->get_clip_plane_stack().get_current_for_rendering();
			for_every(clipPlane, clipPlaneSet.planes)
			{
				clipPlanes.push_back(clipPlane->to_vector4_for_rendering());
				if (clipPlanes.get_size() >= PLANE_SET_PLANE_LIMIT)
				{
					break;
				}
			}
		}
#ifndef AN_OPEN_CLOSE_CLIP_PLANES
		video3d->access_clip_plane_stack().open_close_clip_planes_for_rendering();
#endif
		int useCount = min(PLANE_SET_PLANE_LIMIT, clipPlanes.get_size());
		while (clipPlanes.get_size() < PLANE_SET_PLANE_LIMIT)
		{
			clipPlanes.push_back(Vector4(0.0f, 0.0f, -1.0f, 1000.0f + (float)clipPlanes.get_size())); // distant and not aligned with each other
		}
		clipPlanes.set_size(PLANE_SET_PLANE_LIMIT);
		set_uniform(modelViewClipPlanesUniformIndex, &clipPlanes.get_first(), clipPlanes.get_size());
		if (modelViewClipPlanesCountUniformIndex != NONE)
		{
			set_uniform(modelViewClipPlanesCountUniformIndex, useCount);
		}
	}
}

void ShaderProgram::set_build_in_uniform_in_texture(TextureID const & _textureID)
{
	if (inTextureUniformIndex != NONE)
	{
		set_uniform(inTextureUniformIndex, _textureID);
	}
}

void ShaderProgram::set_build_in_uniform_in_texture_size_related_uniforms(Vector2 const & _inTextureSize, Vector2 const & _outSize, Optional<float> const& _fov)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(set_build_in_uniform_in_texture_size_related_uniforms);
#endif
	if (inTextureTexelSizeUniformIndex != NONE)
	{
		set_uniform(inTextureTexelSizeUniformIndex, _inTextureSize.inverted());
	}
	if (inTextureSizeUniformIndex != NONE)
	{
		set_uniform(inTextureSizeUniformIndex, _inTextureSize);
	}
	if (inTextureAspectRatioUniformIndex != NONE)
	{
		set_uniform(inTextureAspectRatioUniformIndex, Vector2(_inTextureSize.x / _inTextureSize.y, 1.0f));
	}
	if (inOutputTexelSizeUniformIndex != NONE)
	{
		set_uniform(inOutputTexelSizeUniformIndex, _outSize.inverted());
	}
	if (inOutputSizeUniformIndex != NONE)
	{
		set_uniform(inOutputSizeUniformIndex, _outSize);
	}
	if (inOutputTexelSizeFovedUniformIndex != NONE ||
		inOutputSizeFovedUniformIndex != NONE)
	{
		float sizeFoved = tan_deg(_fov.get(90.0f) * 0.5f) * _outSize.y;
		sizeFoved /= 2.0f; // to make them larger a bit, to be more stable
		if (inOutputTexelSizeFovedUniformIndex != NONE)
		{
			set_uniform(inOutputTexelSizeFovedUniformIndex, safe_inv(sizeFoved));
		}
		if (inOutputSizeFovedUniformIndex != NONE)
		{
			set_uniform(inOutputSizeFovedUniformIndex, sizeFoved);
		}
	}
	if (inOutputAspectRatioUniformIndex != NONE)
	{
		set_uniform(inOutputAspectRatioUniformIndex, Vector2(_outSize.x / _outSize.y, 1.0f));
	}
}

void ShaderProgram::apply(Name const & _name, ShaderParam const & _param, bool _force)
{
	if (is_automatic_build_in_uniform(_name) ||
		Video3D::get()->is_custom_build_in_uniform(_name))
	{
		// skip build in
		return;
	}
	if (auto idx = uniformMap.get_existing(_name))
	{
		apply(*idx, _param, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::apply(int _idx, ShaderParam const& _param, bool _force)
{
	assert_rendering_thread();
	ShaderParamInfo const& uniform = uniforms[_idx];
	if (is_automatic_build_in_uniform(uniform.name) ||
		Video3D::get()->is_custom_build_in_uniform(uniform.name))
	{
		// skip build in
		return;
	}
	apply_non_build_in(_idx, uniform, _param, _force);
}

void ShaderProgram::apply_non_build_in(int _idx, ShaderParamInfo const& uniform, ShaderParam const& _param, bool _force)
{
	currentValues.uniforms.set_size_to_have(_idx);
	ShaderParam& uniformValue = currentValues.uniforms[_idx];
	_force |= _param.forced;

#ifdef AN_N_RENDERING
	if (::System::Input::get()->has_key_been_pressed(System::Key::N) &&
		_param.type != ShaderParamType::notSet)
	{
		output(TXT("%S shader param %S : %S"),
			(_param.type != ShaderParamType::texture &&
				!_force &&
				uniformValue == _param) ? TXT("keeping") : TXT("applying"),
			get_uniform_name(_idx).to_char(), _param.value_to_string().to_char());
	}
#endif

	if (_param.type != ShaderParamType::texture &&
		!_force &&
		uniformValue == _param)
	{
		// everything same
		return;
	}

	apply_value(uniform, uniformValue, _param, _force);
}

void ShaderProgram::apply_value(ShaderParamInfo const& uniform, ShaderParam& uniformValue, ShaderParam const& _param, bool _force)
{
	an_assert(!_param.forced || _force, TXT("apply param.forced before calling this"));

	if (_param.type == ShaderParamType::texture)
	{
		// uniform is set at the creation time (check if this uniform is related to textureSlot)
		an_assert(uniform.textureSlot != NONE);
		// always set texture
		mark_use_texture_slot(uniform.textureSlot, _param.textureID);
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform1i)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1i(uniform.id, _param.valueI[0]); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform1iArray)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1iv(uniform.id, _param.valueDsize / sizeof(int32), (GLint const*)_param.get_uniform_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform1f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1f(uniform.id, _param.valueF[0]); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform1fArray)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1fv(uniform.id, _param.valueDsize / sizeof(float), (float const*)_param.get_uniform_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform2f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform2f(uniform.id, _param.valueF[0], _param.valueF[1]); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformVectors2f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform2fv(uniform.id, _param.valueDsize / sizeof(Vector2), (float const*)_param.get_uniform_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform3f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform3f(uniform.id, _param.valueF[0], _param.valueF[1], _param.valueF[2]); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformVectors3f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform3fv(uniform.id, _param.valueDsize / sizeof(Vector3), (float const*)_param.get_uniform_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformVectorsStereo3f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform3fv(uniform.id, 2, (float const*)_param.valueF); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform4f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4f(uniform.id, _param.valueF[0], _param.valueF[1], _param.valueF[2], _param.valueF[3]); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformVectors4f ||
			 _param.type == ShaderParamType::uniformVectorsPointer4f)
	{
		if (uniform.id != NONE)
		{
			void const* pStart = _param.type == ShaderParamType::uniformVectors4f ? _param.get_uniform_data() : _param.valueDptr;
			int32 pDataSize = _param.valueDsize;
			DIRECT_GL_CALL_ glUniform4fv(uniform.id, pDataSize / sizeof(Vector4), (float const*)pStart); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformVectorsStereo4f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4fv(uniform.id, 2, (float const*)_param.valueF); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniform4i)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4i(uniform.id, _param.valueI[0], _param.valueI[1], _param.valueI[2], _param.valueI[3]); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformMatrix3f)
	{
		// turn row-major to column-major (transpose)
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix3fv(uniform.id, 1, GL_FALSE, _param.valueF); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformMatrices3f ||
			 _param.type == ShaderParamType::uniformMatricesPointer3f)
	{
		if (uniform.id != NONE)
		{
			void const* pStart = _param.type == ShaderParamType::uniformMatrices3f ? _param.get_uniform_data() : _param.valueDptr;
			int32 pDataSize = _param.valueDsize;
			if (Video3D::get()->should_use_vec4_arrays_instead_of_mat4_arrays())
			{
				DIRECT_GL_CALL_ glUniform3fv(uniform.id, pDataSize / sizeof(Vector3), (float const*)pStart); AN_GL_CHECK_FOR_ERROR
			}
			else
			{
				// turn row-major to column-major (transpose)
				DIRECT_GL_CALL_ glUniformMatrix3fv(uniform.id, pDataSize / sizeof(Matrix33), GL_FALSE, (float const*)pStart); AN_GL_CHECK_FOR_ERROR
			}
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformMatrix4f)
	{
		// turn row-major to column-major (transpose)
		DIRECT_GL_CALL_ glUniformMatrix4fv(uniform.id, 1, GL_FALSE, _param.valueF); AN_GL_CHECK_FOR_ERROR
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformMatrices4f ||
			 _param.type == ShaderParamType::uniformMatricesPointer4f)
	{
		if (uniform.id != NONE)
		{
			void const* pStart = _param.type == ShaderParamType::uniformMatrices4f ? _param.get_uniform_data() : _param.valueDptr;
			int32 pDataSize = _param.valueDsize;
			if (Video3D::get()->should_use_vec4_arrays_instead_of_mat4_arrays())
			{
				DIRECT_GL_CALL_ glUniform4fv(uniform.id, pDataSize / sizeof(Vector4), (float const*)pStart); AN_GL_CHECK_FOR_ERROR
			}
			else
			{
				// turn row-major to column-major (transpose)
				DIRECT_GL_CALL_ glUniformMatrix4fv(uniform.id, pDataSize / sizeof(Matrix44), GL_FALSE, (float const*)pStart); AN_GL_CHECK_FOR_ERROR
			}
		}
		uniformValue.store(_param, _force);
	}
	else if (_param.type == ShaderParamType::uniformMatricesStereo4f)
	{
		if (uniform.id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix4fv(uniform.id, 2, GL_FALSE, (float const*)_param.valueF); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.store(_param, _force);
	}
	else
	{
		an_assert(_param.type == ShaderParamType::notSet, TXT("not implemented?"));
	}
}

void ShaderProgram::apply(ShaderProgramInstance const& _set, ShaderProgramBindingContext const& _bindingContext, bool _force)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(ShaderProgram_apply);
#endif
	int bcLeft = _bindingContext.shaderParams.get_size();
	int idx = 0;
	auto* uniformDef = uniforms.get_data();
	currentValues.uniforms.set_size_to_have(_set.uniforms.get_size());
	auto* currentParam = currentValues.uniforms.get_data();
	for_every_const(param, _set.uniforms)
	{
		bool forced = _force || param->forced;
		if (! is_automatic_build_in_uniform(uniformDef->name) &&
			! Video3D::get()->is_custom_build_in_uniform(uniformDef->name))
		{
			ShaderParam const* paramToApply = param;
			if (bcLeft > 0)
			{
				for_every(bcu, _bindingContext.shaderParams)
				{
					if (bcu->name == uniformDef->name)
					{
						paramToApply = &bcu->param;
						--bcLeft;
						break;
					}
				}
			}
			if (param->type == ShaderParamType::texture ||
				forced ||
				*currentParam != *paramToApply)
			{
				apply_value(*uniformDef, *currentParam, *paramToApply, forced);
			}
		}
		++idx;
		++uniformDef;
		++currentParam;
	}
}

void ShaderProgram::apply(ShaderProgramBindingContext const & _bindingContext, bool _force)
{
	for_every(shaderParam, _bindingContext.shaderParams)
	{
		// do check here, as not all params have to be present in all shaders
		if (auto idx = uniformMap.get_existing(shaderParam->name))
		{
			apply(*idx, shaderParam->param, _force);
		}
	}
}

void ShaderProgram::apply(ShaderProgramInstance const & _set, bool _force)
{
	an_assert(_set.shaderProgram == this);
	an_assert(_set.uniforms.get_size() <= uniforms.get_size());
	int idx = 0;
	for_every_const(param, _set.uniforms)
	{
		apply(idx, *param, _force);
		++idx;
	}
}

void ShaderProgram::set_uniform(Name const & _name, ShaderParam const & _param, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _param, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, ShaderParam const & _param, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	apply(_index, _param, _force);
}

void ShaderProgram::set_uniform(Name const & _name, TextureID _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, TextureID _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	// uniform is set at the creation time (check if this uniform is related to textureSlot)
	an_assert(uniforms[_index].textureSlot != NONE);
	// always set texture
	mark_use_texture_slot(uniforms[_index].textureSlot, _val);
	uniformValue.set_uniform(_val, _force);
}

void ShaderProgram::set_uniform(Name const & _name, int32 _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, int32 _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform1i;
	if (_force ||
		uniformValue.type != paramType ||
		uniformValue.valueI[0] != _val)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1i(uniforms[_index].id, _val); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Array<int32> const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Array<int32> const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform1iArray;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == _val.get_data_size();
		if (allSame)
		{
			void const *uv = uniformValue.get_uniform_data();
			allSame &= memory_compare(uv, _val.get_data(), uvDataSize);
		}
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1iv(uniforms[_index].id, _val.get_data_size() / sizeof(int32), (GLint const*)_val.get_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, float _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, float _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform1f;
	if (_force ||
		uniformValue.type != paramType ||
		uniformValue.valueF[0] != _val)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1f(uniforms[_index].id, _val); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Array<float> const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Array<float> const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform1fArray;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == _val.get_data_size();
		if (allSame)
		{
			void const *uv = uniformValue.get_uniform_data();
			allSame &= memory_compare(uv, _val.get_data(), uvDataSize);
		}
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform1fv(uniforms[_index].id, _val.get_data_size() / sizeof(float), (float const*)_val.get_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector2 const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector2 const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform2f;
	if (_force ||
		uniformValue.type != paramType ||
		uniformValue.valueF[0] != _val.x ||
		uniformValue.valueF[1] != _val.y)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform2f(uniforms[_index].id, _val.x, _val.y); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Array<Vector2> const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Array<Vector2> const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformVectors2f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == _val.get_data_size();
		if (allSame)
		{
			void const *uv = uniformValue.get_uniform_data();
			allSame &= memory_compare(uv, _val.get_data(), uvDataSize);
		}
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform2fv(uniforms[_index].id, _val.get_data_size() / sizeof(Vector2), (float const*)_val.get_data()); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector2 const * _val, int _count, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _count, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector2 const * _val, int _count, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformVectors2f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == sizeof(Vector2) * _count;
		if (allSame)
		{
			void const *uv = uniformValue.get_uniform_data();
			allSame &= memory_compare(uv, _val, uvDataSize);
		}
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform2fv(uniforms[_index].id, _count, (float const*)_val); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _count, _force);
	}
}

void ShaderProgram::set_uniform(int _index, Vector3 const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform3f;
	if (_force ||
		uniformValue.type != paramType ||
		uniformValue.valueF[0] != _val.x ||
		uniformValue.valueF[1] != _val.y ||
		uniformValue.valueF[2] != _val.z)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform3f(uniforms[_index].id, _val.x, _val.y, _val.z); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector3 const * _val, int _count, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _count, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector3 const * _val, int _count, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformVectors3f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == sizeof(Vector3) * _count;
		if (allSame)
		{
			void const *uv = uniformValue.get_uniform_data();
			allSame &= memory_compare(uv, _val, uvDataSize);
		}
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform3fv(uniforms[_index].id, _count, (float const*)_val); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _count, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector3 const & _v0, Vector3 const & _v1, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _v0, _v1, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector3 const & _v0, Vector3 const & _v1, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformVectorsStereo3f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		allSame &= memory_compare(&uniformValue.valueF[0], &_v0.x, sizeof(Vector3)) &&
				   memory_compare(&uniformValue.valueF[3], &_v1.x, sizeof(Vector3));
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			Vector3 const vs[] = { _v0, _v1 };
			DIRECT_GL_CALL_ glUniform3fv(uniforms[_index].id, 2, &vs->x); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_v0, _v1, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector4 const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector4 const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform4f;
	if (_force ||
		uniformValue.type != paramType ||
		uniformValue.valueF[0] != _val.x ||
		uniformValue.valueF[1] != _val.y ||
		uniformValue.valueF[2] != _val.z ||
		uniformValue.valueF[3] != _val.w)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4f(uniforms[_index].id, _val.x, _val.y, _val.z, _val.w); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector4 const * _v, int32 _count, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _v, _count, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector4 const * _v, int32 _count, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformVectorsPointer4f;
	bool allSame = !_force;
	allSame &= (uniformValue.type == ShaderParamType::uniformVectors4f || uniformValue.type == ShaderParamType::uniformVectorsPointer4f) &&
			   (paramType == ShaderParamType::uniformVectors4f || paramType == ShaderParamType::uniformVectorsPointer4f);
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == sizeof(Vector4) * _count;
		if (allSame)
		{
			void const *uv = uniformValue.type == ShaderParamType::uniformVectors4f ? uniformValue.get_uniform_data() : uniformValue.valueDptr;
			allSame &= memory_compare(uv, &_v->x, uvDataSize);
		}
	}
	if (!allSame)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4fv(uniforms[_index].id, _count, &_v->x); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_v, _count, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Vector4 const & _v0, Vector4 const & _v1, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _v0, _v1, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Vector4 const & _v0, Vector4 const & _v1, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformVectorsStereo4f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		allSame &= memory_compare(&uniformValue.valueF[0], &_v0.x, sizeof(Vector4)) &&
				   memory_compare(&uniformValue.valueF[4], &_v1.x, sizeof(Vector4));
	}
	if (!allSame)
	{
		Vector4 const vs[] = { _v0, _v1 };
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4fv(uniforms[_index].id, 2, &vs->x); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_v0, _v1, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, VectorInt4 const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, VectorInt4 const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniform4i;
	if (_force ||
		uniformValue.type != paramType ||
		uniformValue.valueI[0] != _val.x ||
		uniformValue.valueI[1] != _val.y ||
		uniformValue.valueI[2] != _val.z ||
		uniformValue.valueI[3] != _val.w)
	{
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniform4i(uniforms[_index].id, _val.x, _val.y, _val.z, _val.w); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Matrix33 const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Matrix33 const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatrix3f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		allSame &= memory_compare(uniformValue.valueF, &_val.m00, sizeof(Matrix33));
	}
	if (!allSame)
	{
		// turn row-major to column-major (transpose)
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix3fv(uniforms[_index].id, 1, GL_FALSE, &_val.m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Array<Matrix33> const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Array<Matrix33> const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatrices3f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == _val.get_data_size();
		if (allSame)
		{
			void const *uv = uniformValue.type == ShaderParamType::uniformMatrices3f ? uniformValue.get_uniform_data() : uniformValue.valueDptr;
			allSame &= memory_compare(uv, _val.get_data(), uvDataSize);
		}
	}
	if (!allSame)
	{
		// turn row-major to column-major (transpose)
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix3fv(uniforms[_index].id, _val.get_size(), GL_FALSE, &_val.get_data()->m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Matrix33 const * _v, int32 _count, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _v, _count, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Matrix33 const * _v, int32 _count, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatricesPointer3f;
	bool allSame = !_force;
	allSame &= (uniformValue.type == ShaderParamType::uniformMatrices3f || uniformValue.type == ShaderParamType::uniformMatricesPointer3f) &&
			   (paramType == ShaderParamType::uniformMatrices3f || paramType == ShaderParamType::uniformMatricesPointer3f);
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == sizeof(Matrix33) * _count;
		if (allSame)
		{
			void const *uv = uniformValue.type == ShaderParamType::uniformMatrices3f ? uniformValue.get_uniform_data() : uniformValue.valueDptr;
			allSame &= memory_compare(uv, &_v->m00, uvDataSize);
		}
	}
	if (!allSame)
	{
		// turn row-major to column-major (transpose)
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix3fv(uniforms[_index].id, _count, GL_FALSE, &_v->m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_v, _count, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Matrix44 const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Matrix44 const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatrix4f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		allSame &= memory_compare(uniformValue.valueF, &_val.m00, sizeof(Matrix44));
	}
	if (!allSame)
	{
		// turn row-major to column-major (transpose)
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix4fv(uniforms[_index].id, 1, GL_FALSE, &_val.m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Array<Matrix44> const & _val, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _val, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Array<Matrix44> const & _val, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatrices4f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == _val.get_data_size();
		if (allSame)
		{
			void const *uv = uniformValue.type == ShaderParamType::uniformMatrices4f ? uniformValue.get_uniform_data() : uniformValue.valueDptr;
			allSame &= memory_compare(uv, _val.get_data(), uvDataSize);
		}
	}
	if (!allSame)
	{
		// turn row-major to column-major (transpose)
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix4fv(uniforms[_index].id, _val.get_size(), GL_FALSE, &_val.get_data()->m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_val, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _v, _count, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Matrix44 const * _v, int32 _count, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatricesPointer4f;
	bool allSame = !_force;
	allSame &= (uniformValue.type == ShaderParamType::uniformMatrices4f || uniformValue.type == ShaderParamType::uniformMatricesPointer4f) &&
			   (paramType == ShaderParamType::uniformMatrices4f || paramType == ShaderParamType::uniformMatricesPointer4f);
	if (allSame)
	{
		int32 uvDataSize = uniformValue.valueDsize;
		allSame &= uvDataSize == sizeof(Matrix44) * _count;
		if (allSame)
		{
			void const *uv = uniformValue.type == ShaderParamType::uniformMatrices4f ? uniformValue.get_uniform_data() : uniformValue.valueDptr;
			allSame &= memory_compare(uv, &_v->m00, uvDataSize);
		}
	}
	if (!allSame)
	{
		// turn row-major to column-major (transpose)
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix4fv(uniforms[_index].id, _count, GL_FALSE, &_v->m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_v, _count, _force);
	}
}

void ShaderProgram::set_uniform(Name const & _name, Matrix44 const & _v0, Matrix44 const & _v1, bool _force)
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		set_uniform(*idx, _v0, _v1, _force);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _name.to_char());
#endif
	}
}

void ShaderProgram::set_uniform(int _index, Matrix44 const & _v0, Matrix44 const & _v1, bool _force)
{
	if (_index == NONE)
	{
		return;
	}
	assert_rendering_thread();
	currentValues.uniforms.set_size_to_have(_index);
	ShaderParam & uniformValue = currentValues.uniforms[_index];
	ShaderParamType::Type const paramType = ShaderParamType::uniformMatricesStereo4f;
	bool allSame = !_force;
	allSame &= uniformValue.type == paramType;
	if (allSame)
	{
		allSame &= memory_compare(&uniformValue.valueF[0], &_v0.m00, sizeof(Matrix44)) &&
				   memory_compare(&uniformValue.valueF[16], &_v1.m00, sizeof(Matrix44));
	}
	if (!allSame)
	{
		Matrix44 const vs[] = { _v0, _v1 };
		if (uniforms[_index].id != NONE)
		{
			DIRECT_GL_CALL_ glUniformMatrix4fv(uniforms[_index].id, 2, GL_FALSE, &vs->m00); AN_GL_CHECK_FOR_ERROR
		}
		uniformValue.set_uniform(_v0, _v1, _force);
	}
}

void ShaderProgram::mark_use_texture_slot(Name const & _uniformName, TextureID _textureID)
{
	if (auto idx = textureUniformMap.get_existing(_uniformName))
	{
		mark_use_texture_slot(*idx, _textureID);
	}
	else
	{
#ifdef WARN_ABOUT_MISSING_PARAMS
		warn(TXT("could not find parameter \"%S\""), _uniformName.to_char());
#endif
	}
}

void ShaderProgram::mark_use_texture_slot(int _textureSlotIndex, TextureID _textureID)
{
	if (!textureSlots.is_index_valid(_textureSlotIndex))
	{
		return;
	}
	ShaderTextureSlot const & textureSlot = textureSlots[_textureSlotIndex];
	// make sure texture slot has bound sampler with same number as slot
	an_assert((currentValues.uniforms[textureSlot.uniformIndex].type == ShaderParamType::uniform1i ||
			currentValues.uniforms[textureSlot.uniformIndex].type == ShaderParamType::texture ||
			currentValues.uniforms[textureSlot.uniformIndex].type == ShaderParamType::notSet) &&
		   currentValues.uniforms[textureSlot.uniformIndex].valueI[0] == _textureSlotIndex);
	// this is commented as texture slots should not change and assert above should catch that
	// set_uniform(textureSlot.uniformIndex, (int32)textureSlot.textureSlot);
	// video3d manages switching textures
	video3d->mark_use_texture(textureSlot.textureSampler, _textureID);
}

int ShaderProgram::find_attribute_index(Name const & _name) const
{
	if (auto idx = attributeMap.get_existing(_name))
	{
		return *idx;
	}
	else
	{
		return NONE;
	}
}

int ShaderProgram::find_uniform_index(Name const & _name) const
{
	if (auto idx = uniformMap.get_existing(_name))
	{
		return *idx;
	}
	else
	{
		return NONE;
	}
}

Name const & ShaderProgram::get_uniform_name(int _uniformIndex) const
{
	return uniforms.is_index_valid(_uniformIndex) ? uniforms[_uniformIndex].name : Name::invalid();
}

ShaderParamID ShaderProgram::get_attribute_shader_param_id(int _attributeIndex) const
{
	return attributes.is_index_valid(_attributeIndex) ? attributes[_attributeIndex].id : NONE;
}

ShaderParamID ShaderProgram::get_uniform_shader_param_id(int _uniformIndex) const
{
	return uniforms.is_index_valid(_uniformIndex) ? uniforms[_uniformIndex].id : NONE;
}

void ShaderProgram::bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Vector2 const & _val)
{
	if (attributes.is_index_valid(_attribIndex))
	{
		ShaderParamInfo const & attribute = attributes[_attribIndex];
		_v3d->bind_vertex_attrib(attribute.id, _val);
	}
}

void ShaderProgram::bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Vector3 const & _val)
{
	if (attributes.is_index_valid(_attribIndex))
	{
		ShaderParamInfo const & attribute = attributes[_attribIndex];
		_v3d->bind_vertex_attrib(attribute.id, _val);
	}
}

void ShaderProgram::bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Vector4 const & _val)
{
	if (attributes.is_index_valid(_attribIndex))
	{
		ShaderParamInfo const & attribute = attributes[_attribIndex];
		_v3d->bind_vertex_attrib(attribute.id, _val);
	}
}

void ShaderProgram::bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Colour const & _val)
{
	if (attributes.is_index_valid(_attribIndex))
	{
		ShaderParamInfo const & attribute = attributes[_attribIndex];
		_v3d->bind_vertex_attrib(attribute.id, _val);
	}
}

void ShaderProgram::bind_vertex_attrib_array(Video3D* _v3d, int _attribIndex, int _elementCount, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _stride, void* _pointer)
{
	if (attributes.is_index_valid(_attribIndex))
	{
		ShaderParamInfo const & attribute = attributes[_attribIndex];
		_v3d->bind_vertex_attrib_array(attribute.id, _elementCount, _dataType, _attribType, _stride, _pointer);
	}
}

void ShaderProgram::add_forced_uniforms_from(SimpleVariableStorage const& _from)
{
	for_every(simpleVariableInfo, _from.get_all())
	{
		Name uniformName = simpleVariableInfo->get_name();
		int uniformIndex = find_uniform_index(uniformName);
		if (uniformIndex == NONE)
		{
			uniformIndex = uniforms.get_size();
			uniforms.push_back(ShaderParamInfo(uniformName, NONE));
			uniformMap[uniformName] = uniformIndex;
		}
	}
}

#undef USE_OFFSET
