#pragma once

struct Colour;
struct Vector3;

#include "..\concurrency\mrswLock.h"
#include "..\containers\array.h"
#include "..\types\name.h"
#include "..\types\colour.h"
#include "..\types\optional.h"
#include "..\mesh\iMesh3dRenderBonesProvider.h"
#include "..\memory\refCountObject.h"
#include "..\types\optional.h"
#include "..\system\video\shaderProgramBindingContext.h"
#include "..\vr\vrFovPort.h"

namespace Collision
{
	class Model;
};

namespace System
{
	class Font;
	class Video3D;
	class Material;
	struct MaterialInstance;
	interface_class IMaterialInstanceProvider;
};

namespace Meshes
{
	interface_class IMesh3D;
};

namespace DebugRendererUseCamera
{
	enum Type
	{
		AsIs,
		Provided
	};
};

struct DebugRendererElement
{
	void const * context;
	void const * subject;
	Name filter;
	float timeBased;
	bool front;
	bool blended;

	DebugRendererElement() {}
	DebugRendererElement(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, bool _blended) : context(_context), subject(_subject), filter(_filter), timeBased(_timeBased), front(_front), blended(_blended) {}

	virtual ~DebugRendererElement() {}

	virtual void render(::System::Video3D * _v3d) const = 0;
};

struct DebugRendererText
: public DebugRendererElement
{
	static const int length = 100;
	Colour colour;
	Vector3 a;
	Vector2 pt;
	bool alignToCamera; // placed in world or aligned to camera, if false (default), is aligned to camera
	float scale;
	bool useScreenSpaceSize; // will be scaled with distance to remain same at screen
	tchar text[length + 1];

	DebugRendererText() {}
	DebugRendererText(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Colour const & _colour, Vector3 const & _a, Vector2 const & _pt, bool _alignToCamera, float _scale, bool _useScreenSpaceSize) : DebugRendererElement(_context, _subject, _filter, _timeBased, _front, _colour.a < 1.0f), colour(_colour), a(_a), pt(_pt), alignToCamera(_alignToCamera), scale(_scale), useScreenSpaceSize(_useScreenSpaceSize) { text[0] = 0; }

	override_ void render(::System::Video3D * _v3d) const;
};

struct DebugRendererLine
: public DebugRendererElement
{
	Colour colour;
	Vector3 a, b;

	DebugRendererLine() {}
	DebugRendererLine(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b) : DebugRendererElement(_context, _subject, _filter, _timeBased, _front, _colour.a < 1.0f), colour(_colour), a(_a), b(_b) {}

	override_ void render(::System::Video3D * _v3d) const;
};

struct DebugRendererTriangle
: public DebugRendererElement
{
	Colour colour;
	Vector3 a, b, c;

	DebugRendererTriangle() {}
	DebugRendererTriangle(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c) : DebugRendererElement(_context, _subject, _filter, _timeBased, _front, _colour.a < 1.0f), colour(_colour), a(_a), b(_b), c(_c) {}

	override_ void render(::System::Video3D * _v3d) const;
};

struct DebugRendererMaterialInstance
: public RefCountObject
{
	::System::MaterialInstance* materialInstance = nullptr;

	DebugRendererMaterialInstance() {}
	DebugRendererMaterialInstance(::System::MaterialInstance* _materialInstance) :materialInstance(_materialInstance) {}

	virtual ~DebugRendererMaterialInstance();
};

struct DebugRendererMesh3D
: public DebugRendererElement
, public Meshes::IMesh3DRenderBonesProvider
{
	Meshes::IMesh3D const * mesh;
	Transform placement;
	bool singleMaterial;
	Array<RefCountObjectPtr<DebugRendererMaterialInstance>> materialInstances;
	Array<Matrix44> bones;

	DebugRendererMesh3D();
	DebugRendererMesh3D(DebugRendererMesh3D const & _debugRendererMesh3D);
	DebugRendererMesh3D(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Meshes::IMesh3D const * _mesh, Transform const & _placement, ::System::Material const*_material, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider);
	DebugRendererMesh3D(void const * _context, void const * _subject, Name const & _filter, float _timeBased, bool _front, Meshes::IMesh3D const * _mesh, Transform const & _placement, ::System::IMaterialInstanceProvider const * _materialInstanceProvider, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider);
	~DebugRendererMesh3D();

	override_ void render(::System::Video3D * _v3d) const;

public:	// IMesh3DRenderBonesProvider
	override_ Matrix44 const * get_render_bone_matrices(OUT_ int & _matricesCount) const
	{
		_matricesCount = bones.get_size();
		return bones.get_data();
	}

private:
	void copy_bones_from(Meshes::IMesh3DRenderBonesProvider const * _bonesProvider);
};

