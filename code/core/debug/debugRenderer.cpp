#include "debugRenderer.h"

#include "..\renderDoc.h"
#include "..\concurrency\scopedSpinLock.h"
#include "..\concurrency\threadManager.h"
#include "..\containers\arrayStack.h"
#include "..\system\video\video3d.h"
#include "..\system\video\video3dPrimitives.h"
#include "..\mesh\iMesh3d.h"
#include "..\mesh\mesh3d.h"
#include "..\collision\model.h"

#include "..\system\core.h"
#include "..\system\video\font.h"
#include "..\system\video\materialInstance.h"
#include "..\system\video\iMaterialInstanceProvider.h"
#include "..\system\video\vertexFormat.h"
#include "..\system\video\viewFrustum.h"
#include "..\system\video\videoClipPlaneStackImpl.inl"

#include "..\performance\performanceUtils.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DebugRenderer* DebugRenderer::s_debugRenderer = nullptr;

//

Colour get_debug_highlight_colour()
{
	return get_debug_highlight_colour(1.0f);
}

Colour get_debug_highlight_colour(float _pulseLength)
{
	Colour colour = Colour::red;
	colour.blend_to(Colour::yellow, 0.5f + 0.5f * sin_deg(System::Core::get_timer_raw() * 360.0f / _pulseLength));
	return colour;
}

//

void DebugRendererText::render(::System::Video3D * _v3d) const
{
	assert_rendering_thread();
	if (auto* font = DebugRenderer::get()->get_font())
	{
		auto & mvs = _v3d->access_model_view_matrix_stack();
		mvs.push_to_world(Transform(a, Quat::identity).to_matrix());
		Vector3 relAt = mvs.get_current().get_translation();
		if (alignToCamera)
		{
			mvs.push_set(look_at_matrix(mvs.get_current().get_translation(), mvs.get_current().get_translation() * Vector3(1.0f, 2.0f, 1.0f), Vector3::zAxis));
		}
		float additionalScale = 1.0f;
		if (useScreenSpaceSize)
		{
			float distance = !alignToCamera ? relAt.length() : max(0.00001f, relAt.y);
			additionalScale = 0.5f * tan_deg(_v3d->get_fov() * 0.5f) * distance;
		}
		font->draw_text_at(_v3d, text, colour, Vector3::zero, Vector2::one * (0.05f / font->get_height()) * scale * additionalScale, pt, false);
		if (alignToCamera)
		{
			mvs.pop();
		}
		mvs.pop();
	}
}

//

void DebugRendererLine::render(::System::Video3D * _v3d) const
{
	assert_rendering_thread();
	System::Video3DPrimitives::line(colour, a, b);
}

//

void DebugRendererTriangle::render(::System::Video3D * _v3d) const
{
	assert_rendering_thread();
	System::Video3DPrimitives::triangle(colour, a, b, c);
}

//

class DebugRendererMesh3DHelper
: public ::System::IMaterialInstanceProvider
, public ::Meshes::IMesh3DRenderBonesProvider
{
public:
	DebugRendererMesh3DHelper(DebugRendererMesh3D const * _debugRendererMesh3D)
	: debugRendererMesh3D(_debugRendererMesh3D)
	{}

public:	// IMaterialInstanceProvider
	override_ ::System::MaterialInstance * get_material_instance_for_rendering(int _idx)
	{
		if (debugRendererMesh3D->singleMaterial)
		{
			return debugRendererMesh3D->materialInstances[0]->materialInstance;
		}
		else
		{
			return debugRendererMesh3D->materialInstances.is_index_valid(_idx) ? debugRendererMesh3D->materialInstances[_idx]->materialInstance : nullptr;
		}
	}
	override_ ::System::MaterialInstance const * get_material_instance_for_rendering(int _idx) const
	{
		if (debugRendererMesh3D->singleMaterial)
		{
			return debugRendererMesh3D->materialInstances[0]->materialInstance;
		}
		else
		{
			return debugRendererMesh3D->materialInstances.is_index_valid(_idx) ? debugRendererMesh3D->materialInstances[_idx]->materialInstance : nullptr;
		}
	}

public:	// IMesh3DRenderBonesProvider
	override_ Matrix44 const * get_render_bone_matrices(OUT_ int & _matricesCount) const
	{
		return debugRendererMesh3D->get_render_bone_matrices(OUT_ _matricesCount);
	}

private:
	DebugRendererMesh3D const * debugRendererMesh3D;
};

//

DebugRendererMaterialInstance::~DebugRendererMaterialInstance()
{
	delete_and_clear(materialInstance);
}
//

DebugRendererMesh3D::DebugRendererMesh3D()
: singleMaterial(false)
{
}

DebugRendererMesh3D::DebugRendererMesh3D(DebugRendererMesh3D const & _debugRendererMesh3D)
: DebugRendererElement(_debugRendererMesh3D)
, mesh(_debugRendererMesh3D.mesh)
, placement(_debugRendererMesh3D.placement)
, singleMaterial(_debugRendererMesh3D.singleMaterial)
, bones(_debugRendererMesh3D.bones)
{
	for_every_ref(sourceMaterialInstance, _debugRendererMesh3D.materialInstances)
	{
		::System::MaterialInstance* materialInstance = new ::System::MaterialInstance();
		materialInstance->hard_copy_from_with_filled_missing(*sourceMaterialInstance->materialInstance);
		RefCountObjectPtr<DebugRendererMaterialInstance> mi(new DebugRendererMaterialInstance(materialInstance));
		materialInstances.push_back(mi);
	}
}

DebugRendererMesh3D::DebugRendererMesh3D(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Meshes::IMesh3D const * _mesh, Transform const & _placement, ::System::Material const*_material, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider)
: DebugRendererElement(_context, _subject, _filter, _timeBased, _front, false)
, mesh(_mesh)
, placement(_placement)
, singleMaterial(true)
{
	::System::MaterialInstance* materialInstance = new ::System::MaterialInstance();
	materialInstance->set_material(cast_to_nonconst(_material), ::System::MaterialShaderUsage::Static);
	RefCountObjectPtr<DebugRendererMaterialInstance> mi(new DebugRendererMaterialInstance(materialInstance));
	materialInstances.push_back(mi);
	copy_bones_from(_bonesProvider);
}

