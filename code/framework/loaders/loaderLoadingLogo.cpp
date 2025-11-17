#include "loaderLoadingLogo.h"

#include "..\library\library.h"
#include "..\render\renderCamera.h"
#include "..\render\renderContext.h"
#include "..\render\renderScene.h"

#include "..\..\core\debug\debugRenderer.h"
#include "..\..\core\mainConfig.h"
#include "..\..\core\math\math.h"
#include "..\..\core\splash\splashLogos.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\video\fragmentShaderOutputSetup.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

// this is used to quickly test foveated rendering or just rendering in general
//#define BIG_LOGO
//#define STAY_HERE

//

using namespace Loader;

//

DEFINE_STATIC_NAME(colour);
DEFINE_STATIC_NAME(inData);
DEFINE_STATIC_NAME(logoAlpha);
DEFINE_STATIC_NAME(overrideColour);

//

LoadingLogo::LoadingLogo(bool _onlySplash, Optional<float> const & _showLimit, Optional<bool> const& _waitForVR)
{
	onlySplash = _onlySplash;

	construct_shaders();

	update_starting_yaw_if_required();

	construct_logos();

	logoShowDelay = 1.0f;
	showTimeLeft = _showLimit;
	waitForVR = _waitForVR.get(false);
}

LoadingLogo::~LoadingLogo()
{
	logoShaderProgram = nullptr;
	logoVertexShader = nullptr;
	logoFragmentShader = nullptr;
	delete_and_clear(logoMesh);
	delete_and_clear(logoDemoMesh);
	delete_and_clear(logoMaterial);
	delete_and_clear(logoDemoMaterial);
	delete_and_clear(logoTexture);
	delete_and_clear(logoDemoTexture);
	//
	for_every(logoMesh, logoMeshes)
	{
		delete_and_clear(logoMesh->mesh);
		delete_and_clear(logoMesh->material);
	}
	::Splash::Logos::unload_logos(REF_ splashTextures, REF_ otherSplashTextures);
}

void LoadingLogo::setup_static_display()
{
	activation = 1.0f;
	targetActivation = 1.0f;
}

void LoadingLogo::add_point(REF_ Array<int8> & _vertexData, REF_ int & _pointIdx, ::System::VertexFormat const & _vertexFormat, Vector2 const & _point, Vector2 const& _uv)
{
	int8* currentVertexData = &_vertexData[_vertexFormat.get_stride() * _pointIdx];

	Vector3* location = (Vector3*)(currentVertexData + _vertexFormat.get_location_offset());
	*location = Vector3(_point.x, _point.y, 0.0f);

	if (_vertexFormat.get_texture_uv_offset() != NONE)
	{
		::System::VertexFormatUtils::store(currentVertexData + _vertexFormat.get_texture_uv_offset(), _vertexFormat.get_texture_uv_type(), _uv);
	}

	++_pointIdx;
}

void LoadingLogo::add_triangle(REF_ Array<int32> & _elements, REF_ int & _elementIdx, int32 _a, int32 _b, int32 _c)
{
	_elements[_elementIdx++] = _a;
	_elements[_elementIdx++] = _b;
	_elements[_elementIdx++] = _c;
}