struct DebugRendererCamera
{
	Transform placement;
	float fov;
	Optional<VRFovPort> fovPort;
	bool orthogonal = false;
	float orthogonalScale = 1.0f;

	DebugRendererCamera(): placement(Transform::identity), fov(90.0f) {}
	DebugRendererCamera(Transform const & _placement, float _fov, Optional<VRFovPort> const& _fovPort) : placement(_placement), fov(_fov), fovPort(_fovPort), orthogonal(false) {}
	DebugRendererCamera(Transform const & _placement, bool _orthogonal, float _orthogonalScale) : placement(_placement), fov(0.0f), orthogonal(_orthogonal), orthogonalScale(_orthogonalScale) {}
};

struct DebugRendererContext
{
	void const * context;
	Transform placement;

	DebugRendererContext() {}
	DebugRendererContext(void const * _context, Transform const & _placement) : context(_context), placement(_placement) {}
};

class DebugRenderer;

struct DebugRendererRenderingContext
{
	void const * currentContext; // for both gathering and rendering
	Transform currentPlacement; // when gathering, it is top of transformStack, for rendering it is context

	DebugRendererRenderingContext();
};

struct DebugRendererGatheringContext
{
	void const * currentContext;
	void const * currentSubject;
	Name currentFilter;
	Array<Name> filterStack;
	Transform currentPlacement; // top of transformStack
	Array<Transform> transformStack;
	float timeBased = 0.0f;

	DebugRendererGatheringContext();
};

struct DebugRendererFrameCheckpoint
{
	int lines = 0;
	int triangles = 0;
	int meshes = 0;
	int texts = 0;
};

struct DebugRendererFrame
{
public:
	DebugRendererFrame(bool _reserveMemory = true);
	void clear();
	void clear_non_mesh();
	void advance_clear(float _deltaTime); // will advance time based and remove those that should not be there
	void render(DebugRenderer * _renderer, ::System::Video3D* _v3d, std::function<void()> _do_after_camera_is_set);

	DebugRendererFrameCheckpoint get_checkpoint();
	void restore_checkpoint(DebugRendererFrameCheckpoint const & _checkpoint);
	void apply_transform(DebugRendererFrameCheckpoint const & _sinceCheckpoint, Transform const & _transform);

	void mark_rendered(bool _rendered);

	bool is_empty() const { return lines.is_empty() && triangles.is_empty() && meshes.is_empty() && texts.is_empty(); }

	void add_from(DebugRendererFrame const * _frame);

	// TODO thread safety!
	// have copies of all frames
	// define and undefine context for all frames
	void set_context(void const * _context = nullptr);
	void set_subject(void const * _subject = nullptr);
	void set_filter(Name const & _filter = Name::invalid());
	void push_filter(Name const & _filter);
	void pop_filter();
	Name const & get_filter();
	void define_context(void const * _context, Transform const & _placement);
	void undefine_contexts();

	void push_transform(Transform const & _placement);
	void pop_transform();

	void be_time_based(float _timeBased = 0.0f);

