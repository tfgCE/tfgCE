#include "editorBase3D.h"

#include "editorBaseImpl.inl"
#include "..\..\game\game.h"

#include "..\..\ui\uiCanvas.h"
#include "..\..\ui\uiCanvasInputContext.h"

#include "..\..\ui\uiCanvasButton.h"
#include "..\..\ui\uiCanvasWindow.h"

#include "..\..\ui\utils\uiUtils.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\other\simpleVariableStorage.h"
#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\video3d.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define VIEW_ROTATOR_TOP Rotator3(-90.0f, 0.0f, 0.0f)
#define VIEW_ROTATOR_BOTTOM Rotator3(90.0f, 180.0f, 0.0f)
#define VIEW_ROTATOR_BACK Rotator3(0.0f, 0.0f, 0.0f)
#define VIEW_ROTATOR_FRONT Rotator3(0.0f, 180.0f, 0.0f)
#define VIEW_ROTATOR_RIGHT Rotator3(0.0f, 90.0f, 0.0f)
#define VIEW_ROTATOR_LEFT Rotator3(0.0f, -90.0f, 0.0f)

//

using namespace Framework;

//

// settings
DEFINE_STATIC_NAME(editor3D_camera_pivot);
DEFINE_STATIC_NAME(editor3D_camera_perspectiveUp);
DEFINE_STATIC_NAME(editor3D_camera_perspectiveDistance);
DEFINE_STATIC_NAME(editor3D_camera_perspectiveRotation);
DEFINE_STATIC_NAME(editor3D_camera_fov);
DEFINE_STATIC_NAME(editor3D_camera_orthogonal);
DEFINE_STATIC_NAME(editor3D_camera_orthogonalRotation);
DEFINE_STATIC_NAME(editor3D_camera_orthogonalAxis);
DEFINE_STATIC_NAME(editor3D_camera_orthogonalScale);
DEFINE_STATIC_NAME(editor3D_camera_lastOrthogonalTop);
DEFINE_STATIC_NAME(editor3D_camera_lastOrthogonalBack);
DEFINE_STATIC_NAME(editor3D_camera_lastOrthogonalLeft);

DEFINE_STATIC_NAME(editor3D_grid_on);
DEFINE_STATIC_NAME(editor3D_grid_size);
DEFINE_STATIC_NAME(editor3D_grid_start);
DEFINE_STATIC_NAME(editor3D_grid_x);
DEFINE_STATIC_NAME(editor3D_grid_y);

DEFINE_STATIC_NAME(editor3D_axes_on);

DEFINE_STATIC_NAME(editor3D_gizmos_on);

// ui
DEFINE_STATIC_NAME(editor3D_grid);
DEFINE_STATIC_NAME(editor3D_axes);
DEFINE_STATIC_NAME(editor3D_gizmos);
DEFINE_STATIC_NAME(editor3D_persp);
DEFINE_STATIC_NAME(editor3D_top);
DEFINE_STATIC_NAME(editor3D_bottom);
DEFINE_STATIC_NAME(editor3D_back);
DEFINE_STATIC_NAME(editor3D_front);
DEFINE_STATIC_NAME(editor3D_left);
DEFINE_STATIC_NAME(editor3D_right);

//

REGISTER_FOR_FAST_CAST(Editor::Base3D);

Editor::Base3D::Base3D()
{
	camera.perspectiveRotation = look_at_matrix(camera.pivot + Vector3(1.0f, -1.0f, 0.5f) * 2.0f, camera.pivot, Vector3::zAxis).to_transform().get_orientation();
	camera.perspectiveDistance = 5.0f;

	camera.update_at();
}

Editor::Base3D::~Base3D()
{
}

#define STORE_RESTORE(_op) \
	_op(_setup, camera.pivot, NAME(editor3D_camera_pivot)); \
	_op(_setup, camera.perspectiveUp, NAME(editor3D_camera_perspectiveUp)); \
	_op(_setup, camera.perspectiveDistance, NAME(editor3D_camera_perspectiveDistance)); \
	_op(_setup, camera.perspectiveRotation, NAME(editor3D_camera_perspectiveRotation)); \
	_op(_setup, camera.fov, NAME(editor3D_camera_fov)); \
	_op(_setup, camera.orthogonal, NAME(editor3D_camera_orthogonal)); \
	_op(_setup, camera.orthogonalRotation, NAME(editor3D_camera_orthogonalRotation)); \
	_op(_setup, camera.orthogonalAxis, NAME(editor3D_camera_orthogonalAxis)); \
	_op(_setup, camera.orthogonalScale, NAME(editor3D_camera_orthogonalScale)); \
	_op(_setup, camera.lastOrthogonalTop, NAME(editor3D_camera_lastOrthogonalTop)); \
	_op(_setup, camera.lastOrthogonalBack, NAME(editor3D_camera_lastOrthogonalBack)); \
	_op(_setup, camera.lastOrthogonalLeft, NAME(editor3D_camera_lastOrthogonalLeft)); \
	\
	_op(_setup, grid.on, NAME(editor3D_grid_on)); \
	_op(_setup, grid.size, NAME(editor3D_grid_size)); \
	_op(_setup, grid.start, NAME(editor3D_grid_start)); \
	_op(_setup, grid.x, NAME(editor3D_grid_x)); \
	_op(_setup, grid.y, NAME(editor3D_grid_y)); \
	\
	_op(_setup, axes.on, NAME(editor3D_axes_on)); \
	\
	_op(_setup, gizmos.on, NAME(editor3D_gizmos_on)); \
	;

void Editor::Base3D::restore_for_use(SimpleVariableStorage const& _setup)
{
	base::restore_for_use(_setup);
	STORE_RESTORE(read);
	camera.update_at();
}

void Editor::Base3D::store_for_later(SimpleVariableStorage & _setup) const
{
	base::store_for_later(_setup);
	STORE_RESTORE(write);
}

