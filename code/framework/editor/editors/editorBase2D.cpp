#include "editorBase2D.h"

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

//

using namespace Framework;

//

// settings
DEFINE_STATIC_NAME(editor2D_camera_pivot);
DEFINE_STATIC_NAME(editor2D_camera_scale);

DEFINE_STATIC_NAME(editor2D_grid_on);
DEFINE_STATIC_NAME(editor2D_grid_size);
DEFINE_STATIC_NAME(editor2D_grid_start);
DEFINE_STATIC_NAME(editor2D_grid_x);
DEFINE_STATIC_NAME(editor2D_grid_y);

DEFINE_STATIC_NAME(editor2D_axes_on);

DEFINE_STATIC_NAME(editor2D_gizmos_on);

// ui
DEFINE_STATIC_NAME(editor2D_grid);
DEFINE_STATIC_NAME(editor2D_axes);
DEFINE_STATIC_NAME(editor2D_gizmos);

//

REGISTER_FOR_FAST_CAST(Editor::Base2D);

Editor::Base2D::Base2D()
{
	storeRestoreNames_cameraPivot = NAME(editor2D_camera_pivot);
	storeRestoreNames_cameraScale = NAME(editor2D_camera_scale);
	camera.update_at();
}

Editor::Base2D::~Base2D()
{
}

#define STORE_RESTORE(_op) \
	_op(_setup, camera.pivot, storeRestoreNames_cameraPivot); \
	_op(_setup, camera.scale, storeRestoreNames_cameraScale); \
	\
	_op(_setup, grid.on, NAME(editor2D_grid_on)); \
	_op(_setup, grid.size, NAME(editor2D_grid_size)); \
	_op(_setup, grid.start, NAME(editor2D_grid_start)); \
	\
	_op(_setup, axes.on, NAME(editor2D_axes_on)); \
	\
	_op(_setup, gizmos.on, NAME(editor2D_gizmos_on)); \
	;

void Editor::Base2D::restore_for_use(SimpleVariableStorage const& _setup)
{
	base::restore_for_use(_setup);
	STORE_RESTORE(read);
	camera.update_at();
}

void Editor::Base2D::store_for_later(SimpleVariableStorage & _setup) const
{
	base::store_for_later(_setup);
	STORE_RESTORE(write);
}

