#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"
#include "..\..\containers\map.h"

#include "video.h"
#include "shader.h"
#include "shaderProgramInstance.h"
#include "video3d.h"

/*
 *	Shaders!
 *
 *	Here is list of default uniforms and attributes for version 130 and up:
 *
 *	(those attributes and uniforms ids are stored for all shader program for easy access)
 *
 *	<fw>	framework level (not core), but kept here to have everything in one place
 *
 *	COMMON
 *		input
 *			uniform float random - 0 to 1
 *			uniform float time - going from 0 with delta time
 *
 *	VERTEX SHADER
 *		input
 *			uniform vec3 modelViewOffsets[2]
 *			uniform mat3 normalMatrix /ces[2] - as model view matrix - just rotation part
 *			uniform mat3 normalVRMatrix /ces[2] - vr - just rotation part
 *			uniform mat4 modelViewMatrix - to change points from model space to camera space
 *			uniform mat4 projectionMatrix /ces[2] - projection matrix
 *			uniform vec4 modelViewClipPlanes[6 or more] - clip planes in camera space
 *			uniform int modelViewClipPlanesCount - count of clip planes used
 *
 *			in vec3 inAPosition - vertex position (always - check thingsThatIDontUnderstand.txt [2])
 *			in vec3 inONormal - vertex normal (optional)
 *			in vec3 inOColour - vertex colour (optional)
 *			in vec2 inOUV - texture coord (optional)
 *			in vec2 inViewRay - view ray - for post process
 *
 *		input (optional for skinning)
 *			to multiple bones (max 4)
 *			in vec4 inOSkinningWeights
 *			in ivec4 inOSkinningIndices
 *			to single bone
 *			in int inOSkinningIndex
 *
 *		processing parameters for process() and post_skinning_process()
 *	<fw>	vec4 prcPosition - vertex position for process()
 *	<fw>	vec4 prcNormal - vertex normal for process()
 *	<fw>	vec4 prcColour - vertex colour for process()
 *	<fw>	vec2 prcUV - texture coord for process()
 *
 *		output
 *	<fw>	out vec4 varPosition - position in camera space (!)
 *	<fw>	out vec4 varNormal - normal in camera space (!)
 *	<fw>	out vec4 varColour
 *	<fw>	out vec2 varUV
 *	<fw>	out vec2 varViewRay
 *	<fw>	out vec3 varLightUsage
 *
 *	FRAGMENT SHADER
 *		input
 *	<fw>	uniform mat4 projectionMatrix3D - to change points from camera space to projection space (bound by post process)
 *
 *			uniform sampler inTexture - default texture
 *			uniform vec2 inTextureTexelSize - mostly used for post process
 *			uniform vec2 inTextureSize - mostly used for post process
 *			uniform vec2 inTextureAspectRatio - mostly used for post process
 *
 *	<fw>	uniform sampler lightModulateTexture
 *	<fw>	uniform sampler lightReplaceTexture
 *	<fw>	uniform sampler lightFresnelTexture
 *	<fw>	uniform float lightModulateStrength
 *	<fw>	uniform float lightReplaceStrength
 *	<fw>	uniform float lightFresnelStrength
 *	<fw>	uniform vec3 lightDir
 *
 *	<fw>	uniform vec3 distanceModColourOntoLight
 *	<fw>	uniform vec3 distanceModColourAwayLight
 *	<fw>	uniform float distanceModCoef
 *
 *	<fw>	in vec4 varPosition
 *	<fw>	in vec4 varNormal
 *	<fw>	in vec4 varColour
 *	<fw>	in vec2 varUV
 *	<fw>	in vec3 varLightUsage
 *
 *		output
 *			out vec4 colour (bound to first texture)
 *
 *
 *
 *	ShaderParam
 *
 *	For textures:
 *		Uniform value in actual shader is set when shader is bound
 *		Then in shader param there is only texture ID that is set to appropriate texture
 *		When unbinding shader, textures are set to none
 *
 */

namespace System
{
	struct ShaderProgramInstance;
	class ShaderProgramBindingContext;

	struct ShaderProgramSetup
	{
		String shaderProgramCacheId;
		RefCountObjectPtr<Shader> vertexShader;
		RefCountObjectPtr<Shader> fragmentShader;

		static ShaderProgramSetup create(Shader * _vertexShader, Shader * _fragmentShader, String const & _shaderProgramCacheId = String::empty()) { ShaderProgramSetup sps; sps.vertexShader = _vertexShader; sps.fragmentShader = _fragmentShader; sps.shaderProgramCacheId = _shaderProgramCacheId; return sps; }