void Editor::Base3D::process_controls()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	{
		for (int i = 0; i < gizmos.gizmos.get_size(); ++i)
		{
			if (!gizmos.gizmos[i].get())
			{
				gizmos.gizmos.remove_at(i);
				break; // one by one
			}
		}
	}

	UI::CanvasInputContext cic;
	cic.set_cursor(UI::CursorIndex::Mouse, UI::CanvasInputContext::Cursor::use_mouse(canvas));

	float deltaTime = ::System::Core::get_raw_delta_time();

	inputGrabbed.clear();

	controls.cursors.set_size(max(controls.cursors.get_size(), cic.get_cursors().get_size()));
	for_every(c, controls.cursors)
	{
		int cIdx = for_everys_index(c);
		if (c->buttons.is_empty())
		{
			c->buttons.set_size(c->buttons.get_max_size());
		}
		Optional<Vector2> at;
		Optional<Vector2> movedBy;
		if (cic.get_cursors().is_index_valid(cIdx))
		{
			auto& cur = cic.get_cursors()[cIdx];
			if (cur.at.is_set())
			{
				at = canvas->logical_to_real_location(cur.at.get());
			}
			if (cur.movedBy.is_set())
			{
				movedBy = canvas->logical_to_real_vector(cur.movedBy.get());
			}
		}
		Vector3 curAt = cursor_ray_start(at);
		Vector3 curDir = cursor_ray_dir(at);
		bool anyButtonHeld = false;
		bool anyCursorAvailableOutsideCanvas = false;
		for_every(b, c->buttons)
		{
			float prevTimeDown = b->timeDown;
			//float prevTimeUp = b->timeUp;
			bool ignoreInput = false;
			{
				bool isDown = false;
				if (cic.get_cursors().is_index_valid(cIdx))
				{
					auto& cur = cic.get_cursors()[cIdx];
					isDown = is_flag_set(cur.buttonFlags, bit(for_everys_index(b)));
				}
				if (!canvas->is_cursor_available_outside_canvas(cIdx))
				{
					ignoreInput = true;
				}
				else
				{
					anyCursorAvailableOutsideCanvas = true;
				}
				b->isDown = isDown;
			}
			if (b->isDown)
			{
				b->timeDown += deltaTime;
			}
			else
			{
				b->timeUp += deltaTime;
			}
			float const clickThreshold = 0.3f;
			if (b->wasDown != b->isDown)
			{
				b->wasDown = b->isDown;
				bool wasHeld = b->held;
				if (b->held && (!b->isDown || ignoreInput))
				{
					c->grabbedGizmo.clear();
					on_release(cIdx, for_everys_index(b), curAt, curDir);
					b->held = false;
				}
				if (!ignoreInput)
				{
					if (b->isDown)
					{
						b->timeDown = 0.0f;
						if (! ignoreInput)
						{
							on_press(cIdx, for_everys_index(b), curAt, curDir);
							if (c->hoveringOverGizmo.get())
							{
								c->grabbedGizmo = c->hoveringOverGizmo;
								inputGrabbed = false;
								b->held = true;
							}
							else
							{
								b->held = on_hold_early(cIdx, for_everys_index(b), curAt, curDir);
							}
						}
						else
						{
							b->held = false;
						}
					}
					else
					{
						b->timeUp = 0.0f;
						if (!ignoreInput)
						{
							if (b->timeDown < clickThreshold && (!wasHeld || allow_click_after_hold_early(cIdx, for_everys_index(b))))
							{
								on_click(cIdx, for_everys_index(b), curAt, curDir);
							}
						}
					}
				}
			}
			else
			{
				if (b->isDown)
				{
					if (!ignoreInput)
					{
						if (b->held)
						{
							Vector2 movedBy2dNormalised = cursor_moved_by_2d_normalised(movedBy);
							if (auto* g = c->grabbedGizmo.get())
							{
								Vector3 movedBy = cursor_moved_by_2d_normalised_into_3d(movedBy2dNormalised, g->atWorld);
								if (g->axisWorld.is_set())
								{
									Vector3 axis = g->axisWorld.get().normal();
									if (camera.orthogonal && abs(Vector3::dot(movedBy, axis)) < 0.0001f)
									{
										g->atWorld += axis * (movedBy2dNormalised.x + movedBy2dNormalised.y) / camera.orthogonalScale;
									}
									else
									{
										g->atWorld += movedBy.along_normalised(axis);
									}
								}
								else if (g->planeNormalWorld.is_set())
								{
									Vector3 planeAxis = g->planeNormalWorld.get().normal();
									g->atWorld += movedBy.drop_using_normalised(planeAxis);
								}
								else if (g->value.is_set())
								{
									g->value = g->value.get() + (movedBy2dNormalised.x + movedBy2dNormalised.y) * g->valueScale.get(1.0f);
								}
							}
							else
							{
								b->held = on_held(cIdx, for_everys_index(b), curAt, curDir, cursor_moved_by_2d_normalised_into_3d(movedBy2dNormalised), movedBy2dNormalised);
							}
						}
						if (b->timeDown >= clickThreshold &&
							prevTimeDown < clickThreshold)
						{
							if (c->hoveringOverGizmo.get())
							{
								// should be already grabbed
								// ignore clicking on gizmo
								//c->grabbedGizmo = c->hoveringOverGizmo;
								//inputGrabbed = false;
								//b->held = true;
							}
							else
							{
								b->held = on_hold(cIdx, for_everys_index(b), curAt, curDir);
							}
						}
					}
				}
			}

			if (b->held)
			{
				anyButtonHeld = true;
				inputGrabbed.set_if_not_set(true); // any derived class may implicitly say not to grab
			}
		}

		if (auto* g = c->grabbedGizmo.get())
		{
			bool exists = false;
			for_every_ref(gizmo, gizmos.gizmos)
			{
				if (gizmo == g)
				{
					exists = true;
					break;
				}
			}
			if (!exists)
			{
				c->grabbedGizmo.clear();
			}
		}
		else if (gizmos.on)
		{
			Gizmo3D* hoveringOver = nullptr;

			for_every_ref(gizmo, gizmos.gizmos)
			{
				if (gizmo->grabbable)
				{
					Vector3 s = curAt;
					Vector3 d = curDir;
					Vector3 p = gizmo->atWorld;

					Vector3 s2p = p - s;
					Vector3 s2pAlong = s2p.along_normalised(d);
					Vector3 closest = s + s2pAlong;

					float dist = Vector3::distance(closest, p);

					if (dist < gizmo->radiusWorld)
					{
						hoveringOver = gizmo;
					}
				}
			}

			c->hoveringOverGizmo = hoveringOver;
		}

		if (!anyButtonHeld && anyCursorAvailableOutsideCanvas)
		{
			on_hover(cIdx, curAt, curDir);

			if (!canvas->is_modal_window_active())
			{
				if (auto* input = ::System::Input::get())
				{
#ifdef AN_STANDARD_INPUT
					float const MOUSE_WHEEL_ZOOM_SPEED = 0.2f;
					if (input->has_key_been_pressed(System::Key::MouseWheelUp))
					{
						camera.zoom(MOUSE_WHEEL_ZOOM_SPEED);
					}
					if (input->has_key_been_pressed(System::Key::MouseWheelDown))
					{
						camera.zoom(-MOUSE_WHEEL_ZOOM_SPEED);
					}
#endif
				}
			}
		}
	}

	{
		hoveringOverAnyGizmo = false;
		grabbedAnyGizmo = false;
		for_every(c, controls.cursors)
		{
			if (c->hoveringOverGizmo.get())
			{
				hoveringOverAnyGizmo = true;
			}
			if (c->grabbedGizmo.get())
			{
				grabbedAnyGizmo = true;
			}
		}
	}

	base::process_controls();
}