	void add_text(bool _front, Optional<Colour> const & _colour, Vector3 const & _a, Optional<Vector2> const & _pt, Optional<bool> const & _alignToCamera, Optional<float> const & _scale, Optional<bool> const & _useScreenSpaceSize, tchar const * const _text, ...);
	void add_line(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b);
	void add_arrow(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, float _size = 0.0f, bool _doubleSide = false); // if size is zero, it is auto calculated
	void add_segment(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, float _size = 0.0f); // if size is zero, it is auto calculated
	void add_triangle(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c);
	void add_triangle_border(bool _front, Colour const & _colour, Vector3 const & _a, Vector3 const & _b, Vector3 const & _c);
	void add_matrix(bool _front, Matrix44 const & _matrix, float _size = 1.0f, Colour const & _colourX = Colour::red, Colour const & _colourY = Colour::green, Colour const & _colourZ = Colour::blue, float _rgbAt = 1.0f);
	void add_mesh(bool _front, Meshes::IMesh3D* _mesh, Transform const & _placement, ::System::Material const * _material, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider);
	void add_mesh(bool _front, Meshes::IMesh3D* _mesh, Transform const & _placement, ::System::IMaterialInstanceProvider const* _materialInstanceProvider, ::Meshes::IMesh3DRenderBonesProvider const* _bonesProvider);
	void add_collision_model(bool _frontBorder, bool _frontFill, ::Collision::Model* _cm, Colour const & _colour, float _alphaFill, Transform const & _placement);
	void add_box(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Box const & _box);
	void add_convex_hull(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, ConvexHull const & _convexHull);
	void add_sphere(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Sphere const & _sphere);
	void add_capsule(bool _frontBorder, bool _frontFill, Colour const & _colour, float _alphaFill, Capsule const & _capsule);

	void set_camera(Transform const & _placement, float _fov, Optional<VRFovPort> const & _fovPort);
	void set_camera_orthogonal(Transform const & _placement, float _orthogonalScale);

	void get_camera_info(OUT_ DebugRendererUseCamera::Type& _useCamera, OUT_ DebugRendererCamera& _camera) const;
	void set_camera_info(DebugRendererUseCamera::Type const & _useCamera, DebugRendererCamera const& _camera);

private:
	Concurrency::SpinLock contextsLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
	Concurrency::SpinLock linesLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
	Concurrency::SpinLock trianglesLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
	Concurrency::SpinLock meshesLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);
	Concurrency::SpinLock textsLock = Concurrency::SpinLock(SYSTEM_SPIN_LOCK);

	bool rendered;

	Array<DebugRendererGatheringContext> gatheringContexts;
	DebugRendererGatheringContext defaultGatheringContexts;

	Array<DebugRendererContext> contexts;
	Array<DebugRendererLine> lines;
	Array<DebugRendererTriangle> triangles;
	Array<DebugRendererMesh3D> meshes;
	Array<DebugRendererText> texts;

	DebugRendererUseCamera::Type useCamera;
	DebugRendererCamera camera;

	DebugRendererGatheringContext& access_gathering_context();

	bool requires_switching_context(REF_ DebugRendererRenderingContext & _renderContext, void const * _context, int _contextIndex); // returns true if requires switching context
	bool switch_context(::System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, void const * _context = nullptr, int * _contextIndex = nullptr); // returns true if has context
	void switch_to_actual_context(::System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, DebugRendererContext const * _context);

	template <typename Element>
	void render_all_of(DebugRenderer * _renderer, ::System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, bool _renderFront, bool _renderBlended, Array<Element> const & _elements);

	void render_lines(DebugRenderer * _renderer, System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, bool _renderFront, bool _renderBlended);
	void render_triangles(DebugRenderer * _renderer, System::Video3D* _v3d, REF_ DebugRendererRenderingContext & _renderContext, bool _renderFront, bool _renderBlended);

	inline bool should_gather_for_debug_renderer() const;

	template <typename Class>
	bool advance_clear(Array<Class> & _array, float _deltaTime);
};

struct DebugRendererRenderingOffset
{
public:
	DebugRendererRenderingOffset();
	void reset();
	void move_by_local(Vector3 const & _translation);
	void rotate_by_local(Rotator3 const & _rotation, bool _levelOut);

	Transform const & get_offset() const { return offset; }
	Transform & access_offset() { return offset; }

private:
	Transform offset;
};

namespace DebugRendererMode
{
	enum Type
	{
		Off,
		OnlyActiveFilters,
		All
	};
};

namespace DebugRendererBlending
{
	enum Type
	{
		Off,
		NoTriangles,
		On
	};
};