		ShaderProgramSetup() {}
	};

	struct ShaderParamInfo
	{
		Name name;
		ShaderParamID id; // if NONE it is forced, won't go to the GPU but can be exchanged here
		int32 textureSlot;

		ShaderParamInfo(Name const & _name, ShaderParamID _id);
	};

	struct ShaderTextureSlot
	{
		// by default uniform with given name has bound sampler at textureSlot, therefore there should be no need to set uniform for texture
		int textureSampler; // 
		Name uniformName;
		int uniformIndex;

		ShaderTextureSlot(int _textureSampler, Name const & _uniformName, int _uniformIndex);
	};

	/*
	 *	Will auto delete on Video3d closing.
	 */
	class ShaderProgram
	: public RefCountObject
	{
	public:
		ShaderProgram(Shader * _vertexShader, Shader * _fragmentShader);
		ShaderProgram(ShaderProgramSetup const & _setup);
		static ShaderProgram* create_stepped(); // follow stepped_creation__*

	public:
		// these steps are part of init or should be called explicitly if created via create_stepped
		void stepped_creation__create(ShaderProgramSetup const& _setup);
		void stepped_creation__load_attribs();
		void stepped_creation__load_uniforms();
		void stepped_creation__finalise();

	public:
		void init(ShaderProgramSetup const & _setup);
		void close();

		bool has_shader_compilation_or_linking_failed() const { return shaderCompilationOrLinkingFailed; }

		void bind(bool _setCustomBuildInUniformsToo = true); // build in uniforms are set here, but if custom require to be set by hand, inform with parameter (as they may rely on some other params set)
		void unbind();

		ShaderProgramInstance & access_default_values() { return defaultValues; }
		ShaderProgramInstance const & get_default_values() const { return defaultValues; }
		ShaderProgramInstance const & get_current_values() const { return currentValues; }

		void use_default_textures_for_missing(ShaderProgramBindingContext const & _bindingContext); // because some textures might be not set at all - fill with default texture
		void use_for_all_textures(TextureID const & _textureID);

		inline Shader const * get_vertex_shader()  const { return vertexShader.get(); }
		inline Shader const * get_fragment_shader() const { return fragmentShader.get(); }

		inline ShaderProgramID get_shader_program_id() const { return program; }

		void set_custom_build_in_uniforms();
		void set_build_in_uniforms(bool _setCustomBuildInUniformsToo = true, bool _eyeRelatedOnly = false);
		void set_build_in_uniform_in_texture(TextureID const & _textureID);
		void set_build_in_uniform_in_texture_size_related_uniforms(Vector2 const & _inTextureSize, Vector2 const & _outSize, Optional<float> const & _fov = NP);

		void apply(ShaderProgramInstance const& _set, ShaderProgramBindingContext const& _bindingContext, bool _force = false);
		void apply(ShaderProgramBindingContext const& _bindingContext, bool _force = false);
		void apply(ShaderProgramInstance const & _set, bool _force = false);
		void apply(Name const & _name, ShaderParam const & _param, bool _force = false);
		void apply(int _idx, ShaderParam const & _param, bool _force = false);

		void set_uniform(Name const & _name, ShaderParam const & _param, bool _force = false);
		//
		void set_uniform(Name const & _name, TextureID _val, bool _force = false);
		void set_uniform(Name const & _name, int32 _val, bool _force = false);
		void set_uniform(Name const & _name, Array<int32> const & _val, bool _force = false);
		void set_uniform(Name const & _name, float _val, bool _force = false);
		void set_uniform(Name const & _name, Array<float> const & _val, bool _force = false);
		void set_uniform(Name const & _name, Vector2 const & _val, bool _force = false);
		void set_uniform(Name const & _name, Array<Vector2> const & _val, bool _force = false);
		void set_uniform(Name const & _name, Vector2 const * _val, int _count, bool _force = false);
		void set_uniform(Name const & _name, Vector3 const & _val, bool _force = false);
		void set_uniform(Name const & _name, Vector3 const * _val, int _count, bool _force = false);
		void set_uniform(Name const & _name, Vector3 const & _v0, Vector3 const & _v1, bool _force = false);
		void set_uniform(Name const & _name, Vector4 const & _val, bool _force = false);
		void set_uniform(Name const & _name, Vector4 const * _v, int32 _count, bool _force = false);
		void set_uniform(Name const & _name, Vector4 const & _v0, Vector4 const & _v1, bool _force = false);
		void set_uniform(Name const & _name, VectorInt4 const & _val, bool _force = false);
		void set_uniform(Name const & _name, Matrix33 const & _val, bool _force = false);
		void set_uniform(Name const & _name, Array<Matrix33> const & _val, bool _force = false);
		void set_uniform(Name const & _name, Matrix33 const * _v, int32 _count, bool _force = false);
		void set_uniform(Name const & _name, Matrix44 const & _val, bool _force = false);
		void set_uniform(Name const & _name, Array<Matrix44> const & _val, bool _force = false);
		void set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, bool _force = false);
		void set_uniform(Name const & _name, Matrix44 const & _v0, Matrix44 const & _v1, bool _force = false);