Vector3 Editor::Base3D::reverse_projection_cursor(Optional<Vector2> const& _cursorAt, Optional<float> const& _depth) const
{
	Vector2 cursorAt;
	if (_cursorAt.is_set())
	{
		cursorAt = _cursorAt.get(camera.rtSize * 0.5f);
		// to -1,1 range
		cursorAt.x = (cursorAt.x / camera.rtSize.x) * 2.0f - 1.0f;
		cursorAt.y = (cursorAt.y / camera.rtSize.y) * 2.0f - 1.0f;
	}
	else
	{
		cursorAt = Vector2::zero;
	}
	return reverse_projection_cursor_2d_normalised(cursorAt, _depth);
}

Vector3 Editor::Base3D::reverse_projection_cursor_2d_normalised(Optional<Vector2> const& _cursorAt, Optional<float> const& _depth) const
{
	Vector2 cursorAt;
	if (_cursorAt.is_set())
	{
		cursorAt = _cursorAt.get();
	}
	else
	{
		cursorAt = Vector2::zero;
	}
	float const w = 1.0f;
	Vector4 cursor4(cursorAt.x, cursorAt.y, _depth.get(1.0f), w);
	Vector4 camSpace4 = camera.projectionMatrix.full_inverted().mul(cursor4);
	// change	x=right		y=up		z=backward
	// into		x=right		y=forward	z=up
	Vector3 camSpace3(camSpace4.x, -camSpace4.z, camSpace4.y);
	if (_depth.is_set())
	{
		// we want the point to be at the depth, otherwise we just want general direction/at 1
		camSpace3 = camSpace3 / camSpace4.w;
	}
	return camSpace3;
}

float Editor::Base3D::calculate_radius_for(Vector3 const& _inWorld, float _2dNormalisedRadius) const
{
	Vector3 inWorldCS = camera.at.location_to_local(_inWorld);
	Vector4 forProjection(inWorldCS.x, inWorldCS.z, -inWorldCS.y, 1.0f);
	Vector4 projected = camera.projectionMatrix.mul(forProjection);
	float projectedDepth = projected.z / projected.w;
	if (projectedDepth >= 1.0f)
	{
		// behind us
		return 0.0f;
	}

	Vector2 pretendCursorStart(0.0f, 0.0f);
	Vector2 pretendCursorEnd(0.0f, _2dNormalisedRadius);
	float const w = 1.0f;
	Vector4 pretendCursorStart4(pretendCursorStart.x, pretendCursorStart.y, projectedDepth, w);
	Vector4 pretendCursorEnd4(pretendCursorEnd.x, pretendCursorEnd.y, projectedDepth, w);

	auto invCamProj = camera.projectionMatrix.full_inverted();
	Vector4 camSpaceStart4 = invCamProj.mul(pretendCursorStart4);
	Vector4 camSpaceEnd4 = invCamProj.mul(pretendCursorEnd4);

	float radius = Vector3::distance(camSpaceStart4.to_vector3() / camSpaceStart4.w, camSpaceEnd4.to_vector3() / camSpaceEnd4.w);

	return radius;
}

Vector3 Editor::Base3D::cursor_ray_start(Optional<Vector2> const& _cursorAt) const
{
	if (camera.orthogonal)
	{
		Vector3 atCamSpace = reverse_projection_cursor(_cursorAt);
		atCamSpace.y = 0.0f;
		Vector3 at = camera.at.location_to_world(atCamSpace);
		at = at + camera.at.get_axis(Axis::Forward) * -1000.0f;
		return at;
	}
	else
	{
		return camera.at.get_translation();
	}
}