void Editor::Base2D::process_controls()
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
		Vector2 curAt = cursor_ray_start(at);
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
					on_release(cIdx, for_everys_index(b), curAt);
					b->held = false;
				}
				if (!ignoreInput)
				{
					if (b->isDown)
					{
						b->timeDown = 0.0f;
						if (! ignoreInput)
						{
							on_press(cIdx, for_everys_index(b), curAt);
							if (c->hoveringOverGizmo.get())
							{
								c->grabbedGizmo = c->hoveringOverGizmo;
								inputGrabbed = false;
								b->held = true;
							}
							else
							{
								b->held = on_hold_early(cIdx, for_everys_index(b), curAt);
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
								on_click(cIdx, for_everys_index(b), curAt);
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
								if (g->axisWorld.is_set())
								{
									if (g->axisWorld.get().is_zero())
									{
										g->atWorld += movedBy2dNormalised / camera.scale;
									}
									else
									{
										Vector2 axis = g->axisWorld.get().normal();
										g->atWorld += axis * (movedBy2dNormalised.x + movedBy2dNormalised.y) / camera.scale;
									}
								}
								else if (g->value.is_set())
								{
									g->value = g->value.get() + (movedBy2dNormalised.x + movedBy2dNormalised.y) * g->valueScale.get(1.0f);
								}
							}
							else
							{
								b->held = on_held(cIdx, for_everys_index(b), curAt, cursor_moved_by_2d_normalised_into_3d(movedBy2dNormalised), movedBy2dNormalised);
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
								b->held = on_hold(cIdx, for_everys_index(b), curAt);
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
			Gizmo2D* hoveringOver = nullptr;

			for_every_ref(gizmo, gizmos.gizmos)
			{
				if (gizmo->grabbable)
				{
					Vector2 s = curAt;
					Vector2 p = gizmo->atWorld;

					float dist = Vector2::distance(p, s);

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
			on_hover(cIdx, curAt);

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

Vector2 Editor::Base2D::reverse_projection_cursor(Optional<Vector2> const& _cursorAt, Optional<float> const& _depth) const
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

Vector2 Editor::Base2D::reverse_projection_cursor_2d_normalised(Optional<Vector2> const& _cursorAt, Optional<float> const& _depth) const
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
	Vector2 camSpace2(camSpace4.x, camSpace4.y);
	return camSpace2;
}

float Editor::Base2D::calculate_radius_for(Vector2 const& _inWorld, float _2dNormalisedRadius) const
{
	Vector3 inWorldCS = camera.at.location_to_local(_inWorld.to_vector3());
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

Vector2 Editor::Base2D::cursor_ray_start(Optional<Vector2> const& _cursorAt) const
{
	Vector2 atCamSpace = reverse_projection_cursor(_cursorAt);
	Vector2 at = camera.at.location_to_world(atCamSpace.to_vector3()).to_vector2();
	return at;
}

Vector2 Editor::Base2D::cursor_moved_by_2d_normalised(Optional<Vector2> const& _cursorMovedBy) const
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

Vector2 Editor::Base2D::cursor_moved_by_2d_normalised_into_3d(Optional<Vector2> const& _cursorMovedBy2dNormalised, Optional<Vector2> const& _inWorld) const
{
	Vector2 centre = Vector2::zero;
	Vector2 moved = centre + _cursorMovedBy2dNormalised.get(Vector2::zero);

	Optional<float> depth;
	if (_inWorld.is_set())
	{
		Vector2 inWorldCS = camera.at.location_to_local(_inWorld.get().to_vector3()).to_vector2();
		Vector4 forProjection(inWorldCS.x, 0.0f, -inWorldCS.y, 1.0f);
		Vector4 projected = camera.projectionMatrix.mul(forProjection);
		depth = clamp(projected.z / projected.w, 0.0f, 1.0f);
	}

	Vector2 centreCS = reverse_projection_cursor_2d_normalised(centre, depth);
	Vector2 movedCS = reverse_projection_cursor_2d_normalised(moved, depth);

	Vector2 movedByCS = movedCS - centreCS;

	return camera.at.vector_to_world(movedByCS.to_vector3()).to_vector2();
}

void Editor::Base2D::pre_render(CustomRenderContext* _customRC)
{
	System::RenderTarget* rtTarget = _customRC ? _customRC->sceneRenderTarget.get() : Game::get()->get_main_render_buffer();
	::System::ForRenderTargetOrNone forRT(rtTarget);

	{
		camera.zRange = Range(-1000.0f, 1000.0f);
	}

	{
		Vector2 viewCentreOffset = Vector2::zero;
		Range2 range;
		Vector2 size;
		size.y = 1.0f / camera.scale;
		size.x = forRT.get_full_size_aspect_ratio() * size.y;
		range.x.min = size.x * 0.5f * (-1.0f - viewCentreOffset.x);
		range.x.max = size.x * 0.5f * (1.0f - viewCentreOffset.x);
		range.y.min = size.y * 0.5f * (-1.0f - viewCentreOffset.y);
		range.y.max = size.y * 0.5f * (1.0f - viewCentreOffset.y);
		camera.projectionMatrix = orthogonal_matrix(range, camera.zRange.min, camera.zRange.max);
		camera.rtSize = forRT.get_size().to_vector2();
		
		//
		debug_camera_orthogonal(camera.renderAt, camera.scale);
	}

	debug_renderer_mode(DebugRendererMode::All);

	base::pre_render(_customRC);
}

void Editor::Base2D::render_main(CustomRenderContext* _customRC)
{
	base::render_main(_customRC);
}

Range2 Editor::Base2D::get_grid_range() const
{
	return Range2(Range(-grid.size * 0.5f, grid.size * 0.5f), Range(-grid.size * 0.5f, grid.size * 0.5f));
}

Vector2 Editor::Base2D::get_grid_step() const
{
	Range2 gridRange = get_grid_range();

	Vector2 gridStep = gridRange.length() / 500.0f;

	for_count(int, e, 2)
	{
		auto& gs = gridStep.access_element(e);
		if (gs < 0.05f) gs = 0.05f; else
		if (gs < 0.10f) gs = 0.10f; else
		if (gs < 0.25f) gs = 0.25f; else
		if (gs < 0.50f) gs = 0.50f; else
		if (gs < 1.00f) gs = 1.00f; else
		if (gs < 5.00f) gs = 5.00f; else
			gs = 10.0f;
	}

	return gridStep;
}

Vector2 Editor::Base2D::get_grid_zero_at() const
{
	return Vector2::zero;
}

float Editor::Base2D::get_grid_alpha(float _v, float _step) const
{
	float a = 0.2f;
	float ac = _v;
	if (abs(ac - round(ac)) < 0.001f)
	{
		a = max(a, 1.0f);
	}
	ac *= 2.0f;
	if (abs(ac - round(ac)) < 0.001f)
	{
		a = max(a, 0.5f);
	}
	return a;
}

void Editor::Base2D::render_debug(CustomRenderContext* _customRC)
{
#ifdef AN_DEBUG_RENDERER
	debug_gather_in_renderer();

	if (grid.on)
	{
		Range2 gridRange = get_grid_range();
		Vector2 gridStep = get_grid_step();
		Vector2 gridZeroAt = get_grid_zero_at();

		for_count(int, axisIdx, 2)
		{
			float axisStep = gridStep.get_element(axisIdx);
			float axisZeroAt = gridZeroAt.get_element(axisIdx);

			float startG = round_to(gridRange.get_element(axisIdx).min - axisZeroAt, axisStep) + axisZeroAt;
			float endG = round_to(gridRange.get_element(axisIdx).max - axisZeroAt, axisStep) + axisZeroAt;

			for (float g = startG; g <= endG + 0.0001f; g += axisStep)
			{
				float a = get_grid_alpha(g, axisStep);
				Vector2 gAt0;
				Vector2 gAt1;
				if (axisIdx == 0)
				{
					gAt0 = Vector2(g, gridRange.y.min);
					gAt1 = Vector2(g, gridRange.y.max);
				}
				else
				{
					gAt0 = Vector2(gridRange.x.min, g);
					gAt1 = Vector2(gridRange.x.max, g);
				}
				debug_draw_line(false, Colour::greyLight.with_set_alpha(a), (gAt0).to_vector3(), (gAt1).to_vector3());
			}
		}
	}

	if (axes.on)
	{
		debug_draw_line(false, Colour::red, Vector3::zero, Vector3::xAxis);
		debug_draw_line(false, Colour::green, Vector3::zero, Vector3::yAxis);
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
					debug_draw_line(true, c, gizmo->fromWorld.get().to_vector3(), gizmo->atWorld.to_vector3());
				}
				if (gizmo->fromWorld2.is_set())
				{
					debug_draw_line(true, c, gizmo->fromWorld2.get().to_vector3(), gizmo->atWorld.to_vector3());
				}
				if (gizmo->grabbable)
				{
					debug_draw_sphere(true, true, c, hovering ? 1.0f : 0.3f, Sphere(gizmo->atWorld.to_vector3(), gizmo->radiusWorld));
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
			v3d->access_model_view_matrix_stack().push_set(camera.renderAt.to_matrix().inverted());

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

void Editor::Base2D::render_edited_asset()
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

void Editor::Base2D::setup_ui_add_to_bottom_left_panel(UI::Canvas* c, UI::CanvasWindow* panel, float scale)
{
	{
		auto* b = Framework::UI::CanvasButton::get()->set_caption(TXT("grid"));
		b->set_id(NAME(editor2D_grid));
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
		b->set_id(NAME(editor2D_axes));
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
		b->set_id(NAME(editor2D_gizmos));
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

void Editor::Base2D::setup_ui(REF_ SetupUIContext& _setupUIContext)
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
}

void Editor::Base2D::update_ui_highlight()
{
	UI::Canvas* canvas = get_canvas();
	if (!canvas) { return; }

	auto* c = canvas;

	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor2D_grid))))
	{
		b->set_highlighted(grid.on);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor2D_axes))))
	{
		b->set_highlighted(axes.on);
	}
	if (auto* b = fast_cast<Framework::UI::CanvasButton>(c->find_by_id(NAME(editor2D_gizmos))))
	{
		b->set_highlighted(gizmos.on);
	}

	base::update_ui_highlight();
}

bool Editor::Base2D::on_hold(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt)
{
	if (_cursorIdx == UI::CursorIndex::Mouse &&
		(_buttonIdx == System::MouseButton::Middle || _buttonIdx == System::MouseButton::Right))
	{
		// rotation, translation, zoom
		return true;
	}

	return false;
}

bool Editor::Base2D::on_held(int _cursorIdx, int _buttonIdx, Vector2 const& _rayAt, Vector2 const& _movedBy, Vector2 const& _cursorMovedBy2DNormalised)
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

	float const ZOOM_SPEED = 2.0f;

	if (zoom)
	{
		camera.zoom(_cursorMovedBy2DNormalised.y * ZOOM_SPEED);
		return true;
	}

	if (rotate)
	{
		move = true;
	}

	if (move)
	{
		{
			Vector2 moveBy = -_movedBy; // negate as we're dragging the world in opposite direction, and use _movedBy as it reflects the actual movement the best
			camera.pivot += moveBy;
			camera.update_at();
			return true;
		}
	}

	return false;
}

void Editor::Base2D::add_gizmo(Gizmo2D* g)
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

bool Editor::Base2D::is_gizmo_grabbed(Gizmo2D const * g) const
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

//--

void Editor::Base2D::Camera::zoom(float _by)
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

	{
		this->scale *= zoomBy;
	}

	update_at();
}