class DebugRenderer
{
public:
	static DebugRenderer* get() { return s_debugRenderer; }

	static void initialise_static();
	static void close_static();

	void use_font(::System::Font* _font) { font = _font; }
	::System::Font* get_font() { return font; }

	void ready_for_next_frame(float _deltaTime);
	void keep_this_frame();

	void set_mode(DebugRendererMode::Type _mode);
	DebugRendererMode::Type get_mode() const { return mode; }

	void set_blending(DebugRendererBlending::Type _blending);
	DebugRendererBlending::Type get_blending() const { return blending; }

	::System::ShaderProgramBindingContext& get_shader_program_binding_context() { return shaderProgramBindingContext; }

	DebugRendererFrame* get_gather_frame();
	DebugRendererFrame* get_render_frame();

	bool does_allow_filter(Name const & _filter) { return is_filter_active(_filter); }

	void allow_filter(Name const & _filter, bool _allow);
	void block_filter(Name const & _filter, bool _block);
	void clear_filter(Name const & _filter);
	void clear_filters();

	void mark_filters_ready() { filtersReady = true; }

	void set_active_subject(void const * _subject);

	DebugRendererRenderingOffset& access_rendering_offset() { return renderingOffset; }
	void set_fov_override(float _fov) { fovOverride = _fov; }
	void clear_fov_override() { fovOverride.clear(); }
	void apply_fov_override(REF_ float & _fov) const { if (fovOverride.is_set()) { _fov = fovOverride.get(); } }

	void gather_in_rendering_frame();
	void gather_restore();

private:
	static DebugRenderer* s_debugRenderer;

	::System::Font* font = nullptr;

	DebugRendererBlending::Type blending =
#ifdef AN_ANDROID
											DebugRendererBlending::NoTriangles
#else
											DebugRendererBlending::On
#endif
											;
	DebugRendererMode::Type mode = DebugRendererMode::OnlyActiveFilters;

	Array<int> gatherInRendererForThread;

	::System::ShaderProgramBindingContext shaderProgramBindingContext;

	DebugRendererFrame frames[2];
	DebugRendererFrame* gatherFrame;
	DebugRendererFrame* renderFrame;
	DebugRendererFrameCheckpoint renderFrameCheckpoint;

	bool filtersReady = false;
	Array<Name> allowFilters; // if non-empty, only allowed are rendered, if empty, all
	Array<Name> blockFilters; // if blocked, will be skipped - always
	void const * activeSubject = nullptr;

	DebugRendererRenderingOffset renderingOffset;
	Optional<float> fovOverride;

	DebugRenderer();

	bool is_filter_active(Name const & _filter) const;
	bool is_active_subject(void const * _subject) const;

	inline bool should_gather_for_debug_renderer() const;

	friend struct DebugRendererFrame;
};

