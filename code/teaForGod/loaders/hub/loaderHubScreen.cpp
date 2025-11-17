#include "loaderHubScreen.h"

#include "loaderHubWidget.h"

#include "widgets\lhw_button.h"
#include "widgets\lhw_slider.h"
#include "widgets\lhw_text.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\system\video\renderTarget.h"
#include "..\..\..\core\system\video\vertexFormatUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Loader;

//

// references
DEFINE_STATIC_NAME_STR(loaderHubScreen, TXT("loader hub screen"));
DEFINE_STATIC_NAME_STR(loaderHubDisplay, TXT("loader hub display"));

// material params
DEFINE_STATIC_NAME(showScreen);

// system tags
DEFINE_STATIC_NAME(lowGraphics);

//

struct HubInfo
{
	Vector3 sphereCentre = Vector3(0.0f, 0.0f, 1.4f);
	float radius = 6.0f;
	float floorWidth = 2.0f;
	float floorLength = 1.5f;
} hubInfo;

//

String const & HubScreenOption::get_text_value() const
{
	for_every(opt, options)
	{
		if (! value || // if no value provided, get first one
			*value == opt->value)
		{
			return opt->text;
		}
	}
	return text;
}


//

REGISTER_FOR_FAST_CAST(HubScreen);

Vector2 HubScreen::s_fontSizeInPixels(24.0f, 32.0f);
Vector2 HubScreen::s_pixelsPerAngle(20.0f, 20.0f);

const float HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER = 5.0f;

HubScreen::HubScreen()
: pixelsPerAngle(s_pixelsPerAngle)
{
}

HubScreen::HubScreen(Hub* _hub, Name const & _id, Vector2 const & _size, Rotator3 const & _at, float _radius, Optional<bool> const & _beVertical, Optional<Rotator3> const & _verticalOffset, Optional<Vector2> const & _pixelsPerAngle)
: pixelsPerAngle(_pixelsPerAngle.get(s_pixelsPerAngle))
, hub(_hub)
, id(_id)
, at(_at)
, radius(_radius)
, size(_size)
, verticalOffset(_verticalOffset.get(Rotator3::zero))
, beVertical(_beVertical.get(true))
{
	set_size(_size, true);
}

void HubScreen::create_display_and_mesh()
{
	if (! hub->screen_wants_to_create_display_and_mesh())
	{
		canBeUsed = false;
		return;
	}
	canBeUsed = true;

	size = mainScreen.length();

	display = new Framework::Display();
	display->ignore_game_frames();

	Framework::DisplayType * displayType = ::Framework::Library::get_current()->get_global_references().get<Framework::DisplayType>(NAME(loaderHubDisplay));
	an_assert(displayType);
	Framework::DisplaySetup displaySetup = displayType->get_setup();
	displaySetup.setup_resolution_with_borders((size * pixelsPerAngle).to_vector_int2(), (border * pixelsPerAngle).to_vector_int2());
	display->use_setup(displaySetup);

	//display->setup_output_size_as_is();
	if (auto* vr = VR::IVR::get())
	{
		display->setup_output_size_min(vr->get_render_size(VR::Eye::Left), 2.0f);
	}
	else
	{
		display->setup_output_size_min(::System::Video3D::get()->get_screen_size(), 2.0f);
	}
	/*
	if (auto* vr = VR::IVR::get())
	{
		display->setup_output_size_max(vr->get_render_size(VR::Eye::Left), 1.0f);
	}
	else
	{
		display->setup_output_size_max(::System::Video3D::get()->get_screen_size(), 1.0f);
	}
	*/
	display->setup_output_size_limit(VectorInt2(1600, 1600));
	display->use_meshes(false); // will have custom mesh
	{
		::Framework::DisplayInitContext displayInitContext;
		displayInitContext.outputTexture = ::System::VideoFormat::rgba8;
		displayInitContext.clearColour = Colour::none;
		display->init(::Framework::Library::get_current(), displayInitContext);
	}

	display->always_draw_commands_immediately();

	create_mesh();

	meshInstance.set_mesh(mesh->get_mesh());
	Framework::MaterialSetup::apply_material_setups(mesh->get_material_setups(), meshInstance, ::System::MaterialSetting::IndividualInstance);
	meshInstance.fill_materials_with_missing();
	//meshInstance.set_blend(::System::BlendOp::SrcAlpha, ::System::BlendOp::OneMinusSrcAlpha);
	meshInstance.set_rendering_order_priority((int)(1000.0f * clamp(1.0f - radius / hubInfo.radius, 0.0f, 1.0f))); // top most

	set_placement(at);

	display->set_on_render_display(this, [this](Framework::Display* _display)
	{
		if (drawOnDisplayNow && activeTarget > 0.5f)
		{
			scoped_call_stack_info(TXT("render screen"));
			scoped_call_stack_info(id.to_char());
			if (!dontClearOnRender)
			{
				float scale = 1.0f;
				::System::Video3DPrimitives::fill_rect_2d(Colour::none, Vector2::zero, _display->get_setup().wholeDisplayResolution.to_vector2(), false);
				// have a screen border extra wide but with alpha 0 so we won't get black outline when there's no blending
				::System::Video3DPrimitives::fill_rect_2d(HubColours::screen_border().with_set_alpha(0.0f),
					_display->get_left_bottom_of_screen() - Vector2::one * 4.0f * scale,
					_display->get_right_top_of_screen() + Vector2::one * 4.0f * scale, false);
				::System::Video3DPrimitives::fill_rect_2d(HubColours::screen_border(),
					_display->get_left_bottom_of_screen() - Vector2::one * 2.0f * scale,
					_display->get_right_top_of_screen() + Vector2::one * 2.0f * scale, false);
				::System::Video3DPrimitives::fill_rect_2d(hub->get_screen_interior_colour(), _display->get_left_bottom_of_screen(), _display->get_right_top_of_screen(), false);
			}

			{
				scoped_call_stack_info(TXT("render widgets"));
				for_every_ref(widget, widgets)
				{
					if (widget->specialHighlight)
					{
						::System::Video3DPrimitives::fill_rect_2d(HubColours::special_highlight_background().with_set_alpha(1.0f), widget->at.bottom_left(), widget->at.top_right(), false);
					}
				}
				for_every_ref(widget, widgets)
				{
					scoped_call_stack_info(widget->id.to_char());
					widget->mark_no_longer_requires_rendering();
					widget->render_to_display(_display);
				}
			}

			for_count(int, idx, 2)
			{
				auto& hand = hub->hands[idx];
				if (hand.onScreen == this)
				{
					Framework::TexturePartDrawingContext tpdc;
					tpdc.alignToPixels = true;
					tpdc.colour.a = hand.onScreenActive;
					hand.cursor->draw_at(::System::Video3D::get(), hand.onScreenAt, tpdc);
				}
			}
		}
	});
}

