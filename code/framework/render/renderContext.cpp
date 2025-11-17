#include "renderContext.h"

#include "roomProxy.h"

#include "..\..\core\system\video\video3d.h"
#include "..\..\core\mesh\mesh3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace Framework::Render;

//

ContextDebug* ContextDebug::use = nullptr;

//

::System::VertexFormat * Context::s_planeVertexFormat = nullptr;

void Context::initialise_static()
{
	s_planeVertexFormat = new ::System::VertexFormat();
	s_planeVertexFormat->no_padding().calculate_stride_and_offsets();
}

void Context::close_static()
{
	delete_and_clear(s_planeVertexFormat);
}

Context::Context(System::Video3D* _v3d)
: video3d(_v3d)
, defaultTextureID(::System::texture_id_none())
{
}

Context::~Context()
{
}

void Context::set_mesh_rendering_context(::Meshes::Mesh3DRenderingContext const & _meshRenderingContext)
{
	meshRenderingContext = _meshRenderingContext;
	meshRenderingContext.shaderProgramBindingContext = &shaderProgramBindingContext;
}

void Context::set_shader_program_binding_context(::System::ShaderProgramBindingContext const & _shaderProgramBindingContext)
{
	shaderProgramBindingContext = _shaderProgramBindingContext;
	shaderProgramBindingContext.set_default_texture_id(defaultTextureID);
	meshRenderingContext.shaderProgramBindingContext = &shaderProgramBindingContext;
}

void Context::set_default_texture_id(::System::TextureID const & _textureID)
{
	defaultTextureID = _textureID;
	shaderProgramBindingContext.set_default_texture_id(defaultTextureID);
}

void Context::setup_far_plane(float _fov, float _aspectRatio, float _farPlane)
{
	internal_setup_far_plane(farPlaneValues, _fov, _aspectRatio, _farPlane);
}

void Context::setup_background_far_plane(float _fov, float _aspectRatio, float _farPlane)
{
	internal_setup_far_plane(backgroundFarPlaneValues, _fov, _aspectRatio, _farPlane);
}

void Context::internal_setup_far_plane(Values & _values, float _fov, float _aspectRatio, float _farPlane)
{
	_values.fov = _fov;
	_values.farPlane = _farPlane;
	float const farPlane = _farPlane;
	float const height = farPlane * tan_deg(_fov / 2.0f) * 1.5f; // a little bit bigger to go beyond the border, this should cover view centre offsets
	_values.farPlaneValues = Vector3(height * _aspectRatio,
#ifdef USE_DEPTH_CLAMP
		farPlane, // right at the far plane, we'll be clamping
#else
#ifdef USE_DEPTH_RANGE
		farPlane * 0.9f, // doesn't really matter, we're be depth range "clamped"
#else
		farPlane * 0.995f, // a bit before far plane
#endif
#endif
		height);
}

void Context::render_far_plane(Optional<Range3> const& _onlyForFlatRange, ::System::MaterialInstance const* _usingMaterial)
{
	internal_render_far_plane(farPlaneValues, _onlyForFlatRange, _usingMaterial);
}

void Context::render_background_far_plane(Optional<Range3> const& _onlyForFlatRange, ::System::MaterialInstance const* _usingMaterial)
{
	internal_render_far_plane(backgroundFarPlaneValues, _onlyForFlatRange, _usingMaterial);
}

void Context::internal_render_far_plane(Values const & _values, Optional<Range3> const& _onlyForFlatRange, ::System::MaterialInstance const* _usingMaterial)
{
	auto fpv = _values.farPlaneValues;
	Vector3 farPlanePoints[4];
	Range3 fpvXZ(Range(-fpv.x, fpv.x), Range(fpv.y), Range(-fpv.z, fpv.z));
	if (_onlyForFlatRange.is_set() && !_onlyForFlatRange.get().is_empty())
	{
		// we're running with flat data, we can just scale to the right distance and intersect
		float atY = fpv.y;
		Range3 forRange = _onlyForFlatRange.get();
		float scale = atY; // / 1.0f (flat!)
		forRange.x *= scale;
		forRange.z *= scale;
		fpvXZ.x.min = max(fpvXZ.x.min, forRange.x.min);
		fpvXZ.x.max = min(fpvXZ.x.max, forRange.x.max);
		fpvXZ.z.min = max(fpvXZ.z.min, forRange.z.min);
		fpvXZ.z.max = min(fpvXZ.z.max, forRange.z.max);
	}

	farPlanePoints[0] = Vector3(fpvXZ.x.min, fpv.y, fpvXZ.z.min);
	farPlanePoints[1] = Vector3(fpvXZ.x.min, fpv.y, fpvXZ.z.max);
	farPlanePoints[2] = Vector3(fpvXZ.x.max, fpv.y, fpvXZ.z.max);
	farPlanePoints[3] = Vector3(fpvXZ.x.max, fpv.y, fpvXZ.z.min);

	::System::ShaderProgramInstance const* shaderProgramInstance = nullptr;
	::System::Material const* material = nullptr;
	if (_usingMaterial)
	{
		shaderProgramInstance = _usingMaterial->get_shader_program_instance();
		material = _usingMaterial->get_material();
	}

	Meshes::Mesh3D::render_data(video3d, shaderProgramInstance, material, ::Meshes::Mesh3DRenderingContext::none(), farPlanePoints, ::Meshes::Primitive::TriangleFan, 2, Context::plane_vertex_format());
}