#ifndef AN_DEBUG_RENDERER
	#define debug_renderer_use_font(font)

	#define debug_camera(placement, fov, fovPort)
	#define debug_camera_orthogonal(placement, orthogonalScale)

	#define debug_renderer_mode(_mode)
	#define debug_renderer_get_mode() DebugRendererMode::Off

	#define debug_renderer_blending(_blending)
	#define debug_renderer_get_blending() DebugRendererBlending::Off

	#define debug_gather_in_renderer()
	#define debug_gather_restore()

	#define debug_context(context)
	#define debug_no_context()

	#define debug_subject(subject)
	#define debug_no_subject()

	#define debug_is_filter_allowed() (false)
	#define debug_filter(filter)
	#define debug_no_filter()
	#define debug_push_filter(filter)
	#define debug_pop_filter()

	#define debug_clear()

	#define debug_push_transform(placement)
	#define debug_pop_transform()

	// debug_draw_time_based(0.2f, debug_draw_line(true, Colour::red, a, b));
	#define debug_draw_time_based(_time, _debug_draw_call)

	#define debug_draw_text(front, colour,a,pt,...)
	#define debug_draw_line(front,colour,a,b)
	#define debug_draw_arrow(front,colour,a,b)
	#define debug_draw_arrow_double_side(front,colour,a,b)
	#define debug_draw_arrow_size(front,colour,a,b)
	#define debug_draw_segment(front,colour,a,b)
	#define debug_draw_segment_size(front,colour,a,b,size)
	#define debug_draw_triangle(front,colour,a,b,c)
	#define debug_draw_triangle_border(front,colour,a,b,c)
	#define debug_draw_transform(front,transform)
	#define debug_draw_transform_coloured(front,transform,colourX,colourY,colourZ)
	#define debug_draw_transform_size(front,transform, size)
	#define debug_draw_transform_size_coloured(front,transform,size,colourX,colourY,colourZ)
	#define debug_draw_transform_size_coloured_lerp(front,transform,size,colour,lerpAmount)
	#define debug_draw_matrix(front,matrix)
	#define debug_draw_matrix_coloured(front,matrix,colourX,colourY,colourZ)
	#define debug_draw_matrix_size(front,matrix, size)
	#define debug_draw_matrix_size_coloured(front,matrix,size,colourX,colourY,colourZ)
	#define debug_draw_mesh(front,mesh,placement,material)
	#define debug_draw_mesh_bones(front,mesh,placement,material,bonesProvider)
	#define debug_draw_collision_model(frontBorder,frontFill,cm,colour,alphaFill,placement)
	#define debug_draw_box(frontBorder,frontFill,colour,alphaFill,box)
	#define debug_draw_convex_hull(frontBorder,frontFill,colour,alphaFill,convexHull)
	#define debug_draw_sphere(frontBorder,frontFill,colour,alphaFill,sphere)
	#define debug_draw_capsule(frontBorder,frontFill,colour,alphaFill,capsule)

	#define debug_renderer_define_context(context, placement)
	#define debug_renderer_undefine_contexts()

	#define debug_renderer_shader_program_binding_context() nullptr

	#define debug_renderer_next_frame_clear()
	#define debug_renderer_next_frame(_deltaTime)
	#define debug_renderer_render(v3d)
	#define debug_renderer_render_do_after_camera(v3d, do_after_camera)
	#define debug_renderer_not_yet_rendered()
	#define debug_renderer_already_rendered()

	#define debug_renderer_enable_filter(filter)
	#define debug_renderer_disable_filter(filter)
	#define debug_renderer_clear_filters()

	#define debug_renderer_rendering_offset_reset()
	#define debug_renderer_rendering_offset_move_by_local(translation)
	#define debug_renderer_rendering_offset_rotate_by_local(rotation, levelOut)
	#define debug_renderer_rendering_offset() Transform::identity
	#define debug_renderer_set_fov(fov)
	#define debug_renderer_clear_fov()
	#define debug_renderer_apply_fov_override(fovRef)