Transform HubScreen::calculate_screen_centre() const
{
	Transform sc = placement;
	sc.set_translation(placement.location_to_world(Vector3::yAxis * radius));
	return sc;
}

void HubScreen::set_placement(Rotator3 const & _at)
{
	at = _at;
	if (beVertical)
	{
		Rotator3 flatAt = (at + verticalOffset) * Rotator3(0.0f, 1.0f, 1.0f);
		Vector3 screenLoc = hubInfo.sphereCentre + flatAt.get_forward() * radius;
		float pitch = at.pitch + verticalOffset.pitch;
		// move up
		screenLoc.z += (sin_deg(1.0f) * pitch) * radius;
		// centre loc should be above sphereCentre
		Vector3 centreLoc = screenLoc - flatAt.get_forward() * radius;
		an_assert((centreLoc - hubInfo.sphereCentre).length_2d() < 0.001f);
		placement = Transform(centreLoc, flatAt.to_quat());
	}
	else
	{
		placement = Transform(hubInfo.sphereCentre, at.to_quat());
	}
}

void HubScreen::render_display_and_ready_to_render(bool _lazyRender)
{
	if (!_lazyRender)
	{
		bool requiresRendering = forceRedraw > 0;

		{
			forceRedraw = max(0, forceRedraw - 1);

			if (!requiresRendering)
			{
				for_count(int, idx, 2)
				{
					auto& hand = hub->hands[idx];
					if (hand.onScreen == this)
					{
						requiresRendering = true;
					}
				}
			}

			if (!requiresRendering)
			{
				for_every_ref(widget, widgets)
				{
					if (widget->does_required_rendering())
					{
						requiresRendering = true;
						break;
					}
				}
			}
		}

		// handle redrawing once more
		if (requiresRendering)
		{
			redrawOnceAgain = true;
		}
		else if (redrawOnceAgain)
		{
			requiresRendering = true;
			redrawOnceAgain = false;
		}

		if (requiresRendering)
		{
			drawOnDisplayNow = true;
		}
	}

	display->update_display();

	drawOnDisplayNow = false;

	display->ready_mesh_3d_instance_to_render(meshInstance);
	if (::System::MaterialInstance * mi = meshInstance.get_material_instance_for_rendering(0))
	{
		if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
		{
			mi->set_uniform(NAME(showScreen), active * (1.0f - inBackground * 1.0f)); // to hide it more
		}
		else
		{
			mi->set_uniform(NAME(showScreen), active * (1.0f - inBackground * 0.88f));
		}
	}
}

