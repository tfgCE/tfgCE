#include "debugPanel.h"

#include "..\game\game.h"
#include "..\video\font.h"

#include "..\..\core\globalDefinitions.h"
#include "..\..\core\performance\performanceSystem.h"
#include "..\..\core\system\input.h"
#include "..\..\core\system\video\renderTarget.h"
#include "..\..\core\system\video\video3d.h"
#include "..\..\core\system\video\video3dPrimitives.h"
#include "..\..\core\system\video\viewFrustum.h"
#include "..\..\core\system\video\videoClipPlaneStackImpl.inl"
#include "..\..\core\vr\iVR.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

DebugPanel::DebugPanel(Game* _game, Optional<float> const& _baseVRScale)
: game(_game)
, baseVRScale(_baseVRScale.get(1.0f))
{
	renderVRMouseAt[0] = Vector2::zero;
	renderVRMouseAt[1] = Vector2::zero;
	stickVR[0] = Vector2::zero;
	stickVR[1] = Vector2::zero;
	inputActivateVR[0] = false;
	inputActivateVR[1] = false;
}

void DebugPanel::update_view_size()
{
	if (!useViewSize.is_zero())
	{
		return;
	}

	::System::Video3D * v3d = ::System::Video3D::get();

	Vector2 viewSize(1000.0f, 1000.0f);
	{
		useViewSize = viewSize;
		float v3dAR = aspect_ratio(v3d->get_screen_size());
		if (v3dAR > 1.0f)
		{
			useViewSize.x = v3dAR * useViewSize.y;
		}
		else
		{
			useViewSize.x = v3dAR * useViewSize.y;
		}
	}
}

DebugPanelOption & DebugPanelOption::add_off_on()
{
	add(TXT("off"), 0);
	add(TXT("on"), 1);
	return *this;
}

DebugPanelOption & DebugPanelOption::add(tchar const * _value, Optional<int> const & _optionValue, Optional<Colour> const & _optionColour)
{
	values.push_back(Value(_value, _optionValue, _optionColour));
	return *this;
}

static void draw_text_multiline(System::Font* font, float _width, ::System::Video3D* _v3d, String const& _text, Colour const& _colour, Vector2 const& _leftBottom, Vector2 const& _scale, Vector2 const& _anchorRelPlacement, Optional<bool> _alignToPixels)
{
	Vector2 fontSize = font->calculate_char_size() * _scale;
	int maxChars = (int)(floor(_width / fontSize.x) + 0.1f);
	if (_text.get_length() <= maxChars)
	{
		font->draw_text_at(_v3d, _text, _colour, _leftBottom, _scale, _anchorRelPlacement, _alignToPixels);
	}
	else
	{
		ArrayStatic<String, 64> lines; SET_EXTRA_DEBUG_INFO(lines, TXT("DebugPanel::draw_text_multiline.lines"));
		String textLeft = _text;
		String textLeftover;
		while (!textLeft.is_empty() || !textLeftover.is_empty())
		{
			while (textLeft.get_length() > maxChars)
			{
				int lastSpace = textLeft.find_last_of(' ');
				if (lastSpace != NONE)
				{
					textLeftover = textLeft.get_sub(lastSpace + 1) + TXT(" ") + textLeftover;
					textLeft = textLeft.get_left(lastSpace);
				}
				else
				{
					textLeftover = textLeft.get_sub(maxChars);
					textLeft = textLeft.get_left(maxChars);
				}
			}
			lines.push_back(textLeft);
			textLeft = textLeftover;
			textLeftover = TXT("");
		}

		Vector2 at = _leftBottom;
		at.y += ((float)lines.get_size() - 1.0f) * 0.5f * fontSize.y;
		for_every(line, lines)
		{
			font->draw_text_at(_v3d, *line, _colour, at, _scale, _anchorRelPlacement, _alignToPixels);
			at.y -= fontSize.y;
		}
	}
}