Vector3 Editor::Base3D::cursor_ray_dir(Optional<Vector2> const& _cursorAt) const
{
	if (camera.orthogonal)
	{
		return camera.at.get_axis(Axis::Forward);
	}
	else
	{
		Vector3 atCamSpace = reverse_projection_cursor(_cursorAt);
		atCamSpace.y = 1.0f;
		Vector3 at = camera.at.location_to_world(atCamSpace);
		Vector3 dir = (at - camera.at.get_translation()).normal();
		return dir;
	}
}

Vector2 Editor::Base3D::cursor_moved_by_2d_normalised(Optional<Vector2> const& _cursorMovedBy) const
{
	if (_cursorMovedBy.is_set())
	{
		Vector2 movedBy = _cursorMovedBy.get(Vector2::zero);
		movedBy.x = movedBy.x / (camera.rtSize.y * 0.5f);
		movedBy.y = movedBy.y / (camera.rtSize.y * 0.5f);
		return movedBy;
	}
	else
	{
		return Vector2::zero;
	}
}

Vector3 Editor::Base3D::cursor_moved_by_2d_normalised_into_3d(Optional<Vector2> const& _cursorMovedBy2dNormalised, Optional<Vector3> const& _inWorld) const
{
	Vector2 centre = Vector2::zero;
	Vector2 moved = centre + _cursorMovedBy2dNormalised.get(Vector2::zero);

	Optional<float> depth;
	if (_inWorld.is_set())
	{
		Vector3 inWorldCS = camera.at.location_to_local(_inWorld.get());
		Vector4 forProjection(inWorldCS.x, inWorldCS.z, -inWorldCS.y, 1.0f);
		Vector4 projected = camera.projectionMatrix.mul(forProjection);
		depth = clamp(projected.z / projected.w, 0.0f, 1.0f);
	}

	Vector3 centreCS = reverse_projection_cursor_2d_normalised(centre, depth);
	Vector3 movedCS = reverse_projection_cursor_2d_normalised(moved, depth);

	Vector3 movedByCS = movedCS - centreCS;
	movedByCS.y = 0.0f;

	return camera.at.vector_to_world(movedByCS);
}

void Editor::Base3D::pre_render(CustomRenderContext* _customRC)
{
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : Game::get()->get_main_render_buffer();
	::System::ForRenderTargetOrNone forRT(rtTarget);

	if (camera.orthogonal)
	{
		camera.zRange = Range(-1000.0f, 1000.0f);
	}
	else
	{
		camera.zRange = Range(0.01f / (camera.fov / 90.0f), 10000.0f);
	}

	if (camera.orthogonal)
	{
		Vector2 viewCentreOffset = Vector2::zero;
		Range2 range;
		Vector2 size;
		size.y = 1.0f / camera.orthogonalScale;
		size.x = forRT.get_full_size_aspect_ratio() * size.y;
		range.x.min = size.x * 0.5f * (-1.0f - viewCentreOffset.x);
		range.x.max = size.x * 0.5f * (1.0f - viewCentreOffset.x);
		range.y.min = size.y * 0.5f * (-1.0f - viewCentreOffset.y);
		range.y.max = size.y * 0.5f * (1.0f - viewCentreOffset.y);
		camera.projectionMatrix = orthogonal_matrix(range, camera.zRange.min, camera.zRange.max);
		camera.rtSize = forRT.get_size().to_vector2();
		
		//
		debug_camera_orthogonal(camera.at, camera.orthogonalScale);
	}
	else
	{
		Matrix44 viewCentreOffsetMatrix = Matrix44::identity;
		viewCentreOffsetMatrix.m30 = 0.0f;//viewCentreOffset.x;
		viewCentreOffsetMatrix.m31 = 0.0f;//viewCentreOffset.y;

		camera.projectionMatrix = viewCentreOffsetMatrix.to_world(projection_matrix(camera.fov, forRT.get_full_size_aspect_ratio(), camera.zRange.min, camera.zRange.max));
		camera.rtSize = forRT.get_size().to_vector2();
		
		//
		debug_camera(camera.at, camera.fov, NP);
	}

	debug_renderer_mode(DebugRendererMode::All);

	base::pre_render(_customRC);
}

void Editor::Base3D::render_main(CustomRenderContext* _customRC)
{
	base::render_main(_customRC);
}