DebugRendererMesh3D::DebugRendererMesh3D(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Meshes::IMesh3D const * _mesh, Transform const & _placement, ::System::IMaterialInstanceProvider const * _materialInstanceProvider, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider)
: DebugRendererElement(_context, _subject, _filter, _timeBased, _front, false)
, mesh(_mesh)
, placement(_placement)
, singleMaterial(false)
{
	int idx = 0;
	while (true)
	{
		if (::System::MaterialInstance const * sourceMaterialInstance = _materialInstanceProvider->get_material_instance_for_rendering(idx))
		{
			::System::MaterialInstance* materialInstance = new ::System::MaterialInstance();
			materialInstance->hard_copy_from_with_filled_missing(*sourceMaterialInstance);
			RefCountObjectPtr<DebugRendererMaterialInstance> mi(new DebugRendererMaterialInstance(materialInstance));
			materialInstances.push_back(mi);
		}
		else
		{
			break;
		}
		++idx;
	}
	copy_bones_from(_bonesProvider);
}

DebugRendererMesh3D::~DebugRendererMesh3D()
{
}

void DebugRendererMesh3D::copy_bones_from(Meshes::IMesh3DRenderBonesProvider const * _bonesProvider)
{
	int matricesCount;
	Matrix44 const * matrices = _bonesProvider->get_render_bone_matrices(OUT_ matricesCount);
	bones.set_size(matricesCount);
	memory_copy(bones.get_data(), matrices, bones.get_data_size());
}

void DebugRendererMesh3D::render(::System::Video3D * _v3d) const
{
	assert_rendering_thread();
	if (!mesh)
	{
		return;
	}

	::System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();

	modelViewStack.push_to_world(placement.to_matrix());
	DebugRendererMesh3DHelper helper(this);
	Meshes::Mesh3DRenderingContext renderingContext;
	renderingContext.meshBonesProvider = &helper;
	renderingContext.shaderProgramBindingContext = debug_renderer_shader_program_binding_context();
	if (!materialInstances.is_empty())
	{
		mesh->render(_v3d, &helper, renderingContext);
	}
	else
	{
		mesh->render(_v3d, nullptr, renderingContext);
	}
	modelViewStack.pop();
}

//

DebugRendererRenderingContext::DebugRendererRenderingContext()
: currentContext(nullptr)
, currentPlacement(Transform::identity)
{
}

//

DebugRendererGatheringContext::DebugRendererGatheringContext()
: currentContext(nullptr)
, currentSubject(nullptr)
, currentPlacement(Transform::identity)
{
}

//

DebugRendererFrame::DebugRendererFrame(bool _reserveMemory)
{
	if (_reserveMemory)
	{
		lines.make_space_for(200000);
		triangles.make_space_for(200000);
		meshes.make_space_for(1000);
		texts.make_space_for(5000);
	}

	clear();

#ifdef AN_CONCURRENCY_STATS
	contextsLock.do_not_report_stats();
	linesLock.do_not_report_stats();
	trianglesLock.do_not_report_stats();
	meshesLock.do_not_report_stats();
	textsLock.do_not_report_stats();
#endif
}

DebugRendererFrameCheckpoint DebugRendererFrame::get_checkpoint()
{
	if (!should_gather_for_debug_renderer())
	{
		return DebugRendererFrameCheckpoint();
	}

	//

	DebugRendererFrameCheckpoint checkpoint;
	{
		Concurrency::ScopedSpinLock lock(linesLock);
		checkpoint.lines = lines.get_size();
	}
	{
		Concurrency::ScopedSpinLock lock(trianglesLock);
		checkpoint.triangles = triangles.get_size();
	}
	{
		Concurrency::ScopedSpinLock lock(meshesLock);
		checkpoint.meshes = meshes.get_size();
	}
	{
		Concurrency::ScopedSpinLock lock(textsLock);
		checkpoint.texts = texts.get_size();
	}

	return checkpoint;
}

void DebugRendererFrame::restore_checkpoint(DebugRendererFrameCheckpoint const & _checkpoint)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	//

	{
		Concurrency::ScopedSpinLock lock(linesLock);
		lines.set_size(min(_checkpoint.lines, lines.get_size()));
	}
	{
		Concurrency::ScopedSpinLock lock(trianglesLock);
		triangles.set_size(min(_checkpoint.triangles, triangles.get_size()));
	}
	{
		Concurrency::ScopedSpinLock lock(meshesLock);
		meshes.set_size(min(_checkpoint.meshes, meshes.get_size()));
	}
	{
		Concurrency::ScopedSpinLock lock(textsLock);
		texts.set_size(min(_checkpoint.texts, texts.get_size()));
	}

	//
}

void DebugRendererFrame::apply_transform(DebugRendererFrameCheckpoint const & _sinceCheckpoint, Transform const & _transform)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	//

	{
		Concurrency::ScopedSpinLock lock(linesLock);
		for (int i = _sinceCheckpoint.lines; i < lines.get_size(); ++i)
		{
			auto& l = lines[i];
			l.a = _transform.location_to_world(l.a);
			l.b = _transform.location_to_world(l.b);
		}
	}
	{
		Concurrency::ScopedSpinLock lock(trianglesLock);
		for (int i = _sinceCheckpoint.triangles; i < triangles.get_size(); ++i)
		{
			auto& t = triangles[i];
			t.a = _transform.location_to_world(t.a);
			t.b = _transform.location_to_world(t.b);
			t.c = _transform.location_to_world(t.c);
		}
	}
	{
		Concurrency::ScopedSpinLock lock(meshesLock);
		for (int i = _sinceCheckpoint.meshes; i < meshes.get_size(); ++i)
		{
			auto& m = meshes[i];
			m.placement = _transform.to_world(m.placement);
		}
	}
	{
		Concurrency::ScopedSpinLock lock(textsLock);
		for (int i = _sinceCheckpoint.texts; i < texts.get_size(); ++i)
		{
			auto& t = texts[i];
			t.a = _transform.location_to_world(t.a);
		}
	}

	//
}