		void set_uniform(int _index, ShaderParam const & _param, bool _force = false);
		//
		void set_uniform(int _index, TextureID _val, bool _force = false);
		void set_uniform(int _index, int32 _val, bool _force = false);
		void set_uniform(int _index, Array<int32> const & _val, bool _force = false);
		void set_uniform(int _index, float _val, bool _force = false);
		void set_uniform(int _index, Array<float> const & _val, bool _force = false);
		void set_uniform(int _index, Vector2 const & _val, bool _force = false);
		void set_uniform(int _index, Array<Vector2> const & _val, bool _force = false);
		void set_uniform(int _index, Vector2 const * _val, int _count, bool _force = false);
		void set_uniform(int _index, Vector3 const & _val, bool _force = false);
		void set_uniform(int _index, Vector3 const * _val, int _count, bool _force = false);
		void set_uniform(int _index, Vector3 const & _v0, Vector3 const & _v1, bool _force = false);
		void set_uniform(int _index, Vector4 const & _val, bool _force = false);
		void set_uniform(int _index, Vector4 const * _v, int32 _count, bool _force = false);
		void set_uniform(int _index, Vector4 const & _v0, Vector4 const & _v1, bool _force = false);
		void set_uniform(int _index, VectorInt4 const & _val, bool _force = false);
		void set_uniform(int _index, Matrix33 const & _val, bool _force = false);
		void set_uniform(int _index, Array<Matrix33> const & _val, bool _force = false);
		void set_uniform(int _index, Matrix33 const * _v, int32 _count, bool _force = false);
		void set_uniform(int _index, Matrix44 const & _val, bool _force = false);
		void set_uniform(int _index, Array<Matrix44> const & _val, bool _force = false);
		void set_uniform(int _index, Matrix44 const * _v, int32 _count, bool _force = false);
		void set_uniform(int _index, Matrix44 const & _v0, Matrix44 const & _v1, bool _force = false);