void HubScreen::create_mesh()
{
	Meshes::Mesh3D* mesh = new Meshes::Mesh3D();

#ifdef AN_SYSTEM_INFO_DRAW_DETAILS
	mesh->set_debug_mesh_name(String(TXT("HubScreen")));
#endif

	System::IndexFormat indexFormat;
	indexFormat.with_element_size(System::IndexElementSize::FourBytes);
	indexFormat.calculate_stride();

	System::VertexFormat vertexFormat;
	vertexFormat.with_padding();
	vertexFormat.with_normal();
	vertexFormat.with_colour_rgb();
	vertexFormat.with_texture_uv().with_texture_uv_type(::System::DataFormatValueType::Float);
	vertexFormat.calculate_stride_and_offsets();

	Vector2 stepSize = border * 0.25f;
	Vector2 wholeSize = size + border * 2.0f;
	VectorInt2 gridCount(max(2, (int)(wholeSize.x / stepSize.x) + 1),
						 max(2, (int)(wholeSize.y / stepSize.y) + 1));
	Vector2 gridSize = gridCount.to_vector2() - Vector2::one;
	Vector2 invGridSize(1.0f / gridSize.x, 1.0f / gridSize.y);
	Vector2 step = wholeSize / (gridCount - VectorInt2::one).to_vector2();
	Vector2 half = (gridCount - VectorInt2::one).to_vector2() * 0.5f;
	Vector2 gridBorder = border / step;
	Vector2 invGridBorder(1.0f / gridBorder.x, 1.0f / gridBorder.y);
	int pointCount = gridCount.x * gridCount.y;
	int triCount = (gridCount.x - 1) * (gridCount.y - 1) * 2;

	Array<int8> vertexData;
	vertexData.set_size(vertexFormat.get_stride() * pointCount);
	int8* currentVertexData = vertexData.get_data();

	for (int iy = 0; iy < gridCount.y; ++iy)
	{
		for (int ix = 0; ix < gridCount.x; ++ix)
		{
			Vector3* location = (Vector3*)(currentVertexData + vertexFormat.get_location_offset());
			Vector2 coord((float)ix, (float)iy);
			Vector2 point = (coord - half) * step;

			Vector2 offSideXY(min(coord.x, gridSize.x - coord.x) * invGridBorder.x,
							  min(coord.y, gridSize.y - coord.y) * invGridBorder.y);
			Vector2 distXY(clamp(1.0f - offSideXY.x, 0.0f, 1.0f), clamp(1.0f - offSideXY.y, 0.0f, 1.0f));
			float offSide = clamp(1.0f - distXY.length(), 0.0f, 1.0f);
			offSide = sqr(offSide);

			Vector3 loc = Vector3::zero;
			Vector3 normal = -Vector3::yAxis;
			if (beVertical)
			{
				Rotator3 rPoint(0.0f, point.x, 0.0f);
				loc = rPoint.get_forward() * radius;
				loc.z += (sin_deg(1.0f) * point.y) * radius;
				normal = -rPoint.get_forward();
			}
			else
			{
				Rotator3 rPoint(point.y, point.x, 0.0f);
				loc = rPoint.get_forward() * radius; // spherical! / abs(cos_deg(point.y))); // vertical!
				normal = -rPoint.get_forward();
			}
			
			*location = loc;

			::System::VertexFormatUtils::store(currentVertexData + vertexFormat.get_texture_uv_offset(), vertexFormat.get_texture_uv_type(), coord * invGridSize);
			::System::VertexFormatUtils::store_normal(currentVertexData + vertexFormat.get_normal_offset(), vertexFormat, normal);

			Colour* vertexColourRGB = (Colour*)(currentVertexData + vertexFormat.get_colour_offset());
			vertexColourRGB->r = offSide;
			vertexColourRGB->g = offSide;
			vertexColourRGB->b = offSide;

			currentVertexData += vertexFormat.get_stride();
		}
	}

	// elements
	Array<int32> elements;
	elements.make_space_for(triCount * 3);

	for (int iy = 0; iy < gridCount.y - 1; ++iy)
	{
		for (int ix = 0; ix < gridCount.x - 1; ++ix)
		{
			int p = iy * gridCount.x + ix;
			int pxp = p + 1;
			int pyp = p + gridCount.x;
			int pxyp = pyp + 1;

			if ((ix < gridCount.x / 2 && iy < gridCount.y / 2) ||
				(ix >= gridCount.x / 2 && iy >= gridCount.y / 2))
			{
				elements.push_back(p);
				elements.push_back(pyp);
				elements.push_back(pxyp);

				elements.push_back(p);
				elements.push_back(pxyp);
				elements.push_back(pxp);
			}
			else
			{
				elements.push_back(p);
				elements.push_back(pyp);
				elements.push_back(pxp);

				elements.push_back(pxp);
				elements.push_back(pyp);
				elements.push_back(pxyp);
			}
		}
	}

	mesh->load_part_data(vertexData.get_data(), elements.get_data(), Meshes::Primitive::Triangle, vertexData.get_size() / vertexFormat.get_stride(), elements.get_size(), vertexFormat, indexFormat);

	auto* library = Framework::Library::get_current();

	this->mesh = new Framework::Mesh(library, Framework::LibraryName::invalid());
	this->mesh->be_temporary(true);
	this->mesh->replace_mesh(mesh);
	if (auto * mat = library->get_global_references().get<Framework::Material>(NAME(loaderHubScreen)))
	{
		this->mesh->set_material(0, mat);
	}

}

void HubScreen::advance(float _deltaTime, bool _beyondModal)
{
	timeSinceLastActive.reset();

	if (! canBeUsed)
	{
		create_display_and_mesh();
	}

	if (!canBeUsed)
	{
		return;
	}

	if (neverGoesToBackground)
	{
		inBackgroundTarget = 0.0f;
	}
	inBackground = blend_to_using_time(inBackground, forceAppearForeground? 0.0f : inBackgroundTarget, 0.1f, _deltaTime);
	active = blend_to_using_time(active, activeTarget, 0.1f, _deltaTime);
	if (activeTarget == 0.0f && active < 0.001f)
	{
		active = 0.0f;
	}
	display->advance(_deltaTime);

	for_every_ref(widget, widgets)
	{
		widget->advance(hub, this, _deltaTime, _beyondModal);
	}

	if (TeaForGodEmperor::TutorialSystem::check_active())
	{
		bool forceRedrawNow = false;
		for_every_ref(widget, widgets)
		{
			if (TeaForGodEmperor::TutorialSystem::should_redraw_hub_for(this, widget))
			{
				forceRedrawNow = true;
				break;
			}
		}
		if (forceRedrawNow)
		{
			force_redraw();
		}
	}

	if ((beAtYaw.is_set() || followYawDeadZone.is_set() || followYawDeadZoneByBorder.is_set()) && !_beyondModal)
	{
		float moveBy = 0.0f;
		Optional<float> useTargetYaw;
		Optional<float> useDeadZone;
		if (beAtYaw.is_set())
		{
			useTargetYaw = beAtYaw;
			useDeadZone = 0.0f;
		}
		else
		{
			auto& useForward = followHead.get(false)? hub->viewForward : hub->forward;
			if (useForward.is_set())
			{
				useTargetYaw = useForward.get().yaw;
			}
		}
		if (useTargetYaw.is_set())
		{
			float targetYaw = useTargetYaw.get();
			float diff = Rotator3::normalise_axis(targetYaw - at.yaw);
			float const deadZone = useDeadZone.get(followYawDeadZone.is_set()? followYawDeadZone.get() : (size.x * 0.5f + followYawDeadZoneByBorder.get()));

			float changeRequired = 0.0f;
			if (diff > deadZone)
			{
				changeRequired += diff - deadZone;
			}
			else if (diff < -deadZone)
			{
				changeRequired += diff - (-deadZone);
			}

			if (changeRequired != 0.0f)
			{
				float const speed = 720.0f;
				float coef = clamp(changeRequired / 20.0f, -1.0f, 1.0f);
				coef = sign(coef) * sqr(coef);
				moveBy = _deltaTime * speed * coef;

				if (changeRequired > 0.0f && moveBy > changeRequired)
				{
					moveBy = changeRequired;
				}
				if (changeRequired < 0.0f && moveBy < changeRequired)
				{
					moveBy = changeRequired;
				}

			}
		}
		if (moveBy != 0.0f)
		{
			at.yaw += moveBy;
			at.yaw = Rotator3::normalise_axis(at.yaw);
			set_placement(at);
			for_every_ref(scr, hub->screens)
			{
				if (scr->followScreen == id)
				{
					scr->at.yaw += moveBy;
					scr->at.yaw = Rotator3::normalise_axis(scr->at.yaw);
					scr->set_placement(scr->at);
				}
			}
		}
	}

	finalPlacement = placement;
	if (moveAway)
	{
		Vector3 loc = finalPlacement.get_translation();
		/*
		if (moveAwayDir.is_zero())
		{
			//moveAwayDir.x = Random::get_float(-1.0f, 1.0f);
			//moveAwayDir.z = Random::get_float(-1.0f, 1.0f);
			//moveAwayDir.normalise();
			moveAwayDir = Vector3::yAxis;
		}
		*/
		Vector3 awayDir = finalPlacement.vector_to_world(moveAwayDir);
		loc += awayDir * (1.0f - activeTarget) * loc.length() * 0.8f;
		finalPlacement.set_translation(loc);
	}
}