bool DebugRendererFrame::should_gather_for_debug_renderer() const
{
	return DebugRenderer::get()->should_gather_for_debug_renderer();
}

void DebugRendererFrame::add_from(DebugRendererFrame const * _frame)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	//

	{
		Concurrency::ScopedSpinLock lock(contextsLock);
		contexts.add_from(_frame->contexts);
	}
	{
		Concurrency::ScopedSpinLock lock(linesLock);
		lines.add_from(_frame->lines);
	}
	{
		Concurrency::ScopedSpinLock lock(trianglesLock);
		triangles.add_from(_frame->triangles);
	}
	{
		Concurrency::ScopedSpinLock lock(meshesLock);
		meshes.add_from(_frame->meshes);
	}
	{
		Concurrency::ScopedSpinLock lock(textsLock);
		texts.add_from(_frame->texts);
	}

	//
}

void DebugRendererFrame::clear()
{

	//

	set_context();
	set_subject();
	set_filter();
	{
		Concurrency::ScopedSpinLock cLock(contextsLock);
		contexts.clear();
	}
	{
		Concurrency::ScopedSpinLock lLock(linesLock);
		lines.clear();
	}
	{
		Concurrency::ScopedSpinLock tLock(trianglesLock);
		triangles.clear();
	}
	{
		Concurrency::ScopedSpinLock mLock(meshesLock);
		meshes.clear();
	}
	{
		Concurrency::ScopedSpinLock xLock(textsLock);
		texts.clear();
	}
	useCamera = DebugRendererUseCamera::AsIs;
	camera = DebugRendererCamera();
	for_every(context, gatheringContexts)
	{
		context->filterStack.clear();
		context->transformStack.clear();
		context->currentPlacement = Transform::identity;
	}

	//
}

void DebugRendererFrame::clear_non_mesh()
{
	{
		Concurrency::ScopedSpinLock lLock(linesLock);
		lines.clear();
	}
	{
		Concurrency::ScopedSpinLock tLock(trianglesLock);
		triangles.clear();
	}
	{
		Concurrency::ScopedSpinLock xLock(textsLock);
		texts.clear();
	}
}

template <typename Class>
bool DebugRendererFrame::advance_clear(Array<Class> & _array, float _deltaTime)
{
	ARRAY_PREALLOC_SIZE(int, keep, _array.get_size());
	for_every(obj, _array)
	{
		obj->timeBased -= _deltaTime;
		if (obj->timeBased > 0.0f)
		{
			keep.push_back(for_everys_index(obj));
		}
	}
	if (keep.is_empty())
	{
		_array.clear();
		return false;
	}
	else
	{
		if (keep.get_size() < _array.get_size())
		{
			int i = 0;
			for_every(k, keep)
			{
				_array[i] = _array[*k];
				++i;
			}
			_array.set_size(keep.get_size());
		}
		return true;
	}
}

void DebugRendererFrame::advance_clear(float _deltaTime)
{
	//

	set_context();
	set_subject();
	set_filter();
	{
		Concurrency::ScopedSpinLock cLock(contextsLock);
		contexts.clear();
	}
	advance_clear(lines, _deltaTime);
	advance_clear(triangles, _deltaTime);
	advance_clear(meshes, _deltaTime);
	advance_clear(texts, _deltaTime);
	useCamera = DebugRendererUseCamera::AsIs;
	camera = DebugRendererCamera();
	for_every(context, gatheringContexts)
	{
		context->filterStack.clear();
		context->transformStack.clear();
		context->currentPlacement = Transform::identity;
	}

	//
}

void DebugRendererFrame::mark_rendered(bool _rendered)
{
	rendered = _rendered;
}