void DebugPanel::render_2d()
{
	SUSPEND_STORE_DIRECT_GL_CALL();
	SUSPEND_STORE_DRAW_CALL();

	update_view_size();

	::System::Video3D * v3d = ::System::Video3D::get();

	::System::RenderTarget::bind_none();

	v3d->set_default_viewport();
	v3d->set_near_far_plane(0.02f, 100.0f);

	v3d->set_2d_projection_matrix_ortho(useViewSize);
	v3d->access_model_view_matrix_stack().clear();

	v3d->setup_for_2d_display();

	v3d->access_model_view_matrix_stack().clear();
	v3d->access_clip_plane_stack().clear();

	render(false);

	RESTORE_STORE_DIRECT_GL_CALL();
	RESTORE_STORE_DRAW_CALL();
}

void DebugPanel::render_vr(VR::Eye::Type _eye)
{
	auto* vr = VR::IVR::get();
	if (!vr || useViewSize.is_zero())
	{
		return;
	}

	SUSPEND_STORE_DIRECT_GL_CALL();
	SUSPEND_STORE_DRAW_CALL();

	update_view_size();

	::System::Video3D * v3d = ::System::Video3D::get();
	
	int currentFrame = System::Core::get_frame();

	if (abs(currentFrame - lastFrameForVR) > 5 && ! keepPosition)
	{
		renderVRAtBasePlacement.clear();
	}

	Transform currentVRView = vr->is_render_view_valid()? vr->get_render_view() : Transform::identity;
	Transform currentVRViewFlat = look_matrix(currentVRView.get_translation(), currentVRView.get_axis(Axis::Forward).normal_2d(), Vector3::zAxis).to_transform();
	if (!renderVRAtBasePlacement.is_set())
	{
		if (defaultPanelPlacement.is_set())
		{
			Transform dpp = defaultPanelPlacement.get();
			dpp.set_translation(dpp.get_translation() * baseVRScale);
			renderVRAtBasePlacement = currentVRViewFlat.to_world(dpp).to_matrix();
		}
		else if (relativeToCamera)
		{
			renderVRAtBasePlacement = Transform(currentVRView.get_translation() + currentVRView.vector_to_world(Vector3::yAxis * 1.2f * baseVRScale), currentVRView.get_orientation()).to_matrix();
		}
		else
		{
			float yaw = currentVRView.get_orientation().to_rotator().yaw;
			renderVRAtBasePlacement = Transform(currentVRView.get_translation() + Vector3::yAxis.rotated_by_yaw(yaw) * 1.2f * baseVRScale, Rotator3(0.0f, yaw, 0.0f).to_quat()).to_matrix();
		}
		if (followCamera)
		{
			renderVRAtBasePlacement = currentVRViewFlat.to_matrix().to_local(renderVRAtBasePlacement.get());
		}
		renderVRAtScaling = translation_matrix(baseVRScale * Vector3(-1.0f, 0.0f, -0.75f));
		renderVRAtScaling = renderVRAtScaling.to_world(scale_matrix(Vector3(baseVRScale * 2.0f / useViewSize.x, 1.0f, (baseVRScale * 2.0f / useViewSize.y) * (useViewSize.y / useViewSize.x))));
	}

	float actualCameraYaw = Rotator3::get_yaw(currentVRView.get_axis(Axis::Forward).normal_2d());
	if (!cameraYaw.is_set())
	{
		cameraYaw = actualCameraYaw;
	}

	cameraYaw = actualCameraYaw + clamp(Rotator3::normalise_axis(cameraYaw.get() - actualCameraYaw), -followCameraYaw, followCameraYaw);

	renderVRAtBase = renderVRAtBasePlacement.get();
	if (followCamera)
	{
		renderVRAtBase = look_matrix(currentVRView.get_translation(), Rotator3(0.0f, cameraYaw.get(), 0.0f).get_axis(Axis::Forward), Vector3::zAxis).to_world(renderVRAtBase);
	}

	renderVRAt = renderVRAtBase;
	renderVRAt = renderVRAt.to_world(renderAtOffset.to_matrix());
	renderVRAt = renderVRAt.to_world(renderVRAtScaling);

	auto& matrixStack = v3d->access_model_view_matrix_stack();
	
	VR::RenderInfo const & renderInfo = VR::IVR::get()->get_render_info();

	matrixStack.push_set(currentVRView.inverted().to_matrix());
	matrixStack.push_to_world(translation_matrix(currentVRView.vector_to_world(-renderInfo.get_eye_offset_to_use(_eye).get_translation())));
	matrixStack.push_to_world(renderVRAt);

	render(true);

	matrixStack.pop();
	matrixStack.pop();
	matrixStack.pop();

	lastFrameForVR = currentFrame;

	RESTORE_STORE_DIRECT_GL_CALL();
	RESTORE_STORE_DRAW_CALL();
}