void HubScreen::clear()
{
	clean_up_widgets();
	widgets.clear();
}

void HubScreen::clean_up_widgets()
{
	for_every_ref(widget, widgets)
	{
		widget->clean_up();
	}
}

void HubScreen::widget_to_front(HubWidget* _w)
{
	RefCountObjectPtr<HubWidget> w(_w);
	if (widgets.does_contain(w))
	{
		widgets.remove(w);
		widgets.push_back(w);
	}
}

void HubScreen::add_widget(HubWidget* _w)
{
	_w->hub = hub;
	_w->screen = this;
	widgets.push_back(RefCountObjectPtr<HubWidget>(_w));
	force_redraw();
}

void HubScreen::remove_widget(HubWidget* _w)
{
	for_every_ref(widget, widgets)
	{
		if (widget == _w)
		{
			widgets.remove_at(for_everys_index(widget));
			break;
		}
	}
	force_redraw();
}

HubWidget* HubScreen::get_widget(Name const & _id) const
{
	for_every_ref(widget, widgets)
	{
		if (widget->id == _id)
		{
			return widget;
		}
	}
	return nullptr;
}

void HubScreen::move_widget_to_foreground(Name const& _id)
{
	RefCountObjectPtr<HubWidget> foundWidget;
	for_every_ref(widget, widgets)
	{
		if (widget->id == _id)
		{
			foundWidget = widget;
			widgets.remove_at(for_everys_index(widget));
			break;
		}
	}
	if (foundWidget.is_set())
	{
		widgets.push_back(foundWidget);
	}
}

HubWidgets::Button* HubScreen::add_button_widget(HubScreenButtonInfo const & _button, Vector2 const & _at, Optional<Vector2> const & _pt, Optional<Vector2> const & _insideSpacing)
{
	String caption = _button.locStrId.is_valid() ? LOC_STR(_button.locStrId) : _button.caption;

	int lineCount, width;
	HubWidgets::Text::measure(hub->get_font(), NP, NP, caption, OUT_ lineCount, OUT_ width);

	Vector2 pt = _pt.get(Vector2(0.5f, 0.4f));
	Vector2 fs = HubScreen::s_fontSizeInPixels;
	Vector2 insideSpacing = _insideSpacing.get(fs * Vector2(1.0f, 0.25f));

	Vector2 size = Vector2((float)(width), fs.y * (float)(lineCount)) + insideSpacing * 2.0f;

	Range2 r(Range(_at.x - size.x * pt.x, _at.x + size.x * (1.0f - pt.x)),
			 Range(_at.y - size.y * pt.y, _at.y + size.y * (1.0f - pt.y)));

	HubWidgets::Button * bWidget = new HubWidgets::Button(_button.id, r, caption);

	if (_button.on_click)
	{
		std::function<void(HubInputFlags::Flags _flags)> on_click = _button.on_click;
		bWidget->on_click = [on_click](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags) {on_click(_flags); };
	}

	add_widget(bWidget);

	return bWidget;
}

void HubScreen::add_button_widgets(Array<HubScreenButtonInfo> & _buttons, Range2 const & _range, Optional<float> const & _spacing)
{
	add_button_widgets(_buttons, Vector2(_range.x.min, _range.y.min), Vector2(_range.x.max, _range.y.max), _spacing);
}

void HubScreen::add_button_widgets(Array<HubScreenButtonInfo> & _buttons, Vector2 const & _leftBottom, Vector2 const & _rightTop, Optional<float> const & _spacing)
{
	Vector2 size = _rightTop - _leftBottom;

	bool horizontal = size.x >= size.y;
	Vector2 buttonSize;

	add_button_widgets_grid(_buttons, horizontal ? VectorInt2(_buttons.get_size(), 1) : VectorInt2(1, _buttons.get_size()), _leftBottom, _rightTop, _spacing.is_set()? Optional<Vector2>(Vector2(_spacing.get(), _spacing.get())) : NP);
}

void HubScreen::add_button_widgets_grid(Array<HubScreenButtonInfo> & _buttons, VectorInt2 const & _grid, Range2 const & _range, Optional<Vector2> const & _spacing, bool _autoHighlightSelected)
{
	add_button_widgets_grid(_buttons, _grid, Vector2(_range.x.min, _range.y.min), Vector2(_range.x.max, _range.y.max), _spacing, _autoHighlightSelected);
}