void LoadingLogo::construct_logos()
{
	{
		for_every(logoMesh, logoMeshes)
		{
			delete_and_clear(logoMesh->mesh);
		}
		::Splash::Logos::unload_logos(REF_ splashTextures, REF_ otherSplashTextures);

		if (!onlySplash)
		{
			::Splash::Logos::load_logos(REF_ splashTextures, REF_ otherSplashTextures);

			if (!splashTextures.is_empty())
			{
				System::IndexFormat indexFormat;
				indexFormat.with_element_size(System::IndexElementSize::FourBytes);
				indexFormat.calculate_stride();

				System::VertexFormat vertexFormat;
				vertexFormat.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float);
				vertexFormat.with_padding();
				vertexFormat.calculate_stride_and_offsets();

				logoMeshes.set_size(splashTextures.get_size());

				float letterSize = 1.0f;

				float height = 0.0f;
				float spacing = letterSize * 0.5f;
				for_every_reverse_ptr(splashTexture, splashTextures)
				{
					LogoMesh& lm = logoMeshes[for_everys_index(splashTexture)];
					lm.mesh = new Meshes::Mesh3D();

					Array<int32> elements;
					Array<int8> vertexData;

					int pointCount = 4;
					int triangleCount = 2;

					vertexData.set_size(vertexFormat.get_stride() * pointCount);
					elements.set_size(3 * triangleCount);

					Vector2 size = splashTexture->get_size().to_vector2();
					float scale = (letterSize * 3.0f) / size.x;
					size *= scale;

					Vector2 halfSize = size * Vector2::half;

					int pointIdx = 0;
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x, -halfSize.y), Vector2(0.0f, 0.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x,  halfSize.y), Vector2(0.0f, 1.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2( halfSize.x,  halfSize.y), Vector2(1.0f, 1.0f));
					add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2( halfSize.x, -halfSize.y), Vector2(1.0f, 0.0f));

					int elementIdx = 0;
					add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
					add_triangle(REF_ elements, REF_ elementIdx, 0, 2, 3);

					lm.mesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

					lm.material = new ::System::Material();
					lm.material->set_shader(System::MaterialShaderUsage::Static, logoShaderProgram.get());
					lm.material->set_uniform(NAME(inData), splashTexture->get_texture_id());
					lm.material->set_src_blend_op(System::BlendOp::SrcAlpha);
					lm.material->set_dest_blend_op(System::BlendOp::OneMinusSrcAlpha);
					lm.material->allow_individual_instances(true);

					lm.meshInstance.set_placement(Transform(Vector3(0.0f, height + halfSize.y, 0.0f), Quat::identity));
					lm.meshInstance.set_mesh(lm.mesh);
					lm.meshInstance.set_material(0, lm.material);

					height += size.y;
					height += spacing;
				}

				logosHeight = height - spacing;
			}
		}
	}

	struct TextureLoader
	{
		static void load(::System::Texture*& loadIntoTexture, String const& path)
		{
			::System::TextureImportSetup tis;
			tis.ignoreIfNotAbleToLoad = true;
			tis.sourceFile = path;
			tis.forceType = TXT("tga");
			tis.autoMipMaps = true;
			tis.wrapU = System::TextureWrap::clamp;
			tis.wrapV = System::TextureWrap::clamp;
			tis.filteringMag = System::TextureFiltering::linear;
			tis.filteringMin = System::TextureFiltering::linearMipMapLinear;
			::System::Texture* texture = new ::System::Texture();
			if (texture->init_with_setup(tis))
			{
				loadIntoTexture = texture;
			}
			else
			{
				delete texture;
			}
		}
	};
	TextureLoader::load(logoTexture, String::printf(TXT("system%SloadingLogo"), IO::get_directory_separator()));
#ifdef BUILD_DEMO
	TextureLoader::load(logoDemoTexture, String::printf(TXT("system%SloadingLogoDemo"), IO::get_directory_separator()));
#endif
	
	Optional<float> topAt;
	Optional<float> scale;
	Optional<float> logoSpacing;
	for_count(int, i, 2)
	{
		auto*& mesh = i == 0 ? logoMesh : logoDemoMesh;
		auto& meshInstance = i == 0 ? logoMeshInstance : logoDemoMeshInstance;
		auto*& material = i == 0 ? logoMaterial : logoDemoMaterial;
		auto*& texture = i == 0 ? logoTexture : logoDemoTexture;

		if (!texture)
		{
			continue;
		}

		System::IndexFormat indexFormat;
		indexFormat.with_element_size(System::IndexElementSize::FourBytes);
		indexFormat.calculate_stride();

		System::VertexFormat vertexFormat;
		vertexFormat.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float);
		vertexFormat.with_padding();
		vertexFormat.calculate_stride_and_offsets();

		mesh = new Meshes::Mesh3D();

		Array<int32> elements;
		Array<int8> vertexData;

		int pointCount = 4;
		int triangleCount = 2;

		vertexData.set_size(vertexFormat.get_stride() * pointCount);
		elements.set_size(3 * triangleCount);

		Vector2 size = texture->get_size().to_vector2();
		if (!scale.is_set())
		{
			scale = logoHeight / size.y;
		}
		size *= scale.get();

		Vector2 halfSize = size * Vector2::half;

#ifdef BIG_LOGO
		halfSize *= 3.0f;