void DebugRendererFrame::render(DebugRenderer * _renderer, System::Video3D* _v3d, std::function<void()> _do_after_camera_is_set)
{
	assert_rendering_thread();

#ifdef USE_RENDER_DOC
	return;
#endif

	if (rendered)
	{
		return;
	}

	Concurrency::ScopedSpinLock lLock(linesLock);
	Concurrency::ScopedSpinLock tLock(trianglesLock);
	Concurrency::ScopedSpinLock mLock(meshesLock);
	Concurrency::ScopedSpinLock xLock(textsLock);

	//

	SUSPEND_STORE_DIRECT_GL_CALL();
	SUSPEND_STORE_DRAW_CALL();

	if (!lines.is_empty() ||
		!triangles.is_empty() ||
		!meshes.is_empty() ||
		!texts.is_empty())
	{
#ifdef AN_DEVELOPMENT
		_v3d->set_load_matrices(true);
#endif
		// "as-is" doesn't require any other camera
		if (useCamera == DebugRendererUseCamera::AsIs)
		{
			// just get camera
			System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
			System::VideoClipPlaneStack& clipPlaneStack = _v3d->access_clip_plane_stack();

			if (!modelViewStack.is_empty())
			{
				Matrix44 const & cameraMatrix = modelViewStack.get_first();

				modelViewStack.clear();
				clipPlaneStack.clear();

				modelViewStack.push_set(cameraMatrix);
			}
		}
		else if (useCamera == DebugRendererUseCamera::Provided)
		{
			float fov = camera.fov;
			if (_renderer->fovOverride.is_set())
			{
				fov = _renderer->fovOverride.get();
			}
			_v3d->set_fov(max(0.001f, fov));

			Matrix44 viewCentreOffsetMatrix = Matrix44::identity;
			if (camera.orthogonal)
			{
				Vector2 viewCentreOffset;
				viewCentreOffset.x = viewCentreOffsetMatrix.m30;
				viewCentreOffset.y = viewCentreOffsetMatrix.m31;
				Range2 range;
				Vector2 size;
				size.y = 1.0f / camera.orthogonalScale;
				size.x = _v3d->get_aspect_ratio() * size.y;
				range.x.min = size.x * 0.5f * (-1.0f - viewCentreOffset.x);
				range.x.max = size.x * 0.5f * (1.0f - viewCentreOffset.x);
				range.y.min = size.y * 0.5f * (-1.0f - viewCentreOffset.y);
				range.y.max = size.y * 0.5f * (1.0f - viewCentreOffset.y);
				_v3d->set_3d_projection_matrix(orthogonal_matrix(range, _v3d->get_near_far_plane().min, _v3d->get_near_far_plane().max));
			}
			else
			{
				if (camera.fovPort.is_set())
				{
					auto const& fp = camera.fovPort.get();
					_v3d->set_3d_projection_matrix(projection_matrix_assymetric_fov(abs(fp.left), abs(fp.right), abs(fp.up), abs(fp.down), _v3d->get_near_far_plane().min, _v3d->get_near_far_plane().max));
				}
				else
				{
					_v3d->set_3d_projection_matrix(viewCentreOffsetMatrix.to_world(projection_matrix(fov, _v3d->get_aspect_ratio(), _v3d->get_near_far_plane().min, _v3d->get_near_far_plane().max)));
				}
			}

			System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
			System::VideoClipPlaneStack& clipPlaneStack = _v3d->access_clip_plane_stack();

			modelViewStack.set_mode(System::VideoMatrixStackMode::xRight_yForward_zUp);
			modelViewStack.clear();
			clipPlaneStack.clear();

			modelViewStack.push_to_world(camera.placement.to_matrix().inverted());
		}
		else
		{
			todo_important(TXT("implement_ other cameras"));
		}

		if (_do_after_camera_is_set != nullptr)
		{
			_do_after_camera_is_set();
		}

		_v3d->setup_for_2d_display();

		// setup light
		{
			System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
			System::VideoClipPlaneStack& clipPlaneStack = _v3d->access_clip_plane_stack();

			modelViewStack.push_set(Matrix44::identity);
			clipPlaneStack.clear();

#ifdef AN_GL
			// TODO light
			{
				DIRECT_GL_CALL_ glEnable(GL_LIGHTING); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glEnable(GL_LIGHT0); AN_GL_CHECK_FOR_ERROR

				GLfloat lightAmbient[] = { 0.39f, 0.33f, 0.31f };
				GLfloat lightDiffuse[] = { 0.50f, 0.61f, 0.70f };
				GLfloat lightSpecular[] = { 0.0f, 0.0f, 0.0f };
				GLfloat lightPosition[] = { 0.0f, -1.0f, 0.0f, 0.0f }; // directional
				// TODO camera.setup sets up model view stack
				DIRECT_GL_CALL_ glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ glLightfv(GL_LIGHT0, GL_POSITION, lightPosition); AN_GL_CHECK_FOR_ERROR

				DIRECT_GL_CALL_ glEnable(GL_COLOR_MATERIAL); AN_GL_CHECK_FOR_ERROR
				DIRECT_GL_CALL_ //glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ); AN_GL_CHECK_FOR_ERROR
			}
#endif

			modelViewStack.pop();
		}

		for (int mode = 0; mode < 4; ++mode)
		{
			MEASURE_PERFORMANCE(debugRenderer_mode);

			bool renderFront = (mode % 2) == 1;
			bool renderBlended = mode >= 2;

			_v3d->depth_test(renderFront ? ::System::Video3DCompareFunc::None : ::System::Video3DCompareFunc::LessEqual);

			DebugRendererRenderingContext renderContext;

			set_context();

			renderContext.currentPlacement = Transform::identity;

#ifdef AN_GL
			DIRECT_GL_CALL_ glEnable(GL_LIGHTING); AN_GL_CHECK_FOR_ERROR
#endif
			_v3d->mark_disable_blend();
			_v3d->depth_mask(!renderFront);
			_v3d->front_face_order(::System::FaceOrder::CW);
			_v3d->face_display(::System::FaceDisplay::Front);
			_v3d->ready_for_rendering();
			render_all_of(_renderer, _v3d, REF_ renderContext, renderFront, renderBlended, meshes);

#ifdef AN_GL
			DIRECT_GL_CALL_ glDisable(GL_LIGHTING); AN_GL_CHECK_FOR_ERROR
#endif
			_v3d->depth_mask(! renderBlended);
			if (_renderer->blending == DebugRendererBlending::On && renderBlended)
			{
				_v3d->mark_enable_blend();
			}
			_v3d->front_face_order(::System::FaceOrder::CW);
			_v3d->face_display(::System::FaceDisplay::Both);
			_v3d->ready_for_rendering();
			if (_renderer->blending != DebugRendererBlending::Off || ! renderBlended)
			{
				render_triangles(_renderer, _v3d, REF_ renderContext, renderFront, renderBlended);
			}
			_v3d->depth_mask(false);
			render_lines(_renderer, _v3d, REF_ renderContext, renderFront, renderBlended);

			_v3d->depth_mask(!renderBlended);
			if (renderFront)
			{
				render_all_of(_renderer, _v3d, REF_ renderContext, renderFront, renderBlended, texts);
			}

			switch_context(_v3d, REF_ renderContext);
		}

#ifdef AN_DEVELOPMENT
		_v3d->set_load_matrices(false);
#endif
	}

	RESTORE_STORE_DIRECT_GL_CALL();
	RESTORE_STORE_DRAW_CALL();

	//
}

template <typename Element>
void DebugRendererFrame::render_all_of(DebugRenderer * _renderer, System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, bool _renderFront, bool _renderBlended, Array<Element> const & _elements)
{
	for_every(element, _elements)
	{
		if (((_renderFront && element->front) ||
			 (!_renderFront && !element->front)) &&
			((_renderBlended && element->blended) ||
			 (!_renderBlended && !element->blended)) &&
			(_renderer->is_filter_active(element->filter)) &&
			 _renderer->is_active_subject(element->subject))
		{
			int contextIdx = 0;
			while (switch_context(_v3d, REF_ _renderContext, element->context, &contextIdx))
			{
				element->render(_v3d);
			}
		}
	}
}