void DebugPanel::render(bool _vr)
{
	update_view_size();

	::System::Video3D * v3d = ::System::Video3D::get();
	
	v3d->push_state();

	v3d->mark_disable_blend();
	v3d->depth_mask(0);
	v3d->face_display(::System::FaceDisplay::Both);

	v3d->set_fallback_colour();
	v3d->depth_test(::System::Video3DCompareFunc::Always);

	v3d->ready_for_rendering();

	Vector2 buttonPanelSize = buttonSize;
	Vector2 buttonMove = buttonSize + buttonSpacing;

	{
		while (buttonPanelSize.x + buttonMove.x <= useViewSize.x)
		{
			buttonPanelSize.x += buttonMove.x;
		}
		while (buttonPanelSize.y + buttonMove.y <= useViewSize.y)
		{
			buttonPanelSize.y += buttonMove.y;
		}
	}

	Vector2 buttonPanelOffset = (useViewSize - buttonPanelSize) * 0.5f;

	Vector2 fontScale2(fontScale, fontScale);

	Vector2 at = Vector2(0.0f, buttonPanelSize.y - buttonSize.y);
	// no one row lower
	bool lastWasSeparator = false;
	for_every(debugOption, debugPanelOptions)
	{
		if (debugOption->id.is_valid())
		{
			if (lastWasSeparator)
			{
				if (at.x > 0.0f)
				{
					at.x = 0.0f;
					at.y -= buttonMove.y;
				}
			}
			bool highlight = debugOption->valueIdx != 0 && debugOption->useHighlight;
			Colour highlightColour = Colour::green.mul_rgb(0.3f);
			if (!debugOption->noOptions && !debugOption->values.is_empty() &&
				debugOption->values[debugOption->valueIdx].colour.is_set())
			{
				highlight = true;
				highlightColour = debugOption->values[debugOption->valueIdx].colour.get(highlightColour);
			}
			::System::Video3DPrimitives::fill_rect_2d(highlight ? highlightColour.with_alpha(0.65f) : debugOption->backgroundColour.get(Colour::black).with_alpha(0.55f), buttonPanelOffset + at, buttonPanelOffset + at + buttonSize);
			if (auto* font = game->get_default_font())
			{
				if (debugOption->noOptions)
				{
					draw_text_multiline(font->get_font(), buttonSize.x, v3d, debugOption->name, debugOption->textColour.get(Colour::white), buttonPanelOffset + at + buttonSize * 0.5f, fontScale2, Vector2(0.5f, 0.5f), false);
				}
				else
				{
					draw_text_multiline(font->get_font(), buttonSize.x, v3d, debugOption->name,
						debugOption->textColour.get(Colour::white), buttonPanelOffset + at + buttonSize * 0.5f + Vector2(0.0f, buttonSize.y * 0.25f), fontScale2, Vector2(0.5f, 0.5f), false);
					if (!debugOption->values.is_empty())
					{
						draw_text_multiline(font->get_font(), buttonSize.x, v3d, debugOption->values[debugOption->valueIdx].text,
							Colour::white.with_alpha(0.85f), buttonPanelOffset + at + buttonSize * 0.5f - Vector2(0.0f, buttonSize.y * 0.25f), fontScale2, Vector2(0.5f, 0.5f), false);
					}
					else if (!debugOption->noOptions)
					{
						draw_text_multiline(font->get_font(), buttonSize.x, v3d, debugOption->valueIdx ? String(TXT("on")) : String(TXT("off")),
							Colour::white.with_alpha(0.85f), buttonPanelOffset + at + buttonSize * 0.5f - Vector2(0.0f, buttonSize.y * 0.25f), fontScale2, Vector2(0.5f, 0.5f), false);
					}
				}
			}
			if (at.x + buttonMove.x < buttonPanelSize.x - buttonSize.x * 0.5f)
			{
				at.x += buttonMove.x;
			}
			else
			{
				at.x = 0.0f;
				at.y -= buttonMove.y;
			}
			lastWasSeparator = false;
		}
		else
		{
			lastWasSeparator = true;
		}
	}

	if (showPerformance)
	{
		if (auto * font = game->get_default_font())
		{
			Performance::SystemCustomDisplay scd;
			scd.viewportSize = useViewSize.to_vector_int2_cells();
			scd.mouseAt = renderVRMouseAt[1];
			scd.zoom = stickVR[0].y;
			scd.pan = stickVR[1].x;
			scd.inputActivate = inputActivateVR[1];
			Performance::System::display(font->get_font(), true, false, nullptr, &scd);
		}
	}

	if (showLog)
	{
		if (auto * font = game->get_default_font())
		{
			Colour defaultColour = Colour::lerp(0.7f, Colour::greyLight, Colour::white);
			float charY = font->calculate_char_size().y;
			float showLogScale = useViewSize.y / (charY * (float)showLogLineCount);
			int lineLength = max(10, (int)((float)showLogLineCount * useViewSize.x / useViewSize.y));
			int lineIdx = 0;
			float x = 0.0f;
			float y = useViewSize.y;
			float lineY = charY * showLogScale;
			float lowestY = -(float)(showLogLineCount + 1) * lineY;
			Vector2 const scale(showLogScale, showLogScale);
			Colour currentOutline = Colour::black;
			if (showLogBackwards)
			{
				font->get_font()->draw_text_at_with_border(v3d, showLogTitle, Colour::cyan, currentOutline, 1.0f, Vector2(x, y), scale);
				y += lineY * 2.0f;
				y -= lineY * 6.0f;// just an overlap, others should go above (float)showLogLineCount;
				auto const& lines = showLog->get_lines();
				showLogLineBackwards = clamp(showLogLineBackwards, 0, max(0, lines.get_size() - 1));
				// show backwards
				int atLine = 0;
				for_every_reverse(line, lines)
				{
					if (atLine >= showLogLineBackwards &&
						atLine < showLogLineBackwards + showLogLineCount)
					{
						font->draw_text_at_with_border(v3d, line->text, line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
						y += lineY;
					}
					++atLine;
				}
			}
			else
			{
				font->get_font()->draw_text_at_with_border(v3d, showLogTitle, Colour::cyan, currentOutline, 1.0f, Vector2(x, y), scale);
				y -= lineY * 2.0f;
				for_every(line, showLog->get_lines())
				{
					if (y > lowestY)
					{
						if (line->text.get_length() > lineLength)
						{
							String logLine = line->text;
							String startLineWith;
							while (logLine.get_length() + startLineWith.get_length() > lineLength)
							{
								int spaceAt = logLine.find_first_of(String::space(), lineLength);
								if (spaceAt < 0)
								{
									break;
								}
								if (lineIdx >= showLogLine && lineIdx < showLogLine + showLogLineCount)
								{
									font->draw_text_at_with_border(v3d, startLineWith + logLine.get_left(spaceAt), line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
									y -= lineY;
								}
								logLine = logLine.get_sub(spaceAt + 1);
								if (startLineWith.is_empty())
								{
									startLineWith = TXT(" ~ ");
								}
								++lineIdx;
							}
							// leftover
							if (lineIdx >= showLogLine && lineIdx < showLogLine + showLogLineCount)
							{
								font->draw_text_at_with_border(v3d, startLineWith + logLine, line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
								y -= lineY;
							}
							++lineIdx;
						}
						else
						{
							if (lineIdx >= showLogLine && lineIdx < showLogLine + showLogLineCount)
							{
								font->draw_text_at_with_border(v3d, line->text, line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
								y -= lineY;
							}
							++lineIdx;
						}
					}
					else
					{
						break;
					}
				}
			}
		}
	}

	if (_vr && !locked)
	{
		float size = max(useViewSize.x, useViewSize.y) * 0.025f;
		for_count(int, i, 2)
		{
			Colour colour = Colour::white;
			if (showPerformance && i == 0)
			{
				colour = Colour::white.mul_rgb(0.3f);
			}
			if (renderVRMouseAt[i].x >= 0.0f && renderVRMouseAt[i].y <= useViewSize.x &&
				renderVRMouseAt[i].y >= 0.0f && renderVRMouseAt[i].y <= useViewSize.y)
			{
				::System::Video3DPrimitives::triangle_2d(colour, renderVRMouseAt[i], renderVRMouseAt[i] + Vector2(size, -0.5f * size), renderVRMouseAt[i] + Vector2(0.5f * size, -size));
			}
			else
			{
				Vector2 l[4];
				l[0] = renderVRMouseAt[i];
				l[1] = renderVRMouseAt[i] + Vector2(size, -0.5f * size);
				l[2] = renderVRMouseAt[i] + Vector2(0.5f * size, -size);
				l[3] = l[0];
				::System::Video3DPrimitives::line_strip_2d(colour, l, 4);
			}
		}
	}

	v3d->set_fallback_colour();
	v3d->mark_disable_blend();

	v3d->pop_state();

	v3d->mark_disable_blend();
	v3d->depth_mask(true);

	v3d->set_fallback_colour();
	v3d->depth_test(::System::Video3DCompareFunc::LessEqual);
}

void DebugPanel::advance(bool _vr)
{
	update_view_size();

	Vector2 buttonPanelSize = buttonSize;
	Vector2 buttonMove = buttonSize + buttonSpacing;

	{
		while (buttonPanelSize.x + buttonMove.x <= useViewSize.x)
		{
			buttonPanelSize.x += buttonMove.x;
		}
		while (buttonPanelSize.y + buttonMove.y <= useViewSize.y)
		{
			buttonPanelSize.y += buttonMove.y;
		}
	}

	Vector2 buttonPanelOffset = (useViewSize - buttonPanelSize) * 0.5f;

	Vector2 fontScale2(fontScale, fontScale);

	if (_vr && draggedByHand.is_set())
	{
		if (auto * vr = VR::IVR::get())
		{
			int iHand = vr->get_hand(draggedByHand.get());
			auto& handPose = vr->get_controls_pose_set().hands[iHand].placement;
			if (handPose.is_set())
			{
				Transform newHandPose = handPose.get();
				if (draggedHandLastPose.is_set() &&
					renderVRAtBasePlacement.is_set())
				{
					float movementScale = 5.0f * baseVRScale;
					if (!autoOrientate)
					{
						renderAtOffset.set_translation(renderAtOffset.get_translation() + renderVRAtBase.vector_to_local((newHandPose.get_translation() - draggedHandLastPose.get().get_translation()) * movementScale));
						if (orientationScale)
						{
							renderAtOffset.set_orientation(renderAtOffset.get_orientation().to_world(((draggedHandLastPose.get().get_orientation().to_local(newHandPose.get_orientation())).to_rotator() * orientationScale).to_quat()).normal());
						}
					}
					else if (vr->is_render_view_valid())
					{
						Transform currentVRView = vr->get_render_view();
						Transform currentVRViewFlat = look_matrix(currentVRView.get_translation(), currentVRView.get_axis(Axis::Forward).normal_2d(), Vector3::zAxis).to_transform();
						if (!followCamera)
						{
							renderAtOffset = Transform::identity;
						}
						Transform relative; 
						if (followCamera)
						{
							relative = currentVRViewFlat.to_local(renderVRAtBase.to_transform().to_world(renderAtOffset));
						}
						else
						{
							relative = currentVRViewFlat.to_local(renderVRAtBasePlacement.get().to_transform());
						}
						Vector3 newLoc = relative.get_translation() + currentVRViewFlat.vector_to_local((newHandPose.get_translation() - draggedHandLastPose.get().get_translation()) * movementScale);
						relative = look_matrix(newLoc, newLoc.normal(), Vector3::zAxis).to_transform();
						if (followCamera)
						{
							renderAtOffset = renderVRAtBase.to_local(currentVRViewFlat.to_world(relative).to_matrix()).to_transform();
						}
						else
						{
							renderVRAtBasePlacement = currentVRViewFlat.to_world(relative).to_matrix();
						}
					}
				}
				draggedHandLastPose = newHandPose;
			}
			else
			{
				draggedHandLastPose.clear();
			}
		}
	}

	for_count(int, iPointer, _vr ? 2 : 1)
	{
		Vector2 mouseAt = Vector2::zero;
		bool mousePressed2 = false;
		bool mousePressed0 = false;
		bool mouseWheelDown = false;
		bool mouseWheelUp = false;
		if (lockedDragging)
		{
			draggedByHand.clear();
			draggedHandLastPose.clear();
		}
		if (! locked)
		{
			if (_vr)
			{
				if (auto * vr = VR::IVR::get())
				{
					Hand::Type handType = iPointer == 0 ? Hand::Left : Hand::Right;
					int iHand = vr->get_left_hand() == 0 ? iPointer : 1 - iPointer;
					auto& h = vr->get_controls_pose_set().hands[iHand].placement;
					if (h.is_set() &&
						renderVRAtBasePlacement.is_set())
					{
						Vector3 dir = h.get().vector_to_world(Vector3(0.0f, 1.0f, -0.33f));
						Vector3 loc = h.get().get_translation();
						Plane p = Plane(renderVRAt.get_y_axis().normal(), renderVRAt.get_translation());
						Segment s = Segment(loc, loc + dir * 10.0f);
						float t = p.calc_intersection_t(s);
						if (t <= 1.0f)
						{
							Vector3 at = s.get_at_t(t);
							Vector3 mAt = renderVRAt.full_inverted().location_to_world(at);
							mouseAt.x = mAt.x;
							mouseAt.y = mAt.z;
							renderVRMouseAt[iPointer] = mouseAt;
						}
					}
					if (!lockedDragging)
					{
						if (draggedByHand.is_set())
						{
							if (draggedByHand.get() == handType)
							{
								if (!vr->get_controls().hands[iHand].is_button_pressed(VR::Input::Button::Grip).get(false))
								{
									draggedByHand.clear();
									draggedHandLastPose.clear();
								}
							}
						}
						else if (handType == Hand::Right) // only right hand
						{
							if (renderVRMouseAt[iPointer].x >= 0.0f && renderVRMouseAt[iPointer].y <= useViewSize.x &&
								renderVRMouseAt[iPointer].y >= 0.0f && renderVRMouseAt[iPointer].y <= useViewSize.y)
							{
								if (vr->get_controls().hands[iHand].has_button_been_pressed(VR::Input::Button::Grip).get(false))
								{
									draggedByHand = handType;
									draggedHandLastPose.clear();
								}
							}
						}
					}
					stickVR[iPointer] = vr->get_controls().hands[iHand].get_stick(VR::Input::Stick::Joystick);
					mousePressed0 = vr->get_controls().hands[iHand].has_button_been_pressed(VR::Input::Button::Trigger).get(false) ||
									vr->get_controls().hands[iHand].has_button_been_pressed(VR::Input::Button::PointerPinch).get(false);
					inputActivateVR[iPointer] = mousePressed0;
					// no wheels used for debug panels
					mouseWheelDown = false;
					mouseWheelUp = false;
					// log line
					if (renderVRMouseAt[iPointer].x >= 0.0f && renderVRMouseAt[iPointer].y <= useViewSize.x &&
						renderVRMouseAt[iPointer].y >= 0.0f && renderVRMouseAt[iPointer].y <= useViewSize.y)
					{
						float deltaTime = clamp(::System::Core::get_raw_delta_time(), 0.0f, 1.0f);
						float moveBy = 0.0f;
						moveBy -= vr->get_controls().hands[iHand].get_stick(VR::Input::Stick::WithSource(VR::Input::Stick::Joystick)).y * deltaTime * magic_number /* speed */ 20.0f;
						moveBy -= vr->get_controls().hands[iHand].get_mouse_relative_location(VR::Input::Mouse::WithSource(VR::Input::Devices::all)).y * magic_number /* speed */ 0.2f;
						showLogLineBits += moveBy;

						while (showLogLineBits > 0.5f)
						{
							showLogLineBits -= 1.0f;
							if (showLogBackwards)
							{
								showLogLineBackwards -= 1;
							}
							else
							{
								showLogLine += 1;
							}
						}

						while (showLogLineBits < -0.5f)
						{
							showLogLineBits += 1.0f;
							if (showLogBackwards)
							{
								showLogLineBackwards += 1;
							}
							else
							{
								showLogLine -= 1;
							}
						}

						showLogLineBackwards = max(0, showLogLineBackwards);
						showLogLine = max(0, showLogLine);
					}
				}
			}
			else
			{
				Vector2 mouse = ::System::Input::get()->get_mouse_window_location();
				Range2 screenRange(Range(0.0f, (float)::System::Video3D::get()->get_screen_size().x), Range(0.0f, (float)::System::Video3D::get()->get_screen_size().y));
				mouseAt = Vector2(screenRange.x.get_pt_from_value(mouse.x),
					screenRange.y.get_pt_from_value(mouse.y));
				mouseAt *= useViewSize;
				mousePressed2 = ::System::Input::get()->has_mouse_button_been_pressed(2);
				mousePressed0 = ::System::Input::get()->has_mouse_button_been_pressed(0);
#ifdef AN_STANDARD_INPUT
				mouseWheelDown = ::System::Input::get()->has_key_been_pressed(System::Key::MouseWheelDown);
				mouseWheelUp = ::System::Input::get()->has_key_been_pressed(System::Key::MouseWheelUp);
#else
				mouseWheelDown = false;
				mouseWheelUp = false;
#endif
			}
		}

		Vector2 at = Vector2(0.0f, buttonPanelSize.y - buttonSize.y);
		// no one row lower
		bool lastWasSeparator = false;
		for_every(debugOption, debugPanelOptions)
		{
			if (debugOption->id.is_valid())
			{
				if (lastWasSeparator)
				{
					if (at.x > 0.0f)
					{
						at.x = 0.0f;
						at.y -= buttonMove.y;
					}
				}
				if (mouseAt.x >= buttonPanelOffset.x + at.x &&
					mouseAt.x <= buttonPanelOffset.x + at.x + buttonSize.x &&
					mouseAt.y >= buttonPanelOffset.y + at.y &&
					mouseAt.y <= buttonPanelOffset.y + at.y + buttonSize.y)
				{
					bool shouldClamp = false;
					bool callOnPress = false;
					if (mousePressed2)
					{
						--debugOption->valueIdx;
						callOnPress = true;
					}
					if (mousePressed0)
					{
						++debugOption->valueIdx;
						callOnPress = true;
					}
					if (mouseWheelDown)
					{
						--debugOption->valueIdx;
						shouldClamp = true;
						callOnPress = true;
					}
					if (mouseWheelUp)
					{
						++debugOption->valueIdx;
						shouldClamp = true;
						callOnPress = true;
					}
					if (debugOption->values.is_empty())
					{
						if (debugOption->on_press || debugOption->on_hold)
						{
							debugOption->valueIdx = 0;
						}
						else
						{
							debugOption->valueIdx = shouldClamp ? clamp(debugOption->valueIdx, 0, 1) : mod(debugOption->valueIdx, 2);
						}
					}
					else
					{
						debugOption->valueIdx = shouldClamp ? clamp(debugOption->valueIdx, 0, debugOption->values.get_size() - 1) : mod(debugOption->valueIdx, debugOption->values.get_size());
					}
					if (debugOption->on_press && callOnPress)
					{
						if (debugOption->values.is_empty())
						{
							debugOption->valueIdx = (mousePressed0 || mouseWheelUp)? 1 : -1;
						}
						debugOption->on_press(debugOption->get_value());
					}
					if (debugOption->on_hold && ::System::Input::get()->is_mouse_button_pressed(0))
					{
						if (debugOption->values.is_empty())
						{
							debugOption->valueIdx = 1;
						}
						debugOption->on_hold(debugOption->get_value());
					}
				}
				if (at.x + buttonMove.x < buttonPanelSize.x - buttonSize.x * 0.5f)
				{
					at.x += buttonMove.x;
				}
				else
				{
					at.x = 0.0f;
					at.y -= buttonMove.y;
				}
				lastWasSeparator = false;
			}
			else
			{
				lastWasSeparator = true;
			}
		}
	}
}

void DebugPanel::add_separator()
{
	DebugPanelOption emptyOption;
	debugPanelOptions.push_back(emptyOption);
}

DebugPanelOption & DebugPanel::add_option(Name const & _id, tchar const * _name, Optional<Colour> const & _backgroundColour, Optional<Colour> const & _textColour)
{
#ifdef AN_DEVELOPMENT
	for_every(debugOption, debugPanelOptions)
	{
		an_assert(debugOption->id != _id);
	}
#endif
	DebugPanelOption option;
	an_assert(_id.is_valid());
	option.id = _id;
	option.name = _name;
	option.backgroundColour = _backgroundColour;
	option.textColour = _textColour;
	debugPanelOptions.push_back(option);
	return debugPanelOptions.get_last();
}

int DebugPanel::get_option_index(Name const & _id)
{
	if (optionsLocked)
	{
		return 0;
	}
	for_every(debugOption, debugPanelOptions)
	{
		if (debugOption->id == _id)
		{
			return debugOption->valueIdx;
		}
	}
	return 0;
}

void DebugPanel::set_option_index(Name const & _id, int _value)
{
	for_every(debugOption, debugPanelOptions)
	{
		if (debugOption->id == _id)
		{
			debugOption->valueIdx = _value;
		}
	}
}

int DebugPanel::get_option(Name const & _id)
{
	if (optionsLocked)
	{
		return 0;
	}
	for_every(debugOption, debugPanelOptions)
	{
		if (debugOption->id == _id &&
			debugOption->values.is_index_valid(debugOption->valueIdx))
		{
			return debugOption->values[debugOption->valueIdx].value.get(debugOption->valueIdx);
		}
	}
	return 0;
}

void DebugPanel::set_option(Name const & _id, int _value)
{
	for_every(debugOption, debugPanelOptions)
	{
		if (debugOption->id == _id)
		{
			int bestIdx = debugOption->valueIdx;
			int bestDist = 9999999;
			for_every(value, debugOption->values)
			{
				int dist = abs(value->value.get(for_everys_index(value)) - _value);
				if (bestDist > dist)
				{
					bestDist = dist;
					bestIdx = for_everys_index(value);
				}
			}
			debugOption->valueIdx = bestIdx;
		}
	}
}

void DebugPanel::show_log_title(tchar const * _title)
{
	if (!_title)
	{
		_title = TXT("");
	}
	if (showLogTitle != _title)
	{
		showLogTitle = _title;
	}
}