void HubScreen::add_button_widgets_grid(Array<HubScreenButtonInfo> & _buttons, VectorInt2 const & _grid, Vector2 const & _leftBottom, Vector2 const & _rightTop, Optional<Vector2> const & _spacing, bool _autoHighlightSelected)
{
	Vector2 spacing = round(_spacing.get(Vector2::one * min(HubScreen::s_fontSizeInPixels.x, HubScreen::s_fontSizeInPixels.y)));

	Vector2 size = _rightTop - _leftBottom;

	Vector2 buttonSize = Vector2((size.x + 1.0f - spacing.x * (float)(_grid.x - 1)) / (float)_grid.x,
								 (size.y + 1.0f - spacing.y * (float)(_grid.y - 1)) / (float)_grid.y);

	Vector2 actualButtonSize = round(buttonSize);

	Array<HubWidgets::Button*> buttonWidgets;
	buttonWidgets.make_space_for(_buttons.get_size());

	auto * button = _buttons.begin();
	for_count(int, y, _grid.y)
	{
		Vector2 at;

		at.y = _rightTop.y - actualButtonSize.y;
		at.y -= (float)(y) * (buttonSize.y + spacing.y);
		for_count(int, x, _grid.x)
		{
			at.x = _leftBottom.x;
			at.x += (float)(x) * (buttonSize.x + spacing.x);

			if (button->locStrId.is_valid() ||
				! button->caption.is_empty() ||
				button->custom_draw)
			{
				Range2 r(at);
				r.include(at + actualButtonSize - Vector2::one);
				HubWidgets::Button* bWidget;
				if (button->locStrId.is_valid())
				{
					bWidget = new HubWidgets::Button(button->id, r, button->locStrId);
				}
				else
				{
					bWidget = new HubWidgets::Button(button->id, r, button->caption);
				}

				if (button->on_click)
				{
					std::function<void(HubInputFlags::Flags _flags)> on_click = button->on_click;
					bWidget->on_click = [on_click](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags) {on_click(_flags); };
				}

				bWidget->set_highlighted(button->highlighted);
				bWidget->enable(button->enabled);
				bWidget->set_colour(button->overrideColour);
				bWidget->activateOnTriggerHold = button->activateOnTriggerHold;
				bWidget->activateOnHold = button->activateOnHold;
				bWidget->scale = Vector2::one * button->scale;
				bWidget->custom_draw = button->custom_draw;
				if (button->appearAsText)
				{
					bWidget->appear_as_text();
				}
				if (button->alignPt.is_set())
				{
					bWidget->alignPt = button->alignPt.get();
				}

				add_widget(bWidget);
				buttonWidgets.push_back(bWidget);
				if (button->widgetPtr)
				{
					(*button->widgetPtr) = bWidget;
				}
			}

			++button;
			if (button == _buttons.end_const())
			{
				break;
			}
		}
		if (button == _buttons.end_const())
		{
			break;
		}
	}

	if (_autoHighlightSelected)
	{
		for_every_ptr(button, buttonWidgets)
		{
			std::function<void(HubInputFlags::Flags _flags)> on_click = _buttons[for_everys_index(button)].on_click;
			button->on_click = [on_click, buttonWidgets](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)
			{
				for_every_ptr(bw, buttonWidgets)
				{
					bw->set_highlighted(bw == _widget);
				}
				if (on_click)
				{
					on_click(_flags);
				};
			};
		}
	}
}