void Editor::Base3D::render_debug(CustomRenderContext* _customRC)
{
#ifdef AN_DEBUG_RENDERER
	debug_gather_in_renderer();

	if (grid.on)
	{
		float gridStep = grid.size / 500.0f;

		if (gridStep < 0.05f) gridStep = 0.05f; else
		if (gridStep < 0.10f) gridStep = 0.10f; else
		if (gridStep < 0.25f) gridStep = 0.25f; else
		if (gridStep < 0.50f) gridStep = 0.50f; else
		if (gridStep < 1.00f) gridStep = 1.00f; else
		if (gridStep < 5.00f) gridStep = 5.00f; else
			gridStep = 10.0f;

		float startG = round_to(-grid.size * 0.5f, gridStep);
		float endG = round_to(grid.size * 0.5f, gridStep);

		for_count(int, axisIdx, 2)
		{
			Vector3 axisX = axisIdx == 0 ? grid.x : grid.y;
			Vector3 axisY = axisIdx == 0 ? grid.y : grid.x;
			Vector3 axisH = axisY * grid.size * 0.5f;
			for (float g = startG; g <= endG + 0.0001f; g += gridStep)
			{
				float a = 0.2f;
				float ac = g;
				if (abs(ac - round(ac)) < 0.001f)
				{
					a = max(a, 1.0f);
				}
				ac *= 2.0f;
				if (abs(ac - round(ac)) < 0.001f)
				{
					a = max(a, 0.5f);
				}
				Vector3 gAt = grid.start + axisX * g;
				debug_draw_line(false, Colour::greyLight.with_set_alpha(a), gAt - axisH, gAt + axisH);
			}
		}
	}

	if (axes.on)
	{
		debug_draw_line(false, Colour::red, Vector3::zero, Vector3::xAxis);
		debug_draw_line(false, Colour::green, Vector3::zero, Vector3::yAxis);
		debug_draw_line(false, Colour::blue, Vector3::zero, Vector3::zAxis);
	}

	if (gizmos.on)
	{
		for_every_ref(gizmo, gizmos.gizmos)
		{
			gizmo->radiusWorld = calculate_radius_for(gizmo->atWorld, gizmo->radius2d);
			if (gizmo->radiusWorld > 0.0f)
			{
				bool hovering = false;
				for_every(c, controls.cursors)
				{
					if (c->hoveringOverGizmo == gizmo)
					{
						hovering = true;
						break;
					}
				}
				Colour c = hovering ? Colour::red : gizmo->colour.with_alpha(0.8f);
				if (gizmo->fromWorld.is_set())
				{
					debug_draw_line(true, c, gizmo->fromWorld.get(), gizmo->atWorld);
				}
				if (gizmo->fromWorld2.is_set())
				{
					debug_draw_line(true, c, gizmo->fromWorld2.get(), gizmo->atWorld);
				}
				if (gizmo->grabbable)
				{
					debug_draw_sphere(true, true, c, hovering ? 1.0f : 0.3f, Sphere(gizmo->atWorld, gizmo->radiusWorld));
				}
			}
		}
	}

	{
		System::Video3D* v3d = System::Video3D::get();
		System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : Game::get()->get_main_render_buffer();

		::System::ForRenderTargetOrNone forRT(rtTarget);

		forRT.bind();

		v3d->set_viewport(VectorInt2::zero, forRT.get_size());
		v3d->set_near_far_plane(camera.zRange.min, camera.zRange.max);
		v3d->access_model_view_matrix_stack().clear();
		v3d->access_clip_plane_stack().clear();
		v3d->set_3d_projection_matrix(camera.projectionMatrix);
		v3d->setup_for_3d_display();

		// actual asset
		{
			v3d->access_model_view_matrix_stack().push_set(camera.at.to_matrix().inverted());

			render_edited_asset();

			v3d->access_model_view_matrix_stack().pop();
		}

		debug_renderer_render(v3d);

		forRT.unbind();
	}

	debug_gather_restore();
#endif

	base::render_debug(_customRC);
}

void Editor::Base3D::render_edited_asset()
{
	if (auto* a = editedAsset.get())
	{
		if (a->editor_render())
		{
			// ok
		}
		else
		{
			a->debug_draw();
		}
	}
}

void Editor::Base3D::setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("grid"));
		b->set_id(NAME(editor3D_grid));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				grid.on = !grid.on;
				update_ui_highlight();
			});
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("axes"));
		b->set_id(NAME(editor3D_axes));
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				axes.on = !axes.on;
				update_ui_highlight();
			});
		panel->add(b);
	}
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("gizmos"));
		b->set_id(NAME(editor3D_gizmos));
#ifdef AN_STANDARD_INPUT
		b->set_shortcut(System::Key::G, UI::ShortcutParams().with_shift());
#endif
		b->set_scale(scale)->set_auto_width(c)->set_default_height(c);
		b->set_on_press([this](Framework::UI::ActContext const& _context)
			{
				gizmos.on = !gizmos.on;
				update_ui_highlight();
			});
		panel->add(b);
	}
}

void Editor::Base3D::setup_ui(REF_ SetupUIContext& _setupUIContext)
{
	base::setup_ui(REF_ _setupUIContext);

	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }
	auto* c = canvas;

	Framework::UI::ICanvasElement* & above = _setupUIContext.bottomLeft_up;
	Framework::UI::ICanvasElement* & nextTo = _setupUIContext.bottomLeft_right;

	{
		auto* menu = Framework::UI::CanvasWindow::get();
		c->add(menu);
		{
			setup_ui_add_to_bottom_left_panel(c, menu);

			menu->place_content_horizontally(c);
		}
		UI::Utils::place_above(c, menu, REF_ above, 0.0f);
		if (!_setupUIContext.bottomLeft)
		{
			_setupUIContext.bottomLeft = above;
			nextTo = above;
		}
	}
	{
		auto* menu = Framework::UI::CanvasWindow::get();
		c->add(menu);
		{
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("top"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_top));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::X, UI::ShortcutParams().with_shift(), [this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_TOP, VIEW_ROTATOR_BOTTOM);
						update_ui_highlight();
					});
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_TOP);
						update_ui_highlight();
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("bottom"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_bottom));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::X, UI::ShortcutParams().with_shift().not_to_be_used());
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_BOTTOM);
						update_ui_highlight();
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("back"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_back));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::C, UI::ShortcutParams().with_shift(), [this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_BACK, VIEW_ROTATOR_FRONT);
						update_ui_highlight();
					});
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_BACK);
						update_ui_highlight();
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("front"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_front));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::C, UI::ShortcutParams().with_shift().not_to_be_used());
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_FRONT);
						update_ui_highlight();
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("left"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_left));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::V, UI::ShortcutParams().with_shift(), [this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_LEFT, VIEW_ROTATOR_RIGHT);
						update_ui_highlight();
					});
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_RIGHT);
						update_ui_highlight();
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("right"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_right));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::V, UI::ShortcutParams().with_shift().not_to_be_used());
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_orthogonal(VIEW_ROTATOR_LEFT);
						update_ui_highlight();
					});
				menu->add(b);
			}

			menu->place_content_on_grid(c, VectorInt2(2,0));
		}
		UI::Utils::place_above(c, menu, REF_ above, 0.0f);
	}
	{
		auto* menu = Framework::UI::CanvasWindow::get();
		c->add(menu);
		{
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("perspective"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_persp));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::Z, UI::ShortcutParams().with_shift());
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						switch_camera_to_perspective(false);
						update_ui_highlight();
					});
				menu->add(b);
			}
			{
				auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("from ortho"))->set_auto_width(c)->set_default_height(c);
				b->set_id(NAME(editor3D_persp));