void Editor::Base2D::Camera::update_at()
{
	at = Transform(pivot.to_vector3(), Quat::identity);
	renderAt = Transform(pivot.to_vector3(), VIEW_ROTATOR_TOP.to_quat());
}

//--

void Editor::Gizmo2DTwoAxes::clear()
{
	gizmo.clear();
	gizmoXY.clear();
	gizmoX.clear();
	gizmoY.clear();
}

void Editor::Gizmo2DTwoAxes::update(Base2D* _base2D)
{
	if (!gizmo.get())
	{
		gizmo = Gizmo2D::get_one();
		gizmo->grabbable = false;
		gizmo->atWorld = atWorld;
		_base2D->add_gizmo(gizmo.get());
	}
	if (!gizmoXY.get())
	{
		gizmoXY = Gizmo2D::get_one();
		gizmoXY->atWorld = atWorld;
		gizmoXY->axisWorld = Vector2::zero;
		gizmoXY->radius2d *= 0.8f;
		gizmoXY->colour = Colour::orange;
		_base2D->add_gizmo(gizmoXY.get());
	}
	if (!gizmoX.get())
	{
		gizmoX = Gizmo2D::get_one();
		gizmoX->atWorld = atWorld;
		gizmoX->atWorld.x += gizmoAxesDistance;
		gizmoX->axisWorld = Vector2::xAxis;
		gizmoX->radius2d *= 0.5f;
		gizmoX->colour = Colour::red;
		_base2D->add_gizmo(gizmoX.get());
	}
	if (!gizmoY.get())
	{
		gizmoY = Gizmo2D::get_one();
		gizmoY->atWorld = atWorld;
		gizmoY->atWorld.y += gizmoAxesDistance;
		gizmoY->axisWorld = Vector2::yAxis;
		gizmoY->radius2d *= 0.5f;
		gizmoY->colour = Colour::green;
		_base2D->add_gizmo(gizmoY.get());
	}
	gizmo->colour = colour;

	if (_base2D->is_gizmo_grabbed(gizmoXY.get()))
	{
		atWorld.x = gizmoXY->atWorld.x;
		atWorld.y = gizmoXY->atWorld.y;
	}
	else
	{
		atWorld.x = gizmoX->atWorld.x - gizmoAxesDistance;
		atWorld.y = gizmoY->atWorld.y - gizmoAxesDistance;
	}

	gizmoAxesDistance = _base2D->calculate_radius_for(atWorld, axesDistance2d);

	gizmo->atWorld = atWorld;
	gizmoXY->atWorld = atWorld;
	gizmoX->atWorld = atWorld + Vector2::xAxis * gizmoAxesDistance;
	gizmoY->atWorld = atWorld + Vector2::yAxis * gizmoAxesDistance;
	gizmo->fromWorld = fromWorld;
	gizmoXY->fromWorld = fromWorld;
	gizmoX->fromWorld = atWorld;
	gizmoY->fromWorld = atWorld;
}