#else
	#define debug_renderer_use_font(font) DebugRenderer::get()->use_font(font)

	#define debug_camera(placement, fov, fovPort) DebugRenderer::get()->get_gather_frame()->set_camera(placement, fov, fovPort)
	#define debug_camera_orthogonal(placement, orthogonalScale) DebugRenderer::get()->get_gather_frame()->set_camera_orthogonal(placement, orthogonalScale)

	#define debug_renderer_mode(_mode) DebugRenderer::get()->set_mode(_mode)
	#define debug_renderer_get_mode() DebugRenderer::get()->get_mode()

	#define debug_renderer_blending(_blending) DebugRenderer::get()->set_blending(_blending)
	#define debug_renderer_get_blending() DebugRenderer::get()->get_blending()

	// this is to make it possible to gather data during scene building
	#define debug_gather_in_renderer() DebugRenderer::get()->gather_in_rendering_frame()
	#define debug_gather_restore() DebugRenderer::get()->gather_restore()

	#define debug_context(context) DebugRenderer::get()->get_gather_frame()->set_context(context)
	#define debug_no_context() DebugRenderer::get()->get_gather_frame()->set_context()

	#define debug_subject(subject) DebugRenderer::get()->get_gather_frame()->set_subject(subject)
	#define debug_no_subject() DebugRenderer::get()->get_gather_frame()->set_subject()

	#define debug_is_filter_allowed() (DebugRenderer::get()->does_allow_filter(DebugRenderer::get()->get_gather_frame()->get_filter()))
	#define debug_filter(filter) { DEFINE_STATIC_NAME(filter); DebugRenderer::get()->get_gather_frame()->set_filter(NAME(filter)); }
	#define debug_no_filter() DebugRenderer::get()->get_gather_frame()->set_filter()
	#define debug_push_filter(filter) { DEFINE_STATIC_NAME(filter); DebugRenderer::get()->get_gather_frame()->push_filter(NAME(filter)); }
	#define debug_pop_filter() DebugRenderer::get()->get_gather_frame()->pop_filter()

	#define debug_clear() DebugRenderer::get()->get_gather_frame()->clear()

	#define debug_push_transform(placement) DebugRenderer::get()->get_gather_frame()->push_transform(placement)
	#define debug_pop_transform() DebugRenderer::get()->get_gather_frame()->pop_transform()

	#define debug_add_frame(frame) DebugRenderer::get()->get_gather_frame()->add_from(frame)

	// debug_draw_time_based(0.2f, debug_draw_line(true, Colour::red, a, b));
	#define debug_draw_time_based(_time, _debug_draw_call) { DebugRenderer::get()->get_gather_frame()->be_time_based(_time); _debug_draw_call; DebugRenderer::get()->get_gather_frame()->be_time_based(); }

	#define debug_draw_text(front,colour,a,pt,alignToCamera,scale,useScreenSpaceSize,...) DebugRenderer::get()->get_gather_frame()->add_text(front, colour, a, pt, alignToCamera, scale, useScreenSpaceSize, ##__VA_ARGS__)
	#define debug_draw_line(front,colour,a,b) DebugRenderer::get()->get_gather_frame()->add_line(front, colour, a, b)
	#define debug_draw_arrow(front,colour,a,b) DebugRenderer::get()->get_gather_frame()->add_arrow(front, colour, a, b)
	#define debug_draw_arrow_double_side(front,colour,a,b) DebugRenderer::get()->get_gather_frame()->add_arrow(front, colour, a, b, 0.0f, true)
	#define debug_draw_arrow_size(front,colour,a,b,size) DebugRenderer::get()->get_gather_frame()->add_arrow(front, colour, a, b, size)
	#define debug_draw_segment(front,colour,a,b) DebugRenderer::get()->get_gather_frame()->add_segment(front, colour, a, b, 0.0f)
	#define debug_draw_segment_size(front,colour,a,b,size) DebugRenderer::get()->get_gather_frame()->add_segment(front, colour, a, b, size)
	#define debug_draw_triangle(front,colour,a,b,c) DebugRenderer::get()->get_gather_frame()->add_triangle(front, colour, a, b, c)
	#define debug_draw_triangle_border(front,colour,a,b,c) DebugRenderer::get()->get_gather_frame()->add_triangle_border(front, colour, a, b, c)
	#define debug_draw_transform(front,transform) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (transform).to_matrix())
	#define debug_draw_transform_coloured(front,transform,colourX,colourY,colourZ) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (transform).to_matrix(), 1.0f, colourX, colourY, colourZ, 0.8f)
	#define debug_draw_transform_size(front,transform, size) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (transform).to_matrix(), size)
	#define debug_draw_transform_size_coloured(front,transform,size,colourX,colourY,colourZ) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (transform).to_matrix(), size, colourX, colourY, colourZ, 0.8f)
	#define debug_draw_transform_size_coloured_lerp(front,transform,size,colour,lerpAmount) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (transform).to_matrix(), size, Colour::lerp(lerpAmount, Colour::red, colour), Colour::lerp(lerpAmount, Colour::green, colour), Colour::lerp(lerpAmount, Colour::blue, colour), 0.8f)
	#define debug_draw_matrix(front,matrix) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (matrix))
	#define debug_draw_matrix_coloured(front,matrix,colourX,colourY,colourZ) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (matrix), 1.0f, colourX, colourY, colourZ, 0.8f)
	#define debug_draw_matrix_size(front,matrix, size) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (matrix), size)
	#define debug_draw_matrix_size_coloured(front,matrix,size,colourX,colourY,colourZ) DebugRenderer::get()->get_gather_frame()->add_matrix(front, (matrix), size, colourX, colourY, colourZ, 0.8f)
	#define debug_draw_mesh(front,mesh,placement,material) DebugRenderer::get()->get_gather_frame()->add_mesh(front, mesh, placement, material)
	#define debug_draw_mesh_bones(front,mesh,placement,material,bonesProvider) DebugRenderer::get()->get_gather_frame()->add_mesh(front, mesh, placement, material, bonesProvider)
	#define debug_draw_collision_model(frontBorder,frontFill,cm,colour,alphaFill,placement) DebugRenderer::get()->get_gather_frame()->add_collision_model(frontBorder, frontFill, cm, colour, alphaFill, placement)
	#define debug_draw_box(frontBorder,frontFill,colour,alphaFill,box) DebugRenderer::get()->get_gather_frame()->add_box(frontBorder, frontFill, colour, alphaFill, box)
	#define debug_draw_convex_hull(frontBorder,frontFill,colour,alphaFill,convexHull) DebugRenderer::get()->get_gather_frame()->add_convex_hull(frontBorder, frontFill, colour, alphaFill, convexHull)
	#define debug_draw_sphere(frontBorder,frontFill,colour,alphaFill,sphere) DebugRenderer::get()->get_gather_frame()->add_sphere(frontBorder, frontFill, colour, alphaFill, sphere)
	#define debug_draw_capsule(frontBorder,frontFill,colour,alphaFill,capsule) DebugRenderer::get()->get_gather_frame()->add_capsule(frontBorder, frontFill, colour, alphaFill, capsule)

	#define debug_renderer_define_context(context, placement) DebugRenderer::get()->get_render_frame()->define_context(context, placement)
	#define debug_renderer_undefine_contexts() DebugRenderer::get()->get_render_frame()->undefine_contexts()

	#define debug_renderer_shader_program_binding_context() (&DebugRenderer::get()->get_shader_program_binding_context())

	#define debug_renderer_next_frame_clear() DebugRenderer::get()->ready_for_next_frame(9999999.0f)
	#define debug_renderer_next_frame(_deltaTime) DebugRenderer::get()->ready_for_next_frame(_deltaTime)
	#define debug_renderer_keep_frame() DebugRenderer::get()->keep_this_frame()
	#define debug_renderer_render(v3d) DebugRenderer::get()->get_render_frame()->render(DebugRenderer::get(), v3d, nullptr)
	#define debug_renderer_render_do_after_camera(v3d, do_after_camera) DebugRenderer::get()->get_render_frame()->render(DebugRenderer::get(), v3d, do_after_camera)
	#define debug_renderer_not_yet_rendered() DebugRenderer::get()->get_render_frame()->mark_rendered(false)
	#define debug_renderer_already_rendered() DebugRenderer::get()->get_render_frame()->mark_rendered(true)

	#define debug_renderer_enable_filter(filter) DebugRenderer::get()->enable_filter(filter)
	#define debug_renderer_disable_filter(filter) DebugRenderer::get()->disable_filter(filter)
	#define debug_renderer_clear_filters() DebugRenderer::get()->clear_filters()

	#define debug_renderer_rendering_offset_reset() DebugRenderer::get()->access_rendering_offset().reset()
	#define debug_renderer_rendering_offset_move_by_local(translation) DebugRenderer::get()->access_rendering_offset().move_by_local(translation)
	#define debug_renderer_rendering_offset_rotate_by_local(rotation, levelOut) DebugRenderer::get()->access_rendering_offset().rotate_by_local(rotation, levelOut)
	#define debug_renderer_rendering_offset() DebugRenderer::get()->access_rendering_offset().access_offset()
	#define debug_renderer_set_fov(fov) DebugRenderer::get()->set_fov_override(fov)
	#define debug_renderer_clear_fov() DebugRenderer::get()->clear_fov_override()
	#define debug_renderer_apply_fov_override(fovRef) DebugRenderer::get()->apply_fov_override(fovRef)
#endif

Colour get_debug_highlight_colour();
Colour get_debug_highlight_colour(float _pulseLength);