struct DebugRenderVertex
{
	Vector3 loc;
	Colour colour;

	DebugRenderVertex() {}
	DebugRenderVertex(Vector3 const & _loc, Colour const & _colour) : loc(_loc), colour(_colour){}
};

void DebugRendererFrame::render_lines(DebugRenderer * _renderer, System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, bool _renderFront, bool _renderBlended)
{
	assert_rendering_thread();

	::System::VertexFormat vf;
	vf.with_location_3d();
	vf.with_colour_rgba();
	vf.no_padding();
	vf.calculate_stride_and_offsets();
	vf.used_for_dynamic_data();

	if (_renderer->blending == DebugRendererBlending::On)
	{
		_v3d->mark_enable_blend();
	}
	_v3d->ready_for_rendering();
	_v3d->set_fallback_colour();
	_v3d->mark_enable_polygon_offset(-0.1f, -0.1f);

	int const MAXLINES = 5000;
	ArrayStatic<DebugRenderVertex, MAXLINES * 2> renderLineVertices;

	for_every(line, lines)
	{
		if (((_renderFront && line->front) ||
			(!_renderFront && !line->front)) &&
			((_renderBlended && line->blended) ||
			 (!_renderBlended && !line->blended)) &&
			(_renderer->is_filter_active(line->filter)) &&
			 _renderer->is_active_subject(line->subject))
		{
			int contextIdx = 0;
			while (true)
			{
				if (renderLineVertices.get_size() >= MAXLINES * 2 - 2 || requires_switching_context(_renderContext, line->context, contextIdx))
				{
					Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr,
						Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderLineVertices.get_data(), ::Meshes::Primitive::Line, renderLineVertices.get_size() / 2, vf);
					renderLineVertices.clear();
				}
				if (!switch_context(_v3d, REF_ _renderContext, line->context, &contextIdx))
				{
					break;
				}
				renderLineVertices.push_back(DebugRenderVertex(line->a, line->colour));
				renderLineVertices.push_back(DebugRenderVertex(line->b, line->colour));
			}
		}
	}

	if (!renderLineVertices.is_empty())
	{
		Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr,
			Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderLineVertices.get_data(), ::Meshes::Primitive::Line, renderLineVertices.get_size() / 2, vf);
	}

	_v3d->mark_disable_polygon_offset();
	_v3d->set_fallback_colour();
	_v3d->mark_disable_blend();
}


void DebugRendererFrame::render_triangles(DebugRenderer * _renderer, System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, bool _renderFront, bool _renderBlended)
{
	assert_rendering_thread();

	::System::VertexFormat vf;
	vf.with_location_3d();
	vf.with_colour_rgba();
	vf.no_padding();
	vf.calculate_stride_and_offsets();
	vf.used_for_dynamic_data();

	if (_renderer->blending == DebugRendererBlending::On)
	{
		_v3d->mark_enable_blend();
	}
	_v3d->ready_for_rendering();
	_v3d->set_fallback_colour();
	_v3d->mark_enable_polygon_offset(-0.1f, -0.1f);

	int const MAXTRIS = 5000;
	ArrayStatic<DebugRenderVertex, MAXTRIS * 3> renderTriVertices;

	for_every(tri, triangles)
	{
		if (((_renderFront && tri->front) ||
			(!_renderFront && !tri->front)) &&
			((_renderBlended && tri->blended) ||
			 (!_renderBlended && !tri->blended)) &&
			(_renderer->is_filter_active(tri->filter)) &&
			 _renderer->is_active_subject(tri->subject))
		{
			int contextIdx = 0;
			while (true)
			{
				if (renderTriVertices.get_size() >= MAXTRIS * 3 - 3 || requires_switching_context(_renderContext, tri->context, contextIdx))
				{
					if (_renderer->blending == DebugRendererBlending::On)
					{
						Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr,
							Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderTriVertices.get_data(), ::Meshes::Primitive::Triangle, renderTriVertices.get_size() / 3, vf);
					}
					else
					{
						Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr,
							Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderTriVertices.get_data(), ::Meshes::Primitive::Line, renderTriVertices.get_size() / 2, vf);
					}
					renderTriVertices.clear();
				}
				if (!switch_context(_v3d, REF_ _renderContext, tri->context, &contextIdx))
				{
					break;
				}
				if (_renderer->blending == DebugRendererBlending::On)
				{
					renderTriVertices.push_back(DebugRenderVertex(tri->a, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->b, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->c, tri->colour));
				}
				else
				{
					renderTriVertices.push_back(DebugRenderVertex(tri->a, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->b, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->b, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->c, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->c, tri->colour));
					renderTriVertices.push_back(DebugRenderVertex(tri->a, tri->colour));
				}
			}
		}
	}

	if (!renderTriVertices.is_empty())
	{
		if (_renderer->blending == DebugRendererBlending::On)
		{
			Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr,
				Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderTriVertices.get_data(), ::Meshes::Primitive::Triangle, renderTriVertices.get_size() / 3, vf);
		}
		else
		{
			Meshes::Mesh3D::render_data(_v3d, nullptr, nullptr,
				Meshes::Mesh3DRenderingContext().with_existing_face_display(), renderTriVertices.get_data(), ::Meshes::Primitive::Line, renderTriVertices.get_size() / 2, vf);
		}
	}

	_v3d->mark_disable_polygon_offset();
	_v3d->set_fallback_colour();
	_v3d->mark_disable_blend();
}

void DebugRendererFrame::add_text(bool _front, Optional<Colour> const & _colour, Vector3 const & _a, Optional<Vector2> const & _pt, Optional<bool> const & _alignToCamera, Optional<float> const & _scale, Optional<bool> const & _useScreenSpaceSize, tchar const * const _text, ...)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(textsLock);

	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	texts.push_back(DebugRendererText(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour.get(Colour::white), gc.currentPlacement.location_to_world(_a), _pt.get(Vector2::half), _alignToCamera.get(true), _scale.get(1.0f), _useScreenSpaceSize.get(false)));
	va_list list;
	va_start(list, _text);