#endif

		float atY = topAt.is_set() ? topAt.get() - halfSize.y : 0.0f;

		int pointIdx = 0;
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x, -halfSize.y + atY), Vector2(0.0f, 0.0f));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2(-halfSize.x,  halfSize.y + atY), Vector2(0.0f, 1.0f));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2( halfSize.x,  halfSize.y + atY), Vector2(1.0f, 1.0f));
		add_point(REF_ vertexData, REF_ pointIdx, vertexFormat, Vector2( halfSize.x, -halfSize.y + atY), Vector2(1.0f, 0.0f));

		if (!logoSpacing.is_set())
		{
			logoSpacing = 0.1f * size.y;
		}
		topAt = atY - halfSize.y - logoSpacing.get();

		int elementIdx = 0;
		add_triangle(REF_ elements, REF_ elementIdx, 0, 1, 2);
		add_triangle(REF_ elements, REF_ elementIdx, 0, 2, 3);

		mesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

		material = new ::System::Material();
		material->set_shader(System::MaterialShaderUsage::Static, logoShaderProgram.get());
		material->set_uniform(NAME(inData), texture->get_texture_id());
		material->set_src_blend_op(System::BlendOp::SrcAlpha);
		material->set_dest_blend_op(System::BlendOp::OneMinusSrcAlpha);
		material->allow_individual_instances(true);

		meshInstance.set_placement(Transform(Vector3(0.0f, 0.0f, 0.0f), Quat::identity));
		meshInstance.set_mesh(mesh);
		meshInstance.set_material(0, material);
	}
}