#ifdef AN_STANDARD_INPUT
				b->set_shortcut(System::Key::Z, UI::ShortcutParams().with_shift().with_ctrl());
#endif
				b->set_on_press([this](Framework::UI::ActContext const& _context)
					{
						if (camera.orthogonal)
						{
							switch_camera_to_perspective(true);
						}
						update_ui_highlight();
					});
				menu->add(b);
			}

			menu->place_content_vertically(c);
		}
		UI::Utils::place_above(c, menu, REF_ above, 0.0f);
	}
}

void Editor::Base3D::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	auto* c = canvas;

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_grid))))
	{
		b->set_highlighted(grid.on);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_axes))))
	{
		b->set_highlighted(axes.on);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_gizmos))))
	{
		b->set_highlighted(gizmos.on);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_persp))))
	{
		b->set_highlighted(! camera.orthogonal);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_top))))
	{
		b->set_highlighted(camera.orthogonal && (camera.orthogonalRotation == VIEW_ROTATOR_TOP));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_bottom))))
	{
		b->set_highlighted(camera.orthogonal && (camera.orthogonalRotation == VIEW_ROTATOR_BOTTOM));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_back))))
	{
		b->set_highlighted(camera.orthogonal && (camera.orthogonalRotation == VIEW_ROTATOR_BACK));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_front))))
	{
		b->set_highlighted(camera.orthogonal && (camera.orthogonalRotation == VIEW_ROTATOR_FRONT));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_left))))
	{
		b->set_highlighted(camera.orthogonal && (camera.orthogonalRotation == VIEW_ROTATOR_RIGHT));
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor3D_right))))
	{
		b->set_highlighted(camera.orthogonal && (camera.orthogonalRotation == VIEW_ROTATOR_LEFT));
	}

	base::update_ui_highlight();
}

bool Editor::Base3D::on_hold(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir)
{
	if (_cursorIdx == UI::CursorIndex::Mouse &&
		(_buttonIdx == System::MouseButton::Middle || _buttonIdx == System::MouseButton::Right))
	{
		// rotation, translation, zoom
		return true;
	}

	return false;
}

bool Editor::Base3D::on_held(int _cursorIdx, int _buttonIdx, Vector3 const& _rayStart, Vector3 const& _rayDir, Vector3 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised)
{
	bool rotate = false;
	bool move = false;
	bool zoom = false;
	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == System::MouseButton::Middle)
	{
		if (controls.cursors[_cursorIdx].buttons[System::MouseButton::Right].held)
		{
			// will move/zoom, just keep it active
			return true;
		}
		else
		{
			move = true;
		}
	}

	if (_cursorIdx == UI::CursorIndex::Mouse &&
		_buttonIdx == System::MouseButton::Right)
	{
		if (controls.cursors[_cursorIdx].buttons[System::MouseButton::Middle].held)
		{
			zoom = true;
		}
		else
		{
			rotate = true;
		}
	}

	float const MOV_SPEED = 0.5f;
	float const ROT_SPEED = 180.0f;
	float const ZOOM_SPEED = 2.0f;

	if (zoom)
	{
		camera.zoom(_cursorMovedBy2DNormalised.y * ZOOM_SPEED);
		return true;
	}

	if (rotate)
	{
		if (camera.orthogonal)
		{
#ifdef NO_ROTATION_FOR_ORTHOGONAL_CAMERA
			// no rotation for orthogonal, always move
			move = true;
#else
			switch_camera_to_perspective(true);
			update_ui_highlight();
#endif
		}
		if (!camera.orthogonal)
		{
			Rotator3 rotateBy = Rotator3::zero;
			{
				rotateBy.pitch += _cursorMovedBy2DNormalised.y * ROT_SPEED;
				rotateBy.yaw += _cursorMovedBy2DNormalised.x * ROT_SPEED;
			}
			camera.perspectiveRotation = camera.perspectiveRotation.to_world(rotateBy.to_quat());
			if (Vector3::dot(camera.perspectiveRotation.get_z_axis(), camera.perspectiveUp) < 0.0f)
			{
				camera.perspectiveUp = -camera.perspectiveUp;
			}
			camera.update_at();
			return true;
		}
	}

	if (move)
	{
		if (camera.orthogonal)
		{
			Vector3 moveBy = -_movedBy; // negate as we're dragging the world in opposite direction, and use _movedBy as it reflects the actual movement the best
			camera.pivot += moveBy;
			camera.update_at();
			return true;
		}
		else
		{
			Vector3 moveBy = -_movedBy * MOV_SPEED; // negate as we're dragging the world in opposite direction
			moveBy *= camera.perspectiveDistance;
			camera.pivot += moveBy;
			camera.update_at();
			return true;
		}
	}

	return false;
}

void Editor::Base3D::add_gizmo(Gizmo3D* g)
{
	for_every_ref(gizmo, gizmos.gizmos)
	{
		if (gizmo == g)
		{
			return;
		}
	}
	gizmos.gizmos.push_back();
	gizmos.gizmos.get_last() = g;
}

bool Editor::Base3D::is_gizmo_grabbed(Gizmo3D const* g) const
{
	if (g)
	{
		for_every(cursor, controls.cursors)
		{
			if (cursor->grabbedGizmo == g)
			{
				return true;
			}
		}
	}
	return false;
}

void Editor::Base3D::switch_camera_to_perspective(bool _fromOrthogonal)
{
	if (camera.orthogonal)
	{
		camera.perspectiveDistance = 0.5f / camera.orthogonalScale;
	}
	if (_fromOrthogonal)
	{
		camera.perspectiveRotation = camera.orthogonalRotation.to_quat();
		camera.perspectiveUp = camera.perspectiveRotation.get_axis(Axis::Z);
	}
	camera.orthogonal = false;
	camera.update_at();
	grid.x = Vector3::xAxis;
	grid.y = Vector3::yAxis;
}