#ifndef AN_CLANG
	int textsize = (int)(tstrlen(_text) + 1);
	int memsize = textsize * sizeof(tchar);
	allocate_stack_var(tchar, format, textsize);
	memory_copy(format, _text, memsize);
	sanitise_string_format(format);
#else
	tchar const* format = _text;
#endif
	tvsprintf(texts.get_last().text, DebugRendererText::length, format, list);

	//
}

void DebugRendererFrame::add_line(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(linesLock);
	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_b)));

	//
}

void DebugRendererFrame::add_arrow(bool _front, Colour const& _colour, Vector3 const& _a, Vector3 const& _b, float _size, bool _doubleSide)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	if (Vector3::distance_squared(_a, _b) == 0.0f)
	{
		return;
	}

	//

	if (_size == 0.0f)
	{
		_size = (_b - _a).length() * 0.05f;
	}

	DebugRendererGatheringContext& gc = access_gathering_context();
	{
		Concurrency::ScopedSpinLock lock(linesLock);
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_b)));
	}
	Vector3 dir = (_b - _a).normal();
	Vector3 up = Vector3::zAxis;
	if (abs(dir.z) > 0.75f)
	{
		up = dir.x < 0.75f ? Vector3::xAxis : Vector3::yAxis;
	}
	Vector3 side = Vector3::cross(dir, up);
	{
		Concurrency::ScopedSpinLock lock(linesLock);
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_b), gc.currentPlacement.location_to_world(_b + ( side - dir) * _size)));
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_b), gc.currentPlacement.location_to_world(_b + (-side - dir) * _size)));
		if (_doubleSide)
		{
			lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_a + (side + dir) * _size)));
			lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_a + (-side + dir) * _size)));
		}
	}

	//
}

void DebugRendererFrame::add_segment(bool _front, Colour const& _colour, Vector3 const& _a, Vector3 const& _b, float _size)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	if (Vector3::distance_squared(_a, _b) == 0.0f)
	{
		return;
	}

	//

	if (_size == 0.0f)
	{
		_size = (_b - _a).length() * 0.05f;
	}

	DebugRendererGatheringContext& gc = access_gathering_context();
	{
		Concurrency::ScopedSpinLock lock(linesLock);
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_b)));
	}
	Vector3 dir = (_b - _a).normal();
	Vector3 up = Vector3::zAxis;
	if (abs(dir.z) > 0.75f)
	{
		up = dir.x < 0.75f ? Vector3::xAxis : Vector3::yAxis;
	}
	Vector3 side = Vector3::cross(dir, up);
	{
		Concurrency::ScopedSpinLock lock(linesLock);
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a - side * _size), gc.currentPlacement.location_to_world(_a + side * _size)));
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_b - side * _size), gc.currentPlacement.location_to_world(_b + side * _size)));
	}

	//
}

void DebugRendererFrame::add_triangle(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(trianglesLock);
	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	triangles.push_back(DebugRendererTriangle(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_b), gc.currentPlacement.location_to_world(_c)));

	//
}

void DebugRendererFrame::add_triangle_border(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(linesLock);
	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_a), gc.currentPlacement.location_to_world(_b)));
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_b), gc.currentPlacement.location_to_world(_c)));
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colour, gc.currentPlacement.location_to_world(_c), gc.currentPlacement.location_to_world(_a)));

	//
}

void DebugRendererFrame::add_matrix(bool _front, Matrix44 const & _matrix, float _size, Colour const & _colourX, Colour const & _colourY, Colour const & _colourZ, float _rgbAt)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(linesLock);
	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	Matrix44 matrix = gc.currentPlacement.to_matrix().to_world(_matrix);
	Vector3 loc = matrix.get_translation();
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colourX, loc, loc + matrix.get_x_axis() * _size * _rgbAt));
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colourY, loc, loc + matrix.get_y_axis() * _size * _rgbAt));
	lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _colourZ, loc, loc + matrix.get_z_axis() * _size * _rgbAt));
	if (_rgbAt < 1.0f)
	{
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, Colour::red, loc + matrix.get_x_axis() * _size * _rgbAt, loc + matrix.get_x_axis() * _size));
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, Colour::green, loc + matrix.get_y_axis() * _size * _rgbAt, loc + matrix.get_y_axis() * _size));
		lines.push_back(DebugRendererLine(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, Colour::blue, loc + matrix.get_z_axis() * _size * _rgbAt, loc + matrix.get_z_axis() * _size));
	}

	//
}

void DebugRendererFrame::add_mesh(bool _front, Meshes::IMesh3D* _mesh, Transform const & _placement, ::System::Material const* _material, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(meshesLock);
	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	meshes.push_back(DebugRendererMesh3D(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _mesh, gc.currentPlacement.to_world(_placement), _material, _bonesProvider));

	//
}

void DebugRendererFrame::add_mesh(bool _front, Meshes::IMesh3D* _mesh, Transform const & _placement, ::System::IMaterialInstanceProvider const* _materialInstanceProvider, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	Concurrency::ScopedSpinLock lock(meshesLock);
	//

	DebugRendererGatheringContext& gc = access_gathering_context();
	meshes.push_back(DebugRendererMesh3D(gc.currentContext, gc.currentSubject, gc.currentFilter, gc.timeBased, _front, _mesh, gc.currentPlacement.to_world(_placement), _materialInstanceProvider, _bonesProvider));

	//
}

void DebugRendererFrame::add_collision_model(bool _frontBorder, bool _frontFill, ::Collision::Model* _cm, Colour const & _colour, float _alphaFill, Transform const & _placement)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	push_transform(_placement);

	_cm->debug_draw(_frontBorder, _frontFill, _colour, _alphaFill);

	pop_transform();
}

void DebugRendererFrame::add_box(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Box const & _box)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	_box.debug_draw(_frontBorder, _frontFill, _colour, _alphaFill);
}

void DebugRendererFrame::add_convex_hull(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, ConvexHull const & _convexHull)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	_convexHull.debug_draw(_frontBorder, _frontFill, _colour, _alphaFill);
}