void LoadingLogo::construct_shaders()
{
	{	// logos
		String vertexShaderSource
#ifdef AN_GL
			= String(TXT("#version 150\n"))
#endif
#ifdef AN_GLES
			= String(TXT("#version 300 es\n"))
#endif
			+ String(TXT("\n"))
			+ String(TXT("// uniforms\n"))
			+ String(TXT("uniform mat4 modelViewMatrix;\n"))
			+ String(TXT("uniform mat4 projectionMatrix;\n"))
			+ String(TXT("\n"))
			+ String(TXT("in vec2 inAPosition;\n"))
			+ String(TXT("in vec2 inOUV;\n"))
			+ String(TXT("in vec4 inOColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// output\n"))
			+ String(TXT("out vec2 varUV;\n"))
			+ String(TXT("out vec4 varColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("void main()\n"))
			+ String(TXT("{\n"))
			+ String(TXT("	vec4 prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	prcPosition.xy = inAPosition;\n"))
			+ String(TXT("	prcPosition.z = 0.0;\n"))
			+ String(TXT("	prcPosition.w = 1.0;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	prcPosition = modelViewMatrix * prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	// fill output\n"))
			+ String(TXT("	gl_Position = projectionMatrix * prcPosition;\n"))
			+ String(TXT("	\n"))
			+ String(TXT("	// just copy\n"))
			+ String(TXT("	varUV = inOUV;\n"))
			+ String(TXT("  varColour = inOColour;\n"))
			+ String(TXT("}\n"))
			;

		String fragmentShaderSource
#ifdef AN_GL
			= String(TXT("#version 150\n"))
#endif
#ifdef AN_GLES
			= String(TXT("#version 300 es\n"))
#endif
			+ String(TXT("\n"))
			+ String(TXT("#ifdef GL_ES\n"))
			+ String(TXT("precision mediump float;\n"))
			+ String(TXT("precision mediump int;\n"))
			+ String(TXT("#endif\n"))
			+ String(TXT("\n"))
			+ String(TXT("#define PROCESSING_TEXTURE_SHADER\n"))
			+ String(TXT("\n"))
			+ String(TXT("// uniform\n"))
			+ String(TXT("uniform sampler2D inData;\n"))
			+ String(TXT("uniform float logoAlpha;\n"))
			+ String(TXT("uniform float activation;\n"))
			+ String(TXT("uniform vec4 overrideColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// input\n"))
			+ String(TXT("in vec2 varUV;\n"))
			+ String(TXT("in vec4 varColour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("// output\n"))
			+ String(TXT("out vec4 colour;\n"))
			+ String(TXT("\n"))
			+ String(TXT("void main()\n"))
			+ String(TXT("{\n"))
			+ String(TXT("    colour.rgba = texture(inData, varUV).rgba;\n"))
			+ String(TXT("    colour.rgba *= varColour;\n"))
			+ String(TXT("    colour.rgb = colour.rgb * (1.0 - overrideColour.a) + overrideColour.rgb * overrideColour.a;\n"))
			+ String(TXT("    colour.a *= logoAlpha;\n"))
			+ String(TXT("}\n"));

		logoVertexShader = new System::Shader(System::ShaderSetup::from_string(System::ShaderType::Vertex, vertexShaderSource));
		logoFragmentShader = new System::Shader(System::ShaderSetup::from_string(System::ShaderType::Fragment, fragmentShaderSource));

		logoFragmentShader->access_fragment_shader_output_setup().allow_default_output();

		if (logoVertexShader.get() && logoFragmentShader.get())
		{
			logoShaderProgram = new System::ShaderProgram(logoVertexShader.get(), logoFragmentShader.get());
		}
	}
}

void LoadingLogo::update(float _deltaTime)
{
	if (waitForVR)
	{
		if (::System::Core::is_vr_paused() ||
			::System::Core::is_rendering_paused())
		{
			_deltaTime = 0.0f;
		}
	}
	_deltaTime = min(_deltaTime, 0.005f); // shouldn't be higher than that
	if (showTimeLeft.is_set())
	{
		showTimeLeft = showTimeLeft.get() - _deltaTime;
		targetActivation = showTimeLeft.get() <= 0.0f ? 1.0f : 0.0f;
	}
	//_deltaTime *= 0.05f;
	blockDeactivationFor = max(0.0f, blockDeactivationFor - _deltaTime);
#ifdef STAY_HERE
	blockDeactivationFor = 1.0f;
#endif
	float currentTargetActivation = blockDeactivationFor > 0.0f ? 1.0f : targetActivation;
	if (onlySplash && !rotation.is_set())
	{
		currentTargetActivation = min(currentTargetActivation, 0.0001f);
	}
	activation = blend_to_using_speed_based_on_time(activation, currentTargetActivation, currentTargetActivation > activation ? 0.6f : 0.45f, min(_deltaTime, 0.03f));
	if (activation <= 0.0f && targetActivation <= 0.0f)
	{
		base::deactivate();
	}

	update_starting_yaw_if_required();

	if (auto* vr = VR::IVR::get())
	{
		if (vr->is_render_view_valid())
		{
			auto viewTransform = vr->get_render_view();
			Rotator3 newRotation = viewTransform.get_orientation().to_rotator();

			if (!rotation.is_set())
			{
				rotation = newRotation;
			}
			else
			{
				newRotation = rotation.get() + (newRotation - rotation.get()).normal_axes();
				rotation = blend_to_using_time(rotation.get(), newRotation, 1.0f, _deltaTime);
			}
		}
	}
	else
	{
		rotation = Rotator3::zero;
	}

	{
		logoShowDelay = max(0.0f, logoShowDelay - _deltaTime);
		if (logoShowDelay == 0.0f)
		{
			for_every(lm, logoMeshes)
			{
				lm->visible = blend_to_using_speed_based_on_time(lm->visible, 1.0f, logoVisibilityBlend, _deltaTime);
				if (lm->visible < 0.5f)
				{
					blockDeactivationFor = max(blockDeactivationFor, 1.0f);
					break; // wait with other logos to appear
				}
			}
		}
	}
}

void LoadingLogo::update_starting_yaw_if_required()
{
	if (!startingYaw.is_set())
	{
		if (auto* vr = VR::IVR::get())
		{
			if (vr->is_render_view_valid())
			{
				auto viewTransform = vr->get_render_view();
				Rotator3 rot = viewTransform.get_orientation().to_rotator();
				startingYaw = rot.yaw;
			}
		}
	}
}

void LoadingLogo::display(System::Video3D * _v3d, bool _vr)
{
	if (!VR::IVR::get() ||
		VR::IVR::get()->get_available_functionality().renderToScreen)
	{
		display_at(_v3d, false);
	}
	if (_vr)
	{
		display_at(_v3d, _vr);
	}
}

void LoadingLogo::display_at(::System::Video3D * _v3d, bool _vr, Vector2 const & _at, float _scale)
{
	rt->bind();
	_v3d->set_default_viewport();
	_v3d->set_near_far_plane(0.02f, 100.0f);

	// ready sizes
	Vector2 size(12.0f, 12.0f);
	size.x = size.y * _v3d->get_aspect_ratio();
	Vector2 centre = size * 0.5f / _scale;

	float centreToLogoMeshDistance = 7.0f;
	if (_vr)
	{
		centre = Vector2::zero;
	}
	else if (!logoMeshes.is_empty())
	{
		centre.x -= centreToLogoMeshDistance * 0.4f;
	}

	float logosCentreVertically = logosHeight < logoHeight ? 0.4f : 1.0f; // if doesn't fit, always centre
	float logosBottom = (1.0f - logosCentreVertically) * (-logoHeight * 0.5f) + logosCentreVertically * (-logosHeight * 0.5f);

	::System::VideoMatrixStack& matrixStack = _v3d->access_model_view_matrix_stack();

	::Framework::Render::Scene* scenes[2];

	Transform vrPlacement;

	Colour currentBackgroundColour = MainConfig::global().get_video().forcedBlackColour.get(Colour::lerp(BlendCurve::cubic(activation), Colour::black, Colour::black));
	Colour mainColour = Colour::lerp(BlendCurve::cubic(activation), currentBackgroundColour, Colour::white);
	
	Vector3 eyeLoc = Vector3(0.0f, 0.0f, 1.65f);
	if (_vr)
	{
		auto* vr = VR::IVR::get();
		Transform eyePlacement = look_at_matrix(eyeLoc, eyeLoc + Vector3(0.0f, 2.0f, 0.0f), Vector3::zAxis).to_transform();
		if (vr->is_render_view_valid())
		{
			eyePlacement = vr->get_render_view();
		}
		eyeLoc = eyePlacement.get_translation();

		update_vr_centre(eyeLoc);
		Vector3 vrLoc = vrCentre.get() + logoDistance * Vector3::yAxis + Vector3(0.0f, 0.0f, 1.3f);
		Vector3 vrAtLoc = vrLoc + Vector3(0.0f, 0.0f, 1.0f);
		vrPlacement = look_at_matrix(vrLoc, vrAtLoc, -Vector3::yAxis).to_transform();
		//vrPlacement.set_scale(0.5f);

		::Framework::Render::Camera camera;
		camera.set_placement(nullptr, eyePlacement);
		camera.set_near_far_plane(0.02f, 30000.0f);
		camera.set_background_near_far_plane();

		scenes[0] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Left);
		scenes[1] = Framework::Render::Scene::build(camera, NP, nullptr, NP, Framework::Render::SceneMode::VR_Right);

		scenes[0]->set_background_colour(currentBackgroundColour);
		scenes[1]->set_background_colour(currentBackgroundColour);
	}
	else
	{
		_v3d->setup_for_2d_display();
		_v3d->set_2d_projection_matrix_ortho(size);
		matrixStack.clear();
		matrixStack.push_to_world(translation_scale_xy_matrix(-_at * _scale, Vector2(_scale, _scale)));

		_v3d->clear_colour_depth_stencil(currentBackgroundColour);

		_v3d->set_fallback_colour(mainColour);
	}

	int dirCount = 5;
	float dirAngle = 360.0f / (float)dirCount;

	// display logos

	_v3d->set_fallback_colour(Colour::white); // full colour

	Colour useBlack = MainConfig::global().get_video().forcedBlackColour.get(Colour::black);
	Colour overrideColour = useBlack.with_alpha(BlendCurve::cubic(1.0f - activation));
	if (MainConfig::global().get_video().forcedBlackColour.is_set())
	{
		overrideColour = activation > 0.5f? useBlack.with_alpha(0.0f) : useBlack;
	}

	for (int lIdx = 0; lIdx < logoMeshes.get_size(); ++lIdx)
	{
		LogoMesh& l = logoMeshes[lIdx];

		float useVisible = BlendCurve::cubic(l.visible);
		if (MainConfig::global().get_video().forcedBlackColour.is_set())
		{
			useVisible = 1.0f;
		}

		if (_vr)
		{
			Transform logoPlacement = translation_matrix(Vector3(0.0f, logosBottom, 0.0f)).to_transform();
			logoPlacement.set_scale(0.7f);

			Transform logoTransform = vrPlacement.to_world(logoPlacement);
			for_count(int, i, dirCount)
			{
				Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw.get(0.0f) + dirAngle * 0.5f + dirAngle * (float)i, 0.0f).to_quat());
				l.meshInstance.set_fallback_colour(Colour::white.with_alpha(l.visible));
				l.meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
				l.meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
				scenes[0]->add_extra(&l.meshInstance, rotate.to_world(logoTransform));
				scenes[1]->add_extra(&l.meshInstance, rotate.to_world(logoTransform));
			}
		}
		else
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(centre.x + centreToLogoMeshDistance, centre.y + logosBottom, 0.0f));

			::Meshes::Mesh3DRenderingContext renderingContext;

			matrixStack.push_to_world(logoPlacement);
			l.meshInstance.set_fallback_colour(Colour::white.with_alpha(l.visible));
			l.meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
			l.meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
			l.meshInstance.render(_v3d, renderingContext);
			matrixStack.pop();
		}
	}

	// display logo

	_v3d->set_fallback_colour(Colour::white); // full colour

	{
		float useVisible = activation;
		if (MainConfig::global().get_video().forcedBlackColour.is_set())
		{
			useVisible = 1.0f;
		}

		if (_vr)
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(0.0f, 0.0f, 0.0f));

			Transform logoTransform = vrPlacement.to_world(logoPlacement.to_transform());
			if (! onlySplash)
			{
				for_count(int, i, dirCount)
				{
					Transform rotate(Vector3::zero, Rotator3(0.0f, startingYaw.get(0.0f) + dirAngle * (float)i, 0.0f).to_quat());
					for_count(int, i, 2)
					{
						auto& meshInstance = i == 0 ? logoMeshInstance : logoDemoMeshInstance;
						if (!meshInstance.get_mesh()) { continue; }
						meshInstance.set_fallback_colour(Colour::white);
						meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
						meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
						scenes[0]->add_extra(&meshInstance, rotate.to_world(logoTransform));
						scenes[1]->add_extra(&meshInstance, rotate.to_world(logoTransform));
					}
				}
			}
			else
			{
				if (rotation.is_set())
				{
					Transform rotate(vrCentre.get() + Vector3::zAxis * 1.3f, (rotation.get() * Rotator3(1.0f, 1.0f, 0.0f)).to_quat());
					Transform logoAtDist(Vector3::yAxis * logoTransform.get_translation().length(), vrPlacement.get_orientation());
					for_count(int, i, 2)
					{
						auto& meshInstance = i == 0 ? logoMeshInstance : logoDemoMeshInstance;
						if (!meshInstance.get_mesh()) { continue; }
						meshInstance.set_fallback_colour(Colour::white);
						meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
						meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
						scenes[0]->add_extra(&meshInstance, rotate.to_world(logoAtDist));
						scenes[1]->add_extra(&meshInstance, rotate.to_world(logoAtDist));
					}
				}
			}
		}
		else
		{
			Matrix44 logoPlacement = translation_matrix(Vector3(centre.x, centre.y, 0.0f));

			::Meshes::Mesh3DRenderingContext renderingContext;

			matrixStack.push_to_world(logoPlacement);
			for_count(int, i, 2)
			{
				auto& meshInstance = i == 0 ? logoMeshInstance : logoDemoMeshInstance; 
				if (!meshInstance.get_mesh()) { continue; }
				meshInstance.set_fallback_colour(Colour::white);
				meshInstance.access_material_instance(0)->set_uniform(NAME(logoAlpha), useVisible);
				meshInstance.access_material_instance(0)->set_uniform(NAME(overrideColour), overrideColour.to_vector4());
				meshInstance.render(_v3d, renderingContext);
			}
			matrixStack.pop();
		}
	}

	if (_vr)
	{
		scenes[0]->prepare_extras();
		scenes[1]->prepare_extras();

		Framework::Render::Context rc(_v3d);

		todo_note(TXT("store default texture somewhere"));
		//rc.set_default_texture_id(Framework::Library::get_current()->get_default_texture()->get_texture()->get_texture_id());

		_v3d->set_fallback_colour(Colour::lerp(BlendCurve::cubic(activation), currentBackgroundColour, Colour::white));

		for_count(int, i, 2)
		{
			scenes[i]->render(rc, rt.get());
			scenes[i]->release();
		}

		VR::IVR::get()->copy_render_to_output(_v3d);
	}
	else
	{
		matrixStack.pop();

		_v3d->set_fallback_colour(Colour::white);

		System::RenderTarget::bind_none();

		rt->resolve();

		matrixStack.clear();
		matrixStack.force_requires_send_all();

		render_rt(_v3d);
	}
}