//--

void Editor::Gizmo2DTwoAxesAndRotation::clear()
{
	base::clear();
	gizmoYaw.clear();
}

void Editor::Gizmo2DTwoAxesAndRotation::update(Base2D* _base2D)
{
	if (!gizmoYaw.get())
	{
		gizmoYaw = Gizmo2D::get_one();
		gizmoYaw->atWorld = atWorld;
		gizmoYaw->atWorld.x += gizmoAxesDistance;
		gizmoYaw->atWorld.y += gizmoAxesDistance;
		gizmoYaw->radius2d *= 0.4f;
		gizmoYaw->colour = Colour::yellow;
		gizmoYaw->value = rotation.yaw;
		gizmoYaw->valueScale = 90.0f;
		_base2D->add_gizmo(gizmoYaw.get());
	}
	
	rotation.pitch = 0.0f;
	rotation.yaw = gizmoYaw->value.get(rotation.yaw);
	rotation.roll = 0.0f;

	base::update(_base2D);

	gizmoYaw->atWorld = atWorld + ((rotation.get_axis(Axis::X) + rotation.get_axis(Axis::Y)) * gizmoAxesDistance).to_vector2();

	{
		float radius = gizmoAxesDistance * sqrt(2.0f);
		Vector2 f = atWorld + ((rotation.get_axis(Axis::Y)) * radius).to_vector2();
		Vector2 r = atWorld + ((rotation.get_axis(Axis::X)) * radius).to_vector2();
		gizmoYaw->fromWorld = f;
		gizmoYaw->fromWorld2 = r;
	}
}