void DebugRendererFrame::add_sphere(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Sphere const & _sphere)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	_sphere.debug_draw(_frontBorder, _frontFill, _colour, _alphaFill);
}

void DebugRendererFrame::add_capsule(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Capsule const & _capsule)
{
	if (!should_gather_for_debug_renderer())
	{
		return;
	}

	_capsule.debug_draw(_frontBorder, _frontFill, _colour, _alphaFill);
}

void DebugRendererFrame::get_camera_info(OUT_ DebugRendererUseCamera::Type& _useCamera, OUT_ DebugRendererCamera& _camera) const
{
	_useCamera = useCamera;
	_camera = camera;
}

void DebugRendererFrame::set_camera_info(DebugRendererUseCamera::Type const& _useCamera, DebugRendererCamera const& _camera)
{
	useCamera = _useCamera;
	camera = _camera;
}

void DebugRendererFrame::set_camera(Transform const & _placement, float _fov, Optional<VRFovPort> const& _fovPort)
{
	useCamera = DebugRendererUseCamera::Provided;
	camera.placement = _placement;
	camera.fov = _fov;
	camera.fovPort = _fovPort;
	camera.orthogonal = false;
}

void DebugRendererFrame::set_camera_orthogonal(Transform const & _placement, float _orthogonalScale)
{
	useCamera = DebugRendererUseCamera::Provided;
	camera.placement = _placement;
	camera.fov = 0.0f;
	camera.orthogonal = true;
	camera.orthogonalScale = _orthogonalScale;
}

DebugRendererGatheringContext& DebugRendererFrame::access_gathering_context()
{
	Concurrency::ScopedSpinLock cLock(contextsLock);
	if (Concurrency::ThreadManager* threadManager = Concurrency::ThreadManager::get())
	{
		if (gatheringContexts.get_size() == 0)
		{
			static Concurrency::SpinLock lock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
#ifdef AN_CONCURRENCY_STATS
			lock.do_not_report_stats();
#endif
			lock.acquire();
			gatheringContexts.set_size(threadManager->get_thread_count());
			lock.release();
		}
		an_assert(gatheringContexts.get_size() == threadManager->get_thread_count());
		return gatheringContexts[threadManager->get_current_thread_id()];
	}
	else
	{
		return defaultGatheringContexts;
	}
}

void DebugRendererFrame::set_filter(Name const & _filter)
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.currentFilter = _filter;
}

void DebugRendererFrame::push_filter(Name const & _filter)
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.filterStack.push_back(gc.currentFilter);
	gc.currentFilter = _filter;
}

void DebugRendererFrame::pop_filter()
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.currentFilter = gc.filterStack.get_last();
	gc.filterStack.pop_back();
}

Name const & DebugRendererFrame::get_filter()
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	return gc.currentFilter;
}

void DebugRendererFrame::push_transform(Transform const & _placement)
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.currentPlacement = gc.currentPlacement.to_world(_placement);
	gc.transformStack.push_back(gc.currentPlacement);
}

void DebugRendererFrame::pop_transform()
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.transformStack.pop_back();
	gc.currentPlacement = gc.transformStack.is_empty() ? Transform::identity : gc.transformStack.get_last();
}

void DebugRendererFrame::be_time_based(float _timeBased)
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.timeBased = _timeBased;
}

void DebugRendererFrame::set_context(void const * _context)
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.currentContext = _context;
}

void DebugRendererFrame::set_subject(void const * _subject)
{
	DebugRendererGatheringContext& gc = access_gathering_context();
	gc.currentSubject = _subject;
}

void DebugRendererFrame::define_context(void const * _context, Transform const & _placement)
{
	an_assert(_context, TXT("can add context only for valid address"));
	Concurrency::ScopedSpinLock cLock(contextsLock);
	contexts.push_back(DebugRendererContext(_context, _placement));
}

void DebugRendererFrame::undefine_contexts()
{
	Concurrency::ScopedSpinLock cLock(contextsLock);
	contexts.clear();
}

bool DebugRendererFrame::requires_switching_context(REF_ DebugRendererRenderingContext & _renderContext, void const * _context, int _contextIndex)
{
	Concurrency::ScopedSpinLock cLock(contextsLock);
	int index = 0;
	if (_context)
	{
		for_every(context, contexts)
		{
			if (context->context == _context)
			{
				if (index == _contextIndex)
				{
					return _renderContext.currentContext != context->context ||
						  (_context && Transform::same_with_orientation(_renderContext.currentPlacement, context->placement));
				}
				++index;
			}
		}
	}
	return _renderContext.currentContext != _context;
}

bool DebugRendererFrame::switch_context(System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, void const * _context, int * _contextIndex)
{
	Concurrency::ScopedSpinLock cLock(contextsLock);
	int index = 0;
	bool foundAnything = false;
	if (_context && _contextIndex)
	{
		for_every(context, contexts)
		{
			if (context->context == _context)
			{
				foundAnything = true;
				if (index == *_contextIndex)
				{
					switch_to_actual_context(_v3d, REF_ _renderContext, context);
					++*_contextIndex;
					return true;
				}
				++index;
			}
		}
	}
	if (! foundAnything)
	{
		if (_context != nullptr)
		{
			// we wanted to render somewhere, but there is no such place. don't render then anywhere
			return false;
		}
		switch_to_actual_context(_v3d, REF_ _renderContext, nullptr);
		// context undefined, just pretend that there is one
		if (_contextIndex)
		{
			++*_contextIndex;
			return *_contextIndex == 1;
		}
		else
		{
			// result shouldn't matter
			return false;
		}
	}
	return false;
}

void DebugRendererFrame::switch_to_actual_context(System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, DebugRendererContext const * _context)
{
	void const * actualContext = _context ? _context->context : nullptr;
	Transform actualPlacement = _context ? _context->placement : Transform::identity;
	if (_renderContext.currentContext != actualContext ||
		! Transform::same_with_orientation(_renderContext.currentPlacement, actualPlacement))
	{
		System::VideoMatrixStack& modelViewStack = _v3d->access_model_view_matrix_stack();
		if (_renderContext.currentContext)
		{
			modelViewStack.pop();
		}
		_renderContext.currentContext = actualContext;
		_renderContext.currentPlacement = actualPlacement;
		if (_renderContext.currentContext)
		{
			modelViewStack.push_to_world(_renderContext.currentPlacement.to_matrix());
		}
	}
}