		// these are used only to bind stuff through Video3D, they do not change OpenGL set immediately!
		void bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Vector2 const & _val);
		void bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Vector3 const & _val);
		void bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Vector4 const & _val);
		void bind_vertex_attrib(Video3D* _v3d, int _attribIndex, Colour const & _val);
		void bind_vertex_attrib_array(Video3D* _v3d, int _attribIndex, int _elementCount, DataFormatValueType::Type _dataType, VertexFormatAttribType::Type _attribType, int _stride, void* _pointer);

		// returns index from attributes or uniforms array
		int find_attribute_index(Name const & _name) const;
		int find_uniform_index(Name const & _name) const;
		Name const & get_uniform_name(int _uniformIndex) const;

		void add_forced_uniforms_from(SimpleVariableStorage const&); // add uniforms (if not there, they won't be set to actual shader but will be handled over to other things)

		ShaderParamID get_attribute_shader_param_id(int _attributeIndex) const;
		ShaderParamID get_uniform_shader_param_id(int _uniformIndex) const;

		// "build-in"
		inline int get_in_position_attribute_index() const { return inPositionAttributeIndex; }
		inline int get_in_normal_attribute_index() const { return inNormalAttributeIndex; }
		inline int get_in_colour_attribute_index() const { return inColourAttributeIndex; }
		inline int get_in_uv_attribute_index() const { return inUVAttributeIndex; }
		inline int get_in_texture_uniform_index() const { return inTextureUniformIndex; }
		inline int get_in_texture_texel_size_uniform_index() const { return inTextureTexelSizeUniformIndex; }
		inline int get_in_texture_size_uniform_index() const { return inTextureSizeUniformIndex; }
		inline int get_in_texture_aspect_ratio_uniform_index() const { return inTextureAspectRatioUniformIndex; }
		inline int get_in_output_texel_size_uniform_index() const { return inOutputTexelSizeUniformIndex; }
		inline int get_in_output_size_uniform_index() const { return inOutputSizeUniformIndex; }
		inline int get_in_output_texel_size_foved_uniform_index() const { return inOutputTexelSizeFovedUniformIndex; }
		inline int get_in_output_size_foved_uniform_index() const { return inOutputSizeFovedUniformIndex; }
		inline int get_in_output_aspect_ratio_uniform_index() const { return inOutputAspectRatioUniformIndex; }
		inline bool does_support_skinning() const { return inSkinningBonesUniformIndex != NONE || inSkinningWeightsAttributeIndex != NONE || inSkinningIndicesAttributeIndex != NONE || inSkinningIndexAttributeIndex != NONE; }
		inline bool does_support_skinning_to_single_bone() const { return inSkinningIndexAttributeIndex != NONE; }
		inline int get_in_skinning_weights_attribute_index() const { return inSkinningWeightsAttributeIndex; }
		inline int get_in_skinning_indices_attribute_index() const { return inSkinningIndicesAttributeIndex; }
		inline int get_in_skinning_index_attribute_index() const { return inSkinningIndexAttributeIndex; }
		inline int get_in_skinning_bones_uniform_index() const { return inSkinningBonesUniformIndex; }

	protected: friend class Video3D;
		virtual ~ShaderProgram();

	private:
		Video3D* video3d; // related to this video3d (and added there)
		ShaderProgramID program = 0;
		ShaderID vertexShaderID = 0;
		ShaderID fragmentShaderID = 0;
		bool shadersAttached = false;
		bool shaderCompilationOrLinkingFailed = false;
		RefCountObjectPtr<Shader> vertexShader;
		RefCountObjectPtr<Shader> fragmentShader;
		Array<ShaderParamInfo> attributes;
		Array<ShaderParamInfo> uniforms;
		Array<ShaderTextureSlot> textureSlots;
		Map<Name, int> attributeMap; // index point in attributes array
		Map<Name, int> uniformMap; // index point in uniform array
		Map<Name, int> textureUniformMap; // index point in texture slot array

		int inPositionAttributeIndex = NONE;
		int inNormalAttributeIndex = NONE;
		int inColourAttributeIndex = NONE;
		int inUVAttributeIndex = NONE;
		int inViewRayAttributeIndex = NONE;
		int inTextureUniformIndex = NONE;
		int inTextureTexelSizeUniformIndex = NONE;
		int inTextureSizeUniformIndex = NONE;
		int inOutputTexelSizeUniformIndex = NONE;
		int inOutputSizeUniformIndex = NONE;
		int inOutputTexelSizeFovedUniformIndex = NONE;
		int inOutputSizeFovedUniformIndex = NONE;
		int inOutputAspectRatioUniformIndex = NONE;
		int inTextureAspectRatioUniformIndex = NONE;
		int inSkinningBonesUniformIndex = NONE;

		int eyeOffsetIndex = NONE;
		int randomIndex = NONE;
		int timeIndex = NONE;
		int timePendulumIndex = NONE;
		int individualIndex = NONE;
		int normalMatrixUniformIndex = NONE;
		int normalVRMatrixUniformIndex = NONE;
		int modelViewMatrixUniformIndex = NONE;
		int projectionMatrixUniformIndex = NONE;
		int modelViewClipPlanesUniformIndex = NONE;
		int modelViewClipPlanesCountUniformIndex = NONE;

		int inSkinningWeightsAttributeIndex = NONE;
		int inSkinningIndicesAttributeIndex = NONE;
		int inSkinningIndexAttributeIndex = NONE;

		ShaderProgramInstance defaultValues;
		ShaderProgramInstance currentValues;

		friend struct ShaderProgramInstance;

		ShaderProgram();

		inline bool is_build_in_uniform(Name const & _shaderParam) const;
		inline bool is_automatic_build_in_uniform(Name const & _shaderParam) const;

		void mark_use_texture_slot(Name const & _uniformName, TextureID _textureID);
		//
		void mark_use_texture_slot(int _textureSlotIndex, TextureID _textureID);

		inline void apply_non_build_in(int _idx, ShaderParamInfo const & _uniform, ShaderParam const& _param, bool _force = false);
		inline void apply_value(ShaderParamInfo const& uniform, ShaderParam& currentValue, ShaderParam const& newValue, bool _forced = false);

	};

};

 