Range2 HubScreen::add_option_widgets(Array<HubScreenOption> const & _options, Range2 const & range, Optional<float> const & _lineSize, Optional<float> const & _lineSpacing, Optional<float> const & _optionInsideSpacing, Optional<float> const& _centreAxis, Optional<float> const & _fontScale)
{
	Range2 space = Range2::empty;

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsSpace = max(fs.x, fs.y);
	float fontScale = _fontScale.get(1.0f);
	float lineSpacing = _lineSpacing.get(min(fs.x, fs.y) * 0.5f);
	float optionInsideSpacing = _optionInsideSpacing.get(fs.x * 1.0f);
	float lineSize = _lineSize.get(fs.y * fontScale);
	int maxTextWidth = (int)range.x.length();

	Vector2 at = Vector2(range.x.min, range.y.max - lineSize);
	for_every(option, _options)
	{
		bool anything = false;

		int lineCount = max(1, option->minLines);
		{
			if (option->optionLocStrId.is_valid())
			{
				int tempWidth;
				HubWidgets::Text::measure(hub->get_font(), NP, NP, LOC_STR(option->optionLocStrId), lineCount, tempWidth, maxTextWidth);
			}
			else if (! option->optionStr.is_empty())
			{
				int tempWidth;
				HubWidgets::Text::measure(hub->get_font(), NP, NP, option->optionStr, lineCount, tempWidth, maxTextWidth);
			}
			if (option->type != HubScreenOption::Slider &&
				!option->options.is_empty())
			{
				for_every(o, option->options)
				{
					int lc, w;
					HubWidgets::Text::measure(hub->get_font(), NP, NP, o->text, OUT_ lc, OUT_ w, maxTextWidth);
					lineCount = max(lineCount, lc);
				}
			}
		}

		float atTop = at.y + lineSize;

		if (lineCount > 1)
		{
			at.y -= lineSize * (float)(lineCount - 1);
		}
		if (option->type == HubScreenOption::Button)
		{
			at.y -= lineSize * 0.5f;
		}

		if (option->optionLocStrId.is_valid() ||
			! option->optionStr.is_empty())
		{
			anything = true;
			Range2 r(range.x, Range(at.y, atTop));
			if (option->type == HubScreenOption::Centred)
			{
				r.x.max = range.x.get_at(_centreAxis.get(0.5f)) - fsSpace * 0.5f;
			}
			HubWidgets::Text* text = nullptr;
			Name useId;
			if (option->type == HubScreenOption::Static)
			{
				useId = option->id;
			}
			if (option->optionLocStrId.is_valid())
			{
				text = new HubWidgets::Text(useId, r, option->optionLocStrId);
			}
			else
			{
				text = new HubWidgets::Text(useId, r, option->optionStr);
			}
			if (option->colour.is_set())
			{
				text->set_colour(option->colour.get());
			}
			text->alignPt = Vector2(0.0f, 0.5f);
			text->with_smaller_decimals(option->smallerDecimals);
			text->scale = Vector2(fontScale);
			if (option->type == HubScreenOption::Centred)
			{
				text->alignPt = Vector2(1.0f, 0.5f);
			}
			else if (option->type == HubScreenOption::Header)
			{
				text->alignPt = Vector2(0.5f, 0.5f);
			}
			add_widget(text);
			space.include(text->at);
		}

		if (option->type == HubScreenOption::Button)
		{
			anything = true;
			Range2 r(range.x, Range(at.y, atTop));
			if (! option->textOnButton.is_empty())
			{
				Range2 r(range.x, Range(at.y, atTop));
				HubWidgets::Text* text = new HubWidgets::Text(option->id, r, option->text);
				if (option->colour.is_set())
				{
					text->set_colour(option->colour.get());
				}
				text->alignPt = Vector2(1.0f, 0.5f);
				text->with_smaller_decimals(option->smallerDecimals);
				text->scale = Vector2(fontScale);
				add_widget(text);
				space.include(text->at);
			}
			{
				String useText = !option->textOnButton.is_empty() ? option->textOnButton : option->text;
				{
					int lc, w;
					HubWidgets::Text::measure(hub->get_font(), NP, NP, useText, OUT_ lc, OUT_ w, maxTextWidth);
					r.x.min = r.x.max - ((float)w + 2.0f * fsSpace);
				}
				HubWidgets::Button* button = new HubWidgets::Button(Name::invalid(), r, useText);
				if (option->colour.is_set())
				{
					button->set_colour(option->colour.get());
				}
				button->alignPt = Vector2(0.5f, 0.5f);
				button->scale = Vector2(fontScale);
				if (option->onClick)
				{
					std::function<void()> onClick = option->onClick;
					button->on_click = [onClick](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { onClick(); };
				}
				if (option->onHold)
				{
					std::function<void()> onHold = option->onHold;
					button->on_hold = [onHold](HubWidget* _widget, Vector2 const& _at, float _holdTime, float _prevHoldTime) { onHold(); };
				}
				if (option->onRelease)
				{
					std::function<void()> onRelease = option->onRelease;
					button->on_release = [onRelease](HubWidget* _widget, Vector2 const& _at) { onRelease(); };
				}
				add_widget(button);
				space.include(button->at);
			}
		}
		else if (option->type == HubScreenOption::Text ||
			option->type == HubScreenOption::Centred ||
			(option->type == HubScreenOption::Header && ! option->text.is_empty()))
		{
			anything = true;
			Range2 r(range.x, Range(at.y, atTop));
			if (option->type == HubScreenOption::Centred)
			{
				r.x.min = range.x.get_at(_centreAxis.get(0.5f)) + fsSpace * 0.5f;
			}
			HubWidgets::Text* text = new HubWidgets::Text(option->id, r, option->get_text_value());
			if (option->colour.is_set())
			{
				text->set_colour(option->colour.get());
			}
			text->alignPt = Vector2(1.0f, 0.5f);
			text->with_smaller_decimals(option->smallerDecimals);
			text->scale = Vector2(fontScale);
			if (option->type == HubScreenOption::Centred)
			{
				text->alignPt = Vector2(0.0f, 0.5f);
			}
			else if (option->type == HubScreenOption::Header)
			{
				text->alignPt = Vector2(0.5f, 0.5f);
			}
			if (option->onClick)
			{
				std::function<void()> onClick = option->onClick;
				text->on_click = [onClick](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { onClick(); };
			}
			add_widget(text);
			space.include(text->at);
		}
		else if (option->type == HubScreenOption::Slider)
		{
			anything = true;
			Range2 r(Range(range.x.get_at(1.0f - option->sliderWidthPct), range.x.max), Range(at.y, at.y + lineSize - 1.0f)); // always single line
			HubWidgets::Text* sliderAsText = nullptr;
			if (option->getAsTextForSlider)
			{
				Range2 rt = r;
				rt.x.max = rt.x.min - fs.x;
				rt.x.min = range.x.min;
				sliderAsText = new HubWidgets::Text(Name::invalid(), rt, option->getAsTextForSlider(*option->value));
				sliderAsText->alignPt = Vector2(1.0f, 0.5f);
				sliderAsText->with_smaller_decimals(option->smallerDecimals);
				sliderAsText->scale = Vector2(fontScale);
				add_widget(sliderAsText);
				space.include(sliderAsText->at);
			}
			HubWidgets::Slider* slider = new HubWidgets::Slider(option->id, r, option->sliderRange, (*(option->value)), option->sliderRangeDynamic);
			slider->stepInt = option->sliderStep;
			int* storeValue = option->value;
			RangeInt valueRange = option->sliderRange;
			std::function<void(RangeInt&)> valueRangeDynamic = option->sliderRangeDynamic;
			std::function<String(int)> getAsTextForSlider = option->getAsTextForSlider;
			std::function<void()> doPostUpdate = option->doPostUpdate;
			std::function<void()> onHold = option->onHold;
			slider->on_hold = [storeValue, valueRange, valueRangeDynamic, sliderAsText, getAsTextForSlider, doPostUpdate, onHold](HubWidget* _widget, Vector2 const & _at, float _holdTime, float _prevHoldTime)
			{
				if (auto* slider = fast_cast<HubWidgets::Slider>(_widget))
				{
					if (storeValue)
					{
						*storeValue = slider->valueInt;
						RangeInt useValueRange = valueRange;
						if (valueRangeDynamic)
						{
							valueRangeDynamic(useValueRange);
						}
						*storeValue = useValueRange.clamp(*storeValue);
						if (sliderAsText)
						{
							sliderAsText->set(getAsTextForSlider(*storeValue));
						}
						if (onHold)
						{
							onHold();
						}
						if (doPostUpdate)
						{
							doPostUpdate();
						}
					}
				}
			};
			if (option->onRelease)
			{
				std::function<void()> onRelease = option->onRelease;
				slider->on_release = [onRelease](HubWidget* _widget, Vector2 const& _at) { onRelease(); };
			}
			if (option->onClick)
			{
				std::function<void()> onClick = option->onClick;
				slider->on_click = [onClick](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { onClick(); };
			}
			add_widget(slider);
			space.include(slider->at);
		}
		else if (! option->options.is_empty())
		{
			anything = true;
			int lineCount = 1;
			int maxWidth = 1;
			for_every(o, option->options)
			{
				int lc, w;
				HubWidgets::Text::measure(hub->get_font(), NP, NP, o->text, OUT_ lc, OUT_ w);
				lineCount = max(lineCount, lc);
				maxWidth = max(maxWidth, w);
			}

			float width = (float)maxWidth + optionInsideSpacing * 2.0f;
			Range2 r(Range(range.x.max - width, range.x.max),
					 Range(at.y - lineSpacing * 0.33f, at.y + lineSize * (float)lineCount + lineSpacing * 0.33f - 1.0f));
			HubWidgets::Button* button = new HubWidgets::Button(option->id, r, option->get_text_value());
			HubScreenOption cOption = *option;
			button->scale = Vector2(fontScale);
			std::function<void()> onClick = option->onClick;
			button->on_click = [this, cOption, maxWidth, button, onClick](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags)
			{
				if (onClick) onClick();
				if (cOption.type == HubScreenOption::Switch)
				{
					if (cOption.value)
					{
						int idx = 0;
						for_every(opt, cOption.options)
						{
							if (*cOption.value == opt->value)
							{
								idx = for_everys_index(opt);
								break;
							}
						}
						idx = (idx + 1) % cOption.options.get_size();
						*cOption.value = cOption.options[idx].value;
					}
					if (auto* b = fast_cast<HubWidgets::Button>(_widget))
					{
						b->set(cOption.get_text_value());
					}
					if (cOption.doPostUpdate)
					{
						cOption.doPostUpdate();
					}
				}
				else if (cOption.type == HubScreenOption::List)
				{
					Vector2 usePixelsPerAngle = pixelsPerAngle;
					if (cOption.scalePixelsPerAngle.is_set())
					{
						usePixelsPerAngle *= cOption.scalePixelsPerAngle.get();
					}
					auto fs = HubScreen::s_fontSizeInPixels;
					Rotator3 beAt;
					{
						Vector2 widgetAt = _widget->at.centre();
						Vector2 relativeToScreenCentre = widgetAt - this->mainResolutionInPixels.centre();
						Rotator3 relativeAt(relativeToScreenCentre.y / usePixelsPerAngle.y, relativeToScreenCentre.x / usePixelsPerAngle.x, 0.0f);
						beAt = this->at.to_quat().to_world(relativeAt.to_quat()).to_rotator();
					}
					VectorInt2 grid(1, cOption.options.get_size());
					while (grid.y >= 10)
					{
						grid.x++;
						grid.y = 1;
						while (grid.x * grid.y < cOption.options.get_size())
						{
							grid.y++;
						}
					}
					Vector2 buttonSize = round(Vector2((float)maxWidth, fs.y) * 1.2f);
					Vector2 border = fs * 2.0f;
					Vector2 spacing = Vector2(2.0f, 2.0f);
					Vector2 listInPixels = grid.to_vector2() * buttonSize + (grid - VectorInt2::one).to_vector2() * spacing + border * 2.0f;
					HubScreen* list = new HubScreen(hub, Name::invalid(),
						listInPixels / usePixelsPerAngle, beAt, radius * 0.9f, beVertical, verticalOffset, usePixelsPerAngle);
					list->be_modal(true);
					Array<HubScreenButtonInfo> buttons;
					buttons.make_space_for(cOption.options.get_size());
					int* storeValue = cOption.value;
					auto doPostUpdate = cOption.doPostUpdate;
					for_every(opt, cOption.options)
					{
						int value = opt->value;
						String text = opt->text;
						buttons.push_back(HubScreenButtonInfo(Name::invalid(), text,
							[storeValue, value, text, list, doPostUpdate, button](HubInputFlags::Flags _flags)
							{
								if (storeValue)
								{
									*storeValue = value;
								}
								button->set(text);
								list->deactivate();
								if (doPostUpdate)
								{
									doPostUpdate();
								}
							},
							storeValue && *storeValue == value));
					}
					list->add_button_widgets_grid(buttons, grid, list->mainResolutionInPixels.expanded_by(-border), spacing);

					list->activate();
					hub->add_screen(list);
				}
			};

			add_widget(button);
			space.include(button->at);
		}

		at.y -= (lineSize + lineSpacing) * (anything? 1.0f : 0.5f);
	}

	return space;
}

void HubScreen::set_size(Vector2 const & _size, bool _createDisplayAndMesh)
{
	size = _size;
	wholeScreen = Range2(Range(-size.x * 0.5f - border.x, size.x * 0.5f + border.x),
						 Range(-size.y * 0.5f - border.y, size.y * 0.5f + border.y));
	mainScreen = Range2(Range(-size.x * 0.5f, size.x * 0.5f),
						Range(-size.y * 0.5f, size.y * 0.5f));

	Vector2 borderInPixels = border * pixelsPerAngle;
	Vector2 sizeInPixels = size * pixelsPerAngle;
	// -1 pixel because if we want to have 20 pixels then we should have 0..19
	wholeResolutionInPixels = Range2(Range(0.0f, borderInPixels.x * 2.0f + sizeInPixels.x - 1.0f),
									 Range(0.0f, borderInPixels.y * 2.0f + sizeInPixels.y - 1.0f));
	mainResolutionInPixels = Range2(Range(borderInPixels.x, borderInPixels.x + sizeInPixels.x - 1.0f),
									Range(borderInPixels.y, borderInPixels.y + sizeInPixels.y - 1.0f));

	if (_createDisplayAndMesh)
	{
		create_display_and_mesh();
	}
}

void HubScreen::centre_vertically(Range2 const& _within)
{
	float minY = _within.y.max;
	float maxY = _within.y.min;
	bool anything = false;
	for_every_ref(widget, widgets)
	{
		if (_within.does_contain(widget->at))
		{
			anything = true;
			minY = min(minY, widget->at.y.min);
			maxY = max(maxY, widget->at.y.max);
		}
	}

	float above = _within.y.max - maxY;
	float below = minY - _within.y.min;
	float moveUp = round((above - below) * 0.5f);
	for_every_ref(widget, widgets)
	{
		if (_within.does_contain(widget->at))
		{
			widget->at.y.min += moveUp;
			widget->at.y.max += moveUp;
		}
	}
}

float HubScreen::compress_vertically(Optional<float> const & _maxGap, Optional<bool> const & _compressEdges)
{
	float maxGap = _maxGap.get(HubScreen::s_fontSizeInPixels.y * 1.0f);
	float reduceScreenSizeBy = 0.0f;
	for_every_ref(widget, widgets)
	{
		Optional<float> maxDist;
		Range y = widget->at.y;

		for_every_ref(nextWidget, widgets)
		{
			if (nextWidget != widget &&
				nextWidget->at.y.max > y.max) // check if this one ends above, if so, we should check where the bottom is
			{
				// if overlaps, will end up with max dist <= 0 - this is fine
				// if is above, will have max dist > 0
				maxDist = min(maxDist.get(mainResolutionInPixels.y.length()), nextWidget->at.y.min - y.max);
			}
		}

		if (maxDist.is_set() &&
			maxDist.get() > maxGap)
		{
			float moveDownBy = maxDist.get() - maxGap;
			for_every_ref(nextWidget, widgets)
			{
				if (nextWidget != widget &&
					nextWidget->at.y.max > y.max)
				{
					nextWidget->at.y.min -= moveDownBy;
					nextWidget->at.y.max -= moveDownBy;
				}
			}
			reduceScreenSizeBy += moveDownBy;
		}
	}

	// lowest and highest one
	if (_compressEdges.get(true))
	{
		// we already packed widgets, we will just use highest/lowest to compress edges
		reduceScreenSizeBy = 0.0f;
		float highestY = mainResolutionInPixels.y.min;
		float lowestY = mainResolutionInPixels.y.max;
		for_every_ref(widget, widgets)
		{
			lowestY = min(widget->at.y.min, lowestY);
			highestY = max(widget->at.y.max, highestY);
		}
		float highestPossible = mainResolutionInPixels.y.max - maxGap;
		float lowestPossible = mainResolutionInPixels.y.min + maxGap;
		if (lowestY > lowestPossible)
		{
			float moveDownBy = lowestY - lowestPossible;
			for_every_ref(widget, widgets)
			{
				widget->at.y.min -= moveDownBy;
				widget->at.y.max -= moveDownBy;
			}
			reduceScreenSizeBy += moveDownBy;
		}
		if (highestY < highestPossible)
		{
			reduceScreenSizeBy += highestPossible - highestY;
		}
	}

	if (reduceScreenSizeBy > 0.0f)
	{
		mainResolutionInPixels.y.max -= reduceScreenSizeBy;
		wholeResolutionInPixels.y.max -= reduceScreenSizeBy;

		float halfInAngles = reduceScreenSizeBy * 0.5f / pixelsPerAngle.y;
		size.y -= 2.0f * halfInAngles;
		wholeScreen.y.expand_by(-halfInAngles);
		mainScreen.y.expand_by(-halfInAngles);

		create_display_and_mesh();

		return -halfInAngles * 2.0f;
	}
	else
	{
		return 0.0f;
	}
}

HubWidgets::Button* HubScreen::add_help_button()
{
	Range2 at = mainResolutionInPixels;
	at.expand_by(-Vector2::one * 2.0f);
	//at.x.max -= 1.0f;
	//at.y.max -= 1.0f;
	HubWidgets::Button* b = nullptr;
	if (auto* hi = hub->helpIcon.get())
	{
		at.x.min = at.x.max - (hi->get_size().x * 2.0f + 5.0f);
		at.y.max = at.y.min + (hi->get_size().y * 2.0f + 5.0f);
		b = new HubWidgets::Button(Name::invalid(), at, hi);
		b->scale = Vector2(2.0f, 2.0f);
	}
	else
	{
		at.x.min = at.x.max - 16.0f;
		at.y.max = at.y.min + 16.0f;
		b = new HubWidgets::Button(Name::invalid(), at, String(TXT("?")));
		b->scale = Vector2(2.0f, 2.0f);
	}
	add_widget(b);
	return b;
}

void HubScreen::clear_special_highlights()
{
	for_every_ref(widget, widgets)
	{
		widget->specialHighlight = false;
	}
	force_redraw();
}

void HubScreen::special_highlight(Name const& _widgetId, bool _highlight)
{
	for_every_ref(widget, widgets)
	{
		if (widget->id == _widgetId)
		{
			widget->specialHighlight = _highlight;
		}
	}
	force_redraw();
}

void HubScreen::dont_follow()
{
	followScreen.clear();
	followYawDeadZone.clear();
	followYawDeadZoneByBorder.clear();
}