//

DebugRendererRenderingOffset::DebugRendererRenderingOffset()
: offset(Transform::identity)
{
}
void DebugRendererRenderingOffset::reset()
{
	offset = Transform::identity;
}

void DebugRendererRenderingOffset::move_by_local(Vector3 const & _translation)
{
	offset.set_translation(offset.get_translation() + offset.vector_to_world(_translation));
}

void DebugRendererRenderingOffset::rotate_by_local(Rotator3 const & _rotation, bool _levelOut)
{
	if (_levelOut)
	{
		Rotator3 renderingOffset = offset.get_orientation().to_rotator();
		renderingOffset.pitch += _rotation.pitch;
		renderingOffset.yaw += _rotation.yaw;
		renderingOffset.roll = 0.0f;
		offset.set_orientation(renderingOffset.to_quat());
	}
	else
	{
		offset.set_orientation(offset.get_orientation().rotate_by(_rotation.to_quat()));
	}
}

//

void DebugRenderer::initialise_static()
{
	an_assert(s_debugRenderer == nullptr);
	s_debugRenderer = new DebugRenderer();
}

void DebugRenderer::close_static()
{
	an_assert(s_debugRenderer);
	delete_and_clear(s_debugRenderer);
}

DebugRenderer::DebugRenderer()
: gatherFrame(&frames[0])
, renderFrame(&frames[1])
{
	gatherInRendererForThread.set_size(16);
}

void DebugRenderer::ready_for_next_frame(float _deltaTime)
{
	DebugRendererUseCamera::Type keepUseCamera;
	DebugRendererCamera keepCamera;
	gatherFrame->get_camera_info(OUT_ keepUseCamera, OUT_ keepCamera);

	renderFrame->advance_clear(_deltaTime);
	if (renderFrame->is_empty())
	{
		// nothing to keep, just swap, our new render frame is what we've gathered
		swap(gatherFrame, renderFrame);
	}
	else
	{
		// add whatever came in last gather frame
		renderFrame->add_from(gatherFrame);
	}
	gatherFrame->clear();
	
	// restore camera for both
	renderFrame->set_camera_info(keepUseCamera, keepCamera);
	gatherFrame->set_camera_info(keepUseCamera, keepCamera);

	renderFrameCheckpoint = renderFrame->get_checkpoint();
}

void DebugRenderer::keep_this_frame()
{
	renderFrame->restore_checkpoint(renderFrameCheckpoint);

	// add whatever came in last gather frame
	renderFrame->add_from(gatherFrame);

	gatherFrame->clear();
}

bool DebugRenderer::should_gather_for_debug_renderer() const
{
	if (mode == DebugRendererMode::Off)
	{
		return false;
	}
	if (mode == DebugRendererMode::All)
	{
		return true;
	}
	return debug_is_filter_allowed();
}

bool DebugRenderer::is_filter_active(Name const & _filter) const
{
	if (mode == DebugRendererMode::All)
	{
		return true;
	}
	if (!filtersReady)
	{
		return false;
	}
	if (blockFilters.does_contain(_filter))
	{
		return false;
	}
	if (!allowFilters.is_empty())
	{
		if (!allowFilters.does_contain(_filter))
		{
			return false;
		}
	}
	return true;
}

void DebugRenderer::allow_filter(Name const & _filter, bool _allow)
{
	if (_allow)
	{
		allowFilters.push_back_unique(_filter);
	}
	else
	{
		allowFilters.remove_fast(_filter);
	}
}

void DebugRenderer::block_filter(Name const & _filter, bool _block)
{
	if (_block)
	{
		blockFilters.push_back_unique(_filter);
	}
	else
	{
		blockFilters.remove_fast(_filter);
	}
}

void DebugRenderer::clear_filters()
{
	allowFilters.clear();
	blockFilters.clear();
}

bool DebugRenderer::is_active_subject(void const * _subject) const
{
	return !activeSubject || !_subject || activeSubject == _subject;
}

void DebugRenderer::set_active_subject(void const * _subject)
{
	activeSubject = _subject;
}

void DebugRenderer::gather_in_rendering_frame()
{
	if (Concurrency::ThreadManager* threadManager = Concurrency::ThreadManager::get())
	{
		int threadId = threadManager->get_current_thread_id();
		if (gatherInRendererForThread.get_size() < threadId)
		{
			static Concurrency::SpinLock lock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
#ifdef AN_CONCURRENCY_STATS
			lock.do_not_report_stats();
#endif
			lock.acquire();
			while (gatherInRendererForThread.get_size() < threadManager->get_thread_count())
			{
				gatherInRendererForThread.push_back(0);
			}
			lock.release();
		}
		if (gatherInRendererForThread.is_index_valid(threadId))
		{	
			++ gatherInRendererForThread[threadId];
		}
	}
}

void DebugRenderer::gather_restore()
{
	// we should already setup
	if (gatherInRendererForThread.is_index_valid(Concurrency::ThreadManager::get_current_thread_id()))
	{
		auto& g = gatherInRendererForThread[Concurrency::ThreadManager::get_current_thread_id()];
		g = max(0, g - 1);
	}
}

DebugRendererFrame* DebugRenderer::get_gather_frame()
{
	int currentThreadID = Concurrency::ThreadManager::get_current_thread_id();
	return gatherInRendererForThread.is_index_valid(currentThreadID) && gatherInRendererForThread[currentThreadID] ? renderFrame : gatherFrame;
}

DebugRendererFrame* DebugRenderer::get_render_frame()
{
	return renderFrame;
}

void DebugRenderer::set_mode(DebugRendererMode::Type _mode)
{
	mode = _mode;
}

void DebugRenderer::set_blending(DebugRendererBlending::Type _blending)
{
	blending = _blending;
}