void Editor::Base3D::switch_camera_to_orthogonal(Rotator3 const& _rotator, Optional<Rotator3> const& _altRotator)
{
	if (!camera.orthogonal)
	{
		camera.orthogonalScale = 0.5f / camera.perspectiveDistance;
	}
	bool * temp = (_rotator == VIEW_ROTATOR_TOP)? &camera.lastOrthogonalTop
			   : ((_rotator == VIEW_ROTATOR_BACK)? &camera.lastOrthogonalBack
			   : ((_rotator == VIEW_ROTATOR_LEFT)? &camera.lastOrthogonalLeft : nullptr));
	bool * opTemp = (_rotator == VIEW_ROTATOR_BOTTOM)? &camera.lastOrthogonalTop
			   : ((_rotator == VIEW_ROTATOR_FRONT)? &camera.lastOrthogonalBack
			   : ((_rotator == VIEW_ROTATOR_RIGHT)? &camera.lastOrthogonalLeft : nullptr));
	if (_altRotator.is_set())
	{
		if (camera.orthogonal && camera.orthogonalRotation == _rotator)
		{
			camera.orthogonalRotation = _altRotator.get();
		}
		else if (camera.orthogonal && camera.orthogonalRotation == _altRotator.get())
		{
			camera.orthogonalRotation = _rotator;
		}
		else if (temp)
		{
			// restore the last one
			camera.orthogonalRotation = *temp? _rotator : _altRotator.get();
		}
		else if (opTemp)
		{
			// restore the last one
			camera.orthogonalRotation = !*opTemp? _rotator : _altRotator.get();
		}
		else
		{
			camera.orthogonalRotation = _rotator;
		}
	}
	else
	{
		camera.orthogonalRotation = _rotator;
	}
	camera.orthogonal = true;
	if (camera.orthogonalRotation == VIEW_ROTATOR_TOP || camera.orthogonalRotation == VIEW_ROTATOR_BOTTOM)
	{
		camera.orthogonalAxis = 2;
		grid.x = Vector3::xAxis;
		grid.y = Vector3::yAxis;
		if (temp) *temp = camera.orthogonalRotation == VIEW_ROTATOR_TOP;
		if (opTemp) *opTemp = camera.orthogonalRotation == VIEW_ROTATOR_TOP;
	}
	else if (camera.orthogonalRotation == VIEW_ROTATOR_RIGHT || camera.orthogonalRotation == VIEW_ROTATOR_LEFT)
	{
		camera.orthogonalAxis = 0;
		grid.x = Vector3::yAxis;
		grid.y = Vector3::zAxis;
		if (temp) *temp = camera.orthogonalRotation == VIEW_ROTATOR_LEFT;
		if (opTemp) *opTemp = camera.orthogonalRotation == VIEW_ROTATOR_LEFT;
	}
	else if (camera.orthogonalRotation == VIEW_ROTATOR_FRONT || camera.orthogonalRotation == VIEW_ROTATOR_BACK)
	{
		camera.orthogonalAxis = 1;
		grid.x = Vector3::xAxis;
		grid.y = Vector3::zAxis;
		if (temp) *temp = camera.orthogonalRotation == VIEW_ROTATOR_BACK;
		if (opTemp) *opTemp = camera.orthogonalRotation == VIEW_ROTATOR_BACK;
	}
	else
	{
		an_assert(false);
		camera.orthogonalAxis = 1;
		grid.x = Vector3::xAxis;
		grid.y = Vector3::zAxis;
	}
	camera.update_at();
}

//--

void Editor::Base3D::Camera::zoom(float _by)
{
	float zoomBy = 1.0f;
	if (_by < 0.0f)
	{
		zoomBy = 1.0f / (1.0f + abs(_by));
	}
	else
	{
		zoomBy = 1.0f * (1.0f + abs(_by));
	}

	if (orthogonal)
	{
		this->orthogonalScale *= zoomBy;
	}
	else
	{
		this->perspectiveDistance /= zoomBy;
	}

	update_at();
}

void Editor::Base3D::Camera::update_at()
{
	if (orthogonal)
	{
		at = Transform(pivot, orthogonalRotation.to_quat());
	}
	else
	{
		Vector3 from = pivot - perspectiveRotation.get_y_axis() * perspectiveDistance;
		float lookingVertically = abs(Vector3::dot(Vector3::zAxis, perspectiveRotation.get_y_axis()));
		lookingVertically = remap_value(lookingVertically, 0.0f, 0.9f, 0.0f, 1.0f, true);
		Vector3 orientedUp = Vector3::zAxis * (sign(perspectiveRotation.get_z_axis().z + (-0.3f) * perspectiveRotation.get_y_axis().z) >= 0.0f? 1.0f : -1.0f);
		Vector3 useUp = lerp(lookingVertically, orientedUp, perspectiveUp).normal();
		if (lookingVertically < 1.0f)
		{
			perspectiveUp = blend_to_using_time(perspectiveUp, useUp, ::System::Core::get_raw_delta_time(), 4.0f).normal();
		}
		else
		{
			Vector3 blendTo = perspectiveRotation.get_z_axis();
			perspectiveUp = blend_to_using_time(perspectiveUp, blendTo, ::System::Core::get_raw_delta_time(), 6.0f).normal();
		}
		at = look_at_matrix(from, pivot, useUp).to_transform();
		perspectiveRotation = at.get_orientation();
	}
}

//--

void Editor::Gizmo3DThreeAxes::clear()
{
	gizmo.clear();
	gizmoX.clear();
	gizmoY.clear();
	gizmoZ.clear();
}

void Editor::Gizmo3DThreeAxes::update(Base3D* _base3D)
{
	if (!gizmo.get())
	{
		gizmo = Gizmo3D::get_one();
		gizmo->grabbable = false;
		gizmo->atWorld = atWorld;
		_base3D->add_gizmo(gizmo.get());
	}
	if (!gizmoX.get())
	{
		gizmoX = Gizmo3D::get_one();
		gizmoX->atWorld = atWorld;
		gizmoX->atWorld.x += gizmoAxesDistance;
		gizmoX->axisWorld = Vector3::xAxis;
		gizmoX->radius2d *= 0.6f;
		gizmoX->colour = Colour::red;
		_base3D->add_gizmo(gizmoX.get());
	}
	if (!gizmoY.get())
	{
		gizmoY = Gizmo3D::get_one();
		gizmoY->atWorld = atWorld;
		gizmoY->atWorld.y += gizmoAxesDistance;
		gizmoY->axisWorld = Vector3::yAxis;
		gizmoY->radius2d *= 0.6f;
		gizmoY->colour = Colour::green;
		_base3D->add_gizmo(gizmoY.get());
	}
	if (!gizmoZ.get())
	{
		gizmoZ = Gizmo3D::get_one();
		gizmoZ->atWorld = atWorld;
		gizmoZ->atWorld.z += gizmoAxesDistance;
		gizmoZ->axisWorld = Vector3::zAxis;
		gizmoZ->radius2d *= 0.6f;
		gizmoZ->colour = Colour::blue;
		_base3D->add_gizmo(gizmoZ.get());
	}
	gizmo->colour = colour;

	atWorld.x = gizmoX->atWorld.x - gizmoAxesDistance;
	atWorld.y = gizmoY->atWorld.y - gizmoAxesDistance;
	atWorld.z = gizmoZ->atWorld.z - gizmoAxesDistance;

	gizmoAxesDistance = _base3D->calculate_radius_for(atWorld, axesDistance2d);

	gizmo->atWorld = atWorld;
	gizmoX->atWorld = atWorld + Vector3::xAxis * gizmoAxesDistance;
	gizmoY->atWorld = atWorld + Vector3::yAxis * gizmoAxesDistance;
	gizmoZ->atWorld = atWorld + Vector3::zAxis * gizmoAxesDistance;
	gizmo->fromWorld = fromWorld;
	gizmoX->fromWorld = atWorld;
	gizmoY->fromWorld = atWorld;
	gizmoZ->fromWorld = atWorld;
}

//--

void Editor::Gizmo3DThreeAxesAndRotation::clear()
{
	base::clear();
	gizmoPitch.clear();
	gizmoYaw.clear();
	gizmoRoll.clear();
}

void Editor::Gizmo3DThreeAxesAndRotation::update(Base3D* _base3D)
{
	if (!gizmoPitch.get())
	{
		gizmoPitch = Gizmo3D::get_one();
		gizmoPitch->atWorld = atWorld;
		gizmoPitch->atWorld.y += gizmoAxesDistance;
		gizmoPitch->atWorld.z += gizmoAxesDistance;
		gizmoPitch->radius2d *= 0.4f;
		gizmoPitch->colour = Colour::cyan;
		gizmoPitch->value = rotation.pitch;
		gizmoPitch->valueScale = 90.0f;
		_base3D->add_gizmo(gizmoPitch.get());
	}
	if (!gizmoYaw.get())
	{
		gizmoYaw = Gizmo3D::get_one();
		gizmoYaw->atWorld = atWorld;
		gizmoYaw->atWorld.x += gizmoAxesDistance;
		gizmoYaw->atWorld.y += gizmoAxesDistance;
		gizmoYaw->radius2d *= 0.4f;
		gizmoYaw->colour = Colour::yellow;
		gizmoYaw->value = rotation.yaw;
		gizmoYaw->valueScale = 90.0f;
		_base3D->add_gizmo(gizmoYaw.get());
	}
	if (!gizmoRoll.get())
	{
		gizmoRoll = Gizmo3D::get_one();
		gizmoRoll->atWorld = atWorld;
		gizmoRoll->atWorld.x += gizmoAxesDistance;
		gizmoRoll->atWorld.z += gizmoAxesDistance;
		gizmoRoll->radius2d *= 0.4f;
		gizmoRoll->colour = Colour::magenta;
		gizmoRoll->value = rotation.roll;
		gizmoRoll->valueScale = 90.0f;
		_base3D->add_gizmo(gizmoRoll.get());
	}

	rotation.pitch = gizmoPitch->value.get(rotation.pitch);
	rotation.yaw = gizmoYaw->value.get(rotation.yaw);
	rotation.roll = gizmoRoll->value.get(rotation.roll);

	base::update(_base3D);

	gizmoPitch->atWorld = atWorld + (rotation.get_axis(Axis::Y) + rotation.get_axis(Axis::Z)) * gizmoAxesDistance;
	gizmoYaw->atWorld = atWorld + (rotation.get_axis(Axis::X) + rotation.get_axis(Axis::Y)) * gizmoAxesDistance;
	gizmoRoll->atWorld = atWorld + (rotation.get_axis(Axis::X) + rotation.get_axis(Axis::Z)) * gizmoAxesDistance;

	{
		float radius = gizmoAxesDistance * sqrt(2.0f);
		Vector3 f = atWorld + (rotation.get_axis(Axis::Y)) * radius;
		Vector3 r = atWorld + (rotation.get_axis(Axis::X)) * radius;
		Vector3 u = atWorld + (rotation.get_axis(Axis::Z)) * radius;
		gizmoPitch->fromWorld = f;
		gizmoPitch->fromWorld2 = u;
		gizmoYaw->fromWorld = f;
		gizmoYaw->fromWorld2 = r;
		gizmoRoll->fromWorld = r;
		gizmoRoll->fromWorld2 = u;
	}
}
