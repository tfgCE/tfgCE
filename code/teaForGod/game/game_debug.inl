#include "..\..\core\renderDoc.h"

void Game::temp_render_ui_on_output(bool _justInfo)
{
	/*
	{
		//if (auto* f = Library::get_current()->get_fonts().find_may_fail(Framework::LibraryName::from_string(String(TXT("displays:h_display 24x32 fixed")))))
		if (auto* f = Library::get_current()->get_fonts().find_may_fail(Framework::LibraryName::from_string(String(TXT("displays:h_display 16x16 fixed")))))
		{
			f->debug_draw();
		}
	}
	*/

#ifdef AN_USE_FRAME_INFO
#ifdef RENDER_HITCH_FRAME_INDICATOR
#ifdef AN_ALLOW_BULLSHOTS
	if (! Framework::BullshotSystem::does_want_to_draw())
#endif
	if (showHitchFrameIndicator && allowShowHitchFrameIndicator)
	{
		Optional<Colour> worldJobManagerBusy;
		Optional<Colour> hitchFrame;
		Optional<Colour> fpiStoring;
		Optional<Colour> fpiStored;
		Optional<Colour> renderThread;
		Optional<Colour> renderEndToPreEnd;
		Optional<Colour> renderPreEndToEnd;
		Optional<Colour> gameLoop;

		{
			float timeNow = 0.3f;
			float showFor = 1.0f;
			{
				float t = hitchFrameIndicators.hitchFrame.get_time_since();
				if (t <= timeNow) hitchFrame = Colour::white;
				else if (t < showFor) hitchFrame = Colour::grey;
			}
			{
				if (Performance::System::is_saving())
				{
					fpiStoring = Colour::c64Orange;
				}
				else if (Performance::System::is_saving_or_just_saved())
				{
					fpiStored = Colour::c64Yellow;
				}
			}
			{
				if (is_world_manager_busy())
				{
					worldJobManagerBusy = Colour::c64Green;
				}
			}
			{
				float t = hitchFrameIndicators.renderThread.get_time_since();
				if (t <= timeNow) renderThread = Colour::red;
				else if (t < showFor) renderThread = Colour::grey;
			}
			{
				float t = hitchFrameIndicators.renderEndToPreEnd.get_time_since();
				if (t <= timeNow) renderEndToPreEnd = Colour::c64Blue;
				else if (t < showFor) renderEndToPreEnd = Colour::grey;
			}
			{
				float t = hitchFrameIndicators.renderPreEndToEnd.get_time_since();
				if (t <= timeNow) renderPreEndToEnd = Colour::orange;
				else if (t < showFor) renderPreEndToEnd = Colour::grey;
			}
			{
				float t = hitchFrameIndicators.gameLoop.get_time_since();
				if (t <= timeNow) gameLoop = Colour::blue;
				else if (t < showFor) gameLoop = Colour::grey;
			}
		}

		{
			SUSPEND_STORE_DIRECT_GL_CALL();
			SUSPEND_STORE_DRAW_CALL();

			if (Framework::Font* font = get_default_font())
			{
				set_ignore_retro_rendering_for(font, true);
				for (int eyeIdx = 0; eyeIdx < (does_use_vr() ? 2 : 1); ++eyeIdx)
				{
					System::Video3D* v3d = System::Video3D::get();
					bool forVR = false;
					Vector2 rtSize = v3d->get_screen_size().to_vector2();
					Vector2 startAt;
					startAt.x = 0.0f;
					startAt.y = (float)v3d->get_screen_size().y;
					VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
					System::RenderTarget* rt = nullptr;// get_main_render_output_render_buffer();
					if (does_use_vr() && VR::IVR::get()->get_output_render_target(eye))
					{
						forVR = true;
						rt = VR::IVR::get()->get_output_render_target(eye);
					}
					if (rt)
					{
						rt->bind();
						rtSize = rt->get_size().to_vector2();
					}
					else
					{
						System::RenderTarget::bind_none();
					}
					if (forVR)
					{
#ifdef AN_VIVE_ONLY
						startAt.x = rtSize.x * 0.55f;
						startAt.y = rtSize.y * 0.55f;
#else
						startAt.x = rtSize.x * 0.60f;
						startAt.y = rtSize.y * 0.60f;
#endif
						startAt += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
					}
					v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
					v3d->setup_for_2d_display();
					v3d->set_2d_projection_matrix_ortho(rtSize);
					v3d->access_model_view_matrix_stack().clear();

					Vector2 at = startAt;
					Vector2 fontSize;
					fontSize.x = rtSize.y * get_font_size_rel_to_rt_size();
					fontSize.y = fontSize.x;
					Vector2 fontSizeMultiplier;
					fontSizeMultiplier.x = fontSize.x / 8.0f;
					fontSizeMultiplier.y = fontSizeMultiplier.x;

					if (forVR)
					{
						//at.x -= fontSize.y * 2.0f;
						//at.y -= fontSize.y * 2.0f;
					}
					else
					{
						at.x = (float)v3d->get_viewport_size().x * 0.7f;
						at.y = (float)v3d->get_viewport_size().y * 0.8f;
					}

					at.y += fontSize.y;
					{
						font->draw_text_at_with_border(v3d, String(TXT("frame rate info")), Colour::greyLight, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					at.y -= fontSize.y;

					struct MinMaxValue
					{
						System::TimeStamp minValue_ts;
						System::TimeStamp maxValue_ts;
						Optional<float> minValue;
						Optional<float> maxValue;

						void update(float _value, float const timeShow = 1.0f)
						{
							if (minValue_ts.get_time_since() > timeShow)
							{
								minValue.clear();
							}
							if (maxValue_ts.get_time_since() > timeShow)
							{
								maxValue.clear();
							}
							if (!minValue.is_set() || minValue.get() >= _value)
							{
								minValue_ts.reset();
								minValue = _value;
							}
							if (!maxValue.is_set() || maxValue.get() <= _value)
							{
								maxValue_ts.reset();
								maxValue = _value;
							}
						}
					};

					struct GetColour
					{
						static Colour get_against_frame_sharp(float _time, float _ptOfFrame)
						{
							float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
							expectedFrameTime *= _ptOfFrame; // this is the budget
							if (_time >= expectedFrameTime * 0.95f)
							{
								return Colour::red;
							}
							return Colour::greenDark;
						}
						static Colour get_against_frame(float _time, float _ptOfFrame, float _addFirst = 0.0f)
						{
							float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
							expectedFrameTime = (expectedFrameTime + _addFirst) * _ptOfFrame; // this is the budget
							if (_time > expectedFrameTime * 0.95f)
							{
								return Colour::red;
							}
							if (_time > expectedFrameTime * 0.87f)
							{
								return Colour::red;
							}
							if (_time > expectedFrameTime * 0.79f)
							{
								return Colour::orange;
							}
							if (_time > expectedFrameTime * 0.71f)
							{
								return Colour::yellow;
							}
							return Colour::greenDark;
						}
						static Colour get_game_sync(float _time)
						{
							float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
							if (_time > expectedFrameTime)
							{
								return Colour::red;
							}
							if (_time < expectedFrameTime * 0.1f)
							{
								return Colour::red;
							}
							if (_time < expectedFrameTime * 0.2f)
							{
								return Colour::orange;
							}
							if (_time < expectedFrameTime * 0.3f)
							{
								return Colour::yellow;
							}
							return Colour::greenDark;
						}
					};
#ifndef SHORT_HITCH_FRAME_INDICATOR
					{
						// without game sync as it fills it
						float time = prevFrameInfo->preThreads + prevFrameInfo->threadSoundGameLoop + prevFrameInfo->postThreads - prevFrameInfo->gameSync;
						static MinMaxValue mm;
						mm.update(time);
						font->draw_text_at_with_border(v3d, String::printf(TXT("main : %4.1fms %4.1fms"), time * 1000.0f, mm.maxValue.get(0.0f) * 1000.0f), GetColour::get_against_frame_sharp(max(mm.maxValue.get(0.0f), time), 0.95f), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
					}
					{
						float time = prevFrameInfo->gameLoopTime;
						static MinMaxValue mm;
						mm.update(time);
						font->draw_text_at_with_border(v3d, String::printf(TXT("gl   : %4.1fms %4.1fms"), time * 1000.0f, mm.maxValue.get(0.0f) * 1000.0f), GetColour::get_against_frame(max(mm.maxValue.get(0.0f), time), 0.95f, -0.003f), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
					}
					{
						float time = prevFrameInfo->renderTime_part1 + prevFrameInfo->renderTime_part4; // prepare + actual render
						static MinMaxValue mm;
						mm.update(time);
						font->draw_text_at_with_border(v3d, String::printf(TXT("pr+rn: %4.1fms %4.1fms"), time * 1000.0f, mm.maxValue.get(0.0f) * 1000.0f), GetColour::get_against_frame(max(mm.maxValue.get(0.0f), time), 0.95f, -0.003f), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
					}
					{
						float gameSync = prevFrameInfo->gameSync;
						static MinMaxValue mm;
						mm.update(gameSync);
						font->draw_text_at_with_border(v3d, String::printf(TXT("gsync: %4.1fms"), gameSync * 1000.0f), GetColour::get_game_sync(gameSync), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("  min: %4.1fms"), mm.minValue.get(0.0f) * 1000.0f), GetColour::get_game_sync(mm.minValue.get(0.0f)).mul_rgb(0.8f), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("  max: %4.1fms"), mm.maxValue.get(0.0f) * 1000.0f), GetColour::get_game_sync(mm.maxValue.get(0.0f)).mul_rgb(0.8f), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
					}
					{
						Vector2 tat = at;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%i "), hitchFrameCount), Colour::grey, Colour::blackWarm, 1.0f, tat, fontSizeMultiplier, Vector2(1.0f, 0.0f));
						tat.y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%i "), gameLoopHitchFrameCount), Colour::grey, Colour::blackWarm, 1.0f, tat, fontSizeMultiplier, Vector2(1.0f, 0.0f));
						tat.y -= fontSize.y;
						float fps = 0.0f;
						{
							float expectedFrameTime = System::Video3D::get()->get_ideal_expected_frame_time();
							if (expectedFrameTime != 0.0f)
							{
								float timeTotal = ::System::Core::get_time_since_core_started();
								int fpsFull = TypeConversions::Normal::f_i_closest(1.0f / expectedFrameTime);
								float framesTotal = (float)fpsFull * timeTotal;
								float framesWithoutHitches = framesTotal - (float)hitchFrameCount;
								fps = framesWithoutHitches / timeTotal;
							}
						}
						font->draw_text_at_with_border(v3d, String::printf(TXT("%.2f "), fps), Colour::grey, Colour::blackWarm, 1.0f, tat, fontSizeMultiplier, Vector2(1.0f, 0.0f));
					}
#endif
					if (hitchFrame.is_set())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("hitch frame")), hitchFrame.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					else
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("ok")), Colour::grey, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					at.y -= fontSize.y;
#ifndef SHORT_HITCH_FRAME_INDICATOR
					if (auto* vr = VR::IVR::get())
					{
						float rs = vr->get_render_scaling();
						font->draw_text_at_with_border(v3d, String::printf(TXT("r-s: %5.1f%%"), rs * 100.0f), Colour::grey, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					at.y -= fontSize.y;
#endif
					if (fpiStoring.is_set())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("FPI storing")), fpiStoring.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					else if (fpiStored.is_set())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("FPI stored")), fpiStored.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					at.y -= fontSize.y;
					if (worldJobManagerBusy.is_set())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("woma busy")), worldJobManagerBusy.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
					}
					at.y -= fontSize.y;
#ifndef SHORT_HITCH_FRAME_INDICATOR
					if (!does_use_vr() || !VR::IVR::get()->is_game_sync_blocking())
					{
						if (renderThread.is_set())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("+ render")), renderThread.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						}
						at.y -= fontSize.y;
						if (renderEndToPreEnd.is_set())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("  + CPU")), renderEndToPreEnd.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						}
						at.y -= fontSize.y;
						if (renderPreEndToEnd.is_set())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("  + GPU")), renderPreEndToEnd.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						}
						at.y -= fontSize.y;
						if (gameLoop.is_set())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("+ game loop (CPU)")), gameLoop.get(), Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						}
						at.y -= fontSize.y;
					}

#ifdef AN_LINUX_OR_ANDROID
					// thread info
					if (auto* vr = VR::IVR::get())
					{
						LogInfoContext log;
						vr->log_perf_threads(log);
						at = startAt;
						at.x -= fontSize.x * 20.0f;
						at.y -= fontSize.y * 16.0f;
						for_every(l, log.get_lines())
						{
							font->draw_text_at_with_border(v3d, l->text, Colour::greyLight, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
							at.y -= fontSize.y;
						}
					}
#endif
#endif
					System::RenderTarget::unbind_to_none();
				}
				set_ignore_retro_rendering_for(font, false);
			}
			
			RESTORE_STORE_DIRECT_GL_CALL();
			RESTORE_STORE_DRAW_CALL();
		}
	}
#endif
#endif

#ifdef ADDITIONAL_DEBUG
	{
		SUSPEND_STORE_DIRECT_GL_CALL();
		SUSPEND_STORE_DRAW_CALL();

		if (Framework::Font* font = get_default_font())
		{
			set_ignore_retro_rendering_for(font, true);
			for (int eyeIdx = 0; eyeIdx < (does_use_vr() ? 2 : 1); ++eyeIdx)
			{
				System::Video3D* v3d = System::Video3D::get();
				bool forVR = false;
				Vector2 rtSize = v3d->get_screen_size().to_vector2();
				Vector2 at;
				at.x = (float)v3d->get_screen_size().x * 0.5f;
				at.y = (float)v3d->get_screen_size().y * 0.5f;
				VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
				System::RenderTarget* rt = nullptr;// get_main_render_output_render_buffer();
				if (does_use_vr() && VR::IVR::get()->get_output_render_target(eye))
				{
					forVR = true;
					rt = VR::IVR::get()->get_output_render_target(eye);
				}
				if (rt)
				{
					rt->bind();
					rtSize = rt->get_size().to_vector2();
				}
				else
				{
					System::RenderTarget::bind_none();
				}
				if (forVR)
				{
					at.x = rtSize.x * 0.5f;
					at.y = rtSize.y * 0.5f;
					at += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
				}
				v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
				v3d->setup_for_2d_display();
				v3d->set_2d_projection_matrix_ortho(rtSize);
				v3d->access_model_view_matrix_stack().clear();

				Vector2 fontSize;
				fontSize.x = rtSize.y * get_font_size_rel_to_rt_size();
				fontSize.y = fontSize.x;
				Vector2 fontSizeMultiplier;
				fontSizeMultiplier.x = fontSize.x / 8.0f;
				fontSizeMultiplier.y = fontSizeMultiplier.x;

				/*
				if (forVR)
				{
					//at.x -= fontSize.y * 2.0f;
					//at.y -= fontSize.y * 2.0f;
				}
				else
				{
					at.x = (float)v3d->get_viewport_size().x * 0.7f;
					at.y = (float)v3d->get_viewport_size().y * 0.8f;
				}
				*/

#ifdef ADDITIONAL_DEBUG__HAND_TRACKING
				Vector2 startAt = at;

				font->draw_text_at_with_border(v3d, String(TXT("hand tracking")), Colour::greyLight, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
				at.y -= fontSize.y;

				if (auto* vr = VR::IVR::get())
				{
					at.x -= fontSize.y * 8.0f;
					auto& refPoseSet = vr->get_reference_pose_set();
					auto& poseSet = vr->get_controls_pose_set();
					for_count(int, handIdx, Hand::MAX)
					{
						auto& hand = poseSet.hands[handIdx];
						{
							auto& refHand = refPoseSet.hands[handIdx];
							float handSize = rtSize.x * 0.3f;
							Vector2 at = startAt + Vector2::xAxis * ((handIdx == Hand::Left? - 1.0f : 1.0f) * handSize * 0.7f);
							Transform handTransform = Transform::identity;// (Vector3::zero, Rotator3(0.0f, handIdx == Hand::Left ? -90.0f : 90.0f, 0.0f).to_quat());
							for_count(int, b, VR::VRHandPose::MAX_RAW_BONES)
							{
								int p = refHand.rawBoneParents[b];
								if (p != NONE)
								{
									for_count(int, poseIdx, 2)
									{
										auto& useHand = poseIdx == 0? refHand : hand;
										Colour useColour = poseIdx == 0? Colour::blue : Colour::green;

										auto& aRS = useHand.rawBonesRS[b];
										auto& bRS = useHand.rawBonesRS[p];
										if (aRS.is_set() && bRS.is_set())
										{
											Vector3 a = handTransform.location_to_world(aRS.get().get_translation());
											Vector3 b = handTransform.location_to_world(bRS.get().get_translation());
											Vector2 a2d = at + a.to_vector2() * handSize / 0.20f;
											Vector2 b2d = at + b.to_vector2() * handSize / 0.20f;
											::System::Video3DPrimitives::line_2d(useColour, a2d, b2d, false);
										}
									}
								}
							}
						}

						for_count(int, fingerIdx, VR::VRFinger::MAX)
						{
							auto& finger = hand.fingers[fingerIdx];
							float controlsFinger = 0.0f;
							float pinched = 0.0f;
							auto& handControls = vr->get_controls().hands[handIdx];
							if (fingerIdx == VR::VRFinger::Thumb) { controlsFinger = handControls.thumb.get(-1.0f); pinched = -1.0f; }
							if (fingerIdx == VR::VRFinger::Pointer) { controlsFinger = handControls.pointer.get(-1.0f); pinched = handControls.pointerPinch.get(-1.0f); }
							if (fingerIdx == VR::VRFinger::Middle) { controlsFinger = handControls.middle.get(-1.0f); pinched = handControls.middlePinch.get(-1.0f); }
							if (fingerIdx == VR::VRFinger::Ring) { controlsFinger = handControls.ring.get(-1.0f); pinched = handControls.ringPinch.get(-1.0f); }
							if (fingerIdx == VR::VRFinger::Pinky) { controlsFinger = handControls.pinky.get(-1.0f); pinched = handControls.pinkyPinch.get(-1.0f); }
							font->draw_text_at_with_border(v3d, String::printf(TXT("%i:%i %.3f %.3f -> %.3f p%.3f"), handIdx, fingerIdx, finger.straightLength, finger.straightRefAligned, controlsFinger, pinched), Colour::greyLight, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
							at.y -= fontSize.y;
						}
					}
#endif
#ifdef ADDITIONAL_DEBUG__LAST_FOVEATED_RENDERING_SETUP
				font->draw_text_at_with_border(v3d, String(TXT("last foveated rendering setup")), Colour::greyLight, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
				at.y -= fontSize.y;

				at.x -= fontSize.y * 10.0f;
				{
					LogInfoContext log;
#ifdef TWEAK_AND_TEST_FOVEATED_RENDERING_SETUPS
					::System::RenderTargetUtils::log_last_foveated_rendering_setups(log);
#else
					log.log(TXT("not available"));
#endif
					for_every(l, log.get_lines())
					{
						font->draw_text_at_with_border(v3d, l->text, Colour::greyLight, Colour::blackWarm, 1.0f, at, fontSizeMultiplier);
						at.y -= fontSize.y;
					}
				}
#endif
				System::RenderTarget::unbind_to_none();
			}
			set_ignore_retro_rendering_for(font, false);
		}

		RESTORE_STORE_DIRECT_GL_CALL();
		RESTORE_STORE_DRAW_CALL();
	}
#endif

	bool allowTempRender = false;
#ifdef AN_DEVELOPMENT_OR_PROFILER
	allowTempRender = true;
#else
	allowTempRender = debugDrawVRControls != 0;
#endif
#ifndef AN_DEVELOPMENT_OR_PROFILER
	if (!allowTempRender)
	{
		return;
	}
#endif
#ifdef DUMP
	return;
#endif
#if AN_WINDOWS
	if (!IsDebuggerPresent() && !allowTempRender)
	{
		return;
	}
#endif

#ifdef AN_PERFORMANCE_MEASURE
	if (showPerformance)
	{
		return;
	}
#endif

	MEASURE_PERFORMANCE(render_tempUI);

	SUSPEND_STORE_DIRECT_GL_CALL();
	SUSPEND_STORE_DRAW_CALL();

#ifdef AN_DEVELOPMENT_OR_PROFILER
	System::Video3D * v3d = System::Video3D::get();
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::N5))
	{
		debugDrawVRControls = (debugDrawVRControls + 1) % 3;
	}
#endif

	int showDebugInfo = debugRenderingSuspended ? 0 : debugPanel->get_option(NAME(dboShowDebugInfo));
	int showVRLogPanel = debugRenderingSuspended ? 0 : debugPanel->get_option(NAME(dboShowVRLogPanel));

	if (debugLogPanel)
	{
		// in case we miss something
		debugLogPanel->show_log(nullptr);
	}

	if (showVRLogPanel)
	{
		if (!showLogPanelVR)
		{
			if (debugLogPanel)
			{
				debugLogPanel->show_log(&debugLog);
			}
			change_show_log_panel(debugLogPanel);
		}
	}
	else
	{
		if (debugLogPanel)
		{
			debugLogPanel->show_log(nullptr);
		}
		change_show_log_panel(nullptr);
	}
	if (debugLogPanel)
	{
		debugLogPanel->lock(showVRLogPanel == 2 && !showDebugPanelVR);
		debugLogPanel->lock_dragging(showVRLogPanel == 2 && !showDebugPanelVR);
	}

	if (!showDebugPanel)
	if (showDebugInfo)
	{
		if (Framework::Font* font = get_default_font())
		{
			set_ignore_retro_rendering_for(font, true);
			for (int eyeIdx = 0; eyeIdx < (does_use_vr() ? 2 : 1); ++eyeIdx)
			{
				{
					System::Video3D* v3d = System::Video3D::get();
					bool forVR = false;
					Vector2 rtSize = v3d->get_screen_size().to_vector2();
					Vector2 at;
					at.x = (float)v3d->get_screen_size().x * 0.5f;
					at.y = (float)v3d->get_screen_size().y * 0.5f;
					VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
					System::RenderTarget* rt = nullptr;// get_main_render_output_render_buffer();
					if (does_use_vr() && VR::IVR::get()->get_output_render_target(eye))
					{
						forVR = true;
						rt = VR::IVR::get()->get_output_render_target(eye);
					}
					if (rt)
					{
						rt->bind();
						rtSize = rt->get_size().to_vector2();
					}
					else
					{
						System::RenderTarget::bind_none();
					}
					if (forVR)
					{
						at.x = rtSize.x * 0.5f;
						at.y = rtSize.y * 0.5f;
						at += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
					}
					v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
					v3d->setup_for_2d_display();
					v3d->set_2d_projection_matrix_ortho(rtSize);
					v3d->access_model_view_matrix_stack().clear();
				}

				Vector2 rtSize = v3d->get_screen_size().to_vector2();

				Vector2 at = Vector2::zero;
				at.x = (float)v3d->get_viewport_size().x;
				at.y = (float)v3d->get_viewport_size().y;

				Vector2 fontSize;
				fontSize.x = rtSize.y * get_font_size_rel_to_rt_size();
				fontSize.y = fontSize.x;
				Vector2 fontSizeMultiplier;
				fontSizeMultiplier.x = fontSize.x / 8.0f;
				fontSizeMultiplier.y = fontSizeMultiplier.x;

				font->draw_text_at_with_border(v3d, String::printf(TXT("game#: %i"), get_frame_no()), Colour::whiteWarm, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
				at.y -= fontSize.y;
#ifndef BUILD_PUBLIC_RELEASE
				font->draw_text_at_with_border(v3d, String::printf(TXT("core#: %i"), lastGameFrameNo), Colour::zxYellowBright, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
				at.y -= fontSize.y;
#endif

#ifdef AN_USE_FRAME_INFO
				if (showDebugInfo <= 3)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("draw call: %6i"), prevFrameInfo->systemInfo.get_draw_calls()), Colour::whiteWarm, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("draw tris: %6i"), prevFrameInfo->systemInfo.get_draw_triangles()), Colour::whiteWarm, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("rt draw call: %6i"), prevFrameInfo->systemInfo.get_draw_calls_render_targets()), Colour::whiteWarm, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("rt other call: %6i"), prevFrameInfo->systemInfo.get_other_calls_render_targets()), Colour::whiteWarm, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("d/gl call: %6i"), prevFrameInfo->systemInfo.get_direct_gl_calls()), Colour::whiteWarm, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
#ifdef AN_DEVELOPMENT_OR_PROFILER
					font->draw_text_at_with_border(v3d, String::printf(TXT("dev-vol: %6.0f"), MainConfig::global().get_sound().developmentVolume * 100.0f), MainConfig::global().get_sound().developmentVolume < 1.0f? Colour::cyan : Colour::white, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
#endif
#ifdef AN_SOUND_LOUDNESS_METER
					{
						Optional<float> loudness;
						Optional<float> maxRecentLoudness;
						if (auto* s = System::Sound::get())
						{
							loudness = s->get_loudness();
							maxRecentLoudness = s->get_recent_max_loudness();
						}
						if (loudness.is_set())
						{
							Colour colour = Colour::grey;
							colour = Sound::LoudnessMeter::loudness_to_colour(loudness.get()).get(colour);
							font->draw_text_at_with_border(v3d, String::printf(TXT("loudness: %4.0fdB"), loudness.get()), colour, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
							at.y -= fontSize.y;
						}
						if (maxRecentLoudness.is_set())
						{
							Colour colour = Colour::grey;
							colour = Sound::LoudnessMeter::loudness_to_colour(maxRecentLoudness.get()).get(colour);
							font->draw_text_at_with_border(v3d, String::printf(TXT("max recent l: %4.0fdB"), maxRecentLoudness.get()), colour, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
							at.y -= fontSize.y;
						}
					}
#endif
#ifdef AN_CHECK_MEMORY_LEAKS
					{
						size_t memT = get_memory_allocated_total();
						size_t memC = get_memory_allocated_checked_in();

						font->draw_text_at_with_border(v3d, String::printf(TXT("memory:%5iMB"), memC >> 20), Colour::white, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
						at.y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("memtot:%5iMB"), memT >> 20), Colour::white, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
						at.y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("memwst:%6.2f%%"), 100.0f * (1.0f - (float)memC / (float)memT)), Colour::white, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
						at.y -= fontSize.y;
					}
#endif
#ifdef DEBUG_GPU_MEMORY
					{
						size_t memC, memA;
						gpu_memory_info_get(memC, memA);

						font->draw_text_at_with_border(v3d, String::printf(TXT("gpu memory:%5iMB"), memC >> 20), Colour::white, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
						at.y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("gpu avlble:%5iMB"), memA >> 20), Colour::white, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
						at.y -= fontSize.y;
					}
#endif
				}
#endif

#ifdef AN_PERFORMANCE_MEASURE
				if (allowToStopToShowPerformanceOnAnyHitch)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("stop <90fps")), Colour::green, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
				}
				if (allowToStopToShowPerformanceOnHighHitch)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("stop HIGH")), Colour::green, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
				}
				if (stopToShowPerformanceEachFrame)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("stop each frame")), Colour::red, Colour::blackWarm, 1.0f, at, fontSizeMultiplier, Vector2(1.0f, 1.0f));
					at.y -= fontSize.y;
				}
#endif

			}
			set_ignore_retro_rendering_for(font, false);
		}
	}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	float maxLogLineY = (float)v3d->get_viewport_size().y - 20.0f;
#endif

	::System::RenderTarget::unbind_to_none();

	// frames
	if (!showDebugPanel)
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (showDebugInfo)
#else
	if (debugDrawVRControls)
#endif
	{
		if (Framework::Font* font = get_default_font())
		{
			set_ignore_retro_rendering_for(font, true);
			for (int eyeIdx = 0; eyeIdx < (does_use_vr() ? 2 : 1); ++eyeIdx)
			{
				System::Video3D* v3d = System::Video3D::get();
				bool forVR = false;
				Vector2 rtSize = v3d->get_screen_size().to_vector2();
				Vector2 at;
				at.x = 0.0f;
				at.y = (float)v3d->get_screen_size().y;
				VR::Eye::Type eye = (VR::Eye::Type)eyeIdx;
				System::RenderTarget* rt = nullptr;// get_main_render_output_render_buffer();
				if (does_use_vr() && VR::IVR::get()->get_output_render_target(eye))
				{
					forVR = true;
					rt = VR::IVR::get()->get_output_render_target(eye);
				}
				if (rt)
				{
					rt->bind();
					rtSize = rt->get_size().to_vector2();
				}
				else
				{
					System::RenderTarget::bind_none();
				}
				if (forVR)
				{
					at.x = rtSize.x * 0.6f;
					at.y = rtSize.y * 0.6f;
					at += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
				}
				v3d->set_viewport(VectorInt2::zero, rtSize.to_vector_int2());
				v3d->setup_for_2d_display();
				v3d->set_2d_projection_matrix_ortho(rtSize);
				v3d->access_model_view_matrix_stack().clear();

				Vector2 fontSize;
				fontSize.x = rtSize.y * get_font_size_rel_to_rt_size();
				fontSize.y = fontSize.x;
				Vector2 fontSizeMultiplier;
				fontSizeMultiplier.x = fontSize.x / 8.0f;
				fontSizeMultiplier.y = fontSizeMultiplier.x;

				if (forVR)
				{
					//at.x -= fontSize.y * 2.0f;
					//at.y -= fontSize.y * 2.0f;
				}
				else
				{
					at.x = 0.0f;
					at.y = (float)v3d->get_viewport_size().y;
				}

				if (!showDebugPanel)
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (showDebugInfo <= 3)
#endif
				{
#ifdef AN_ALLOW_BULLSHOTS
					if (Framework::BullshotSystem::is_active())
					{
						Vector2 textAt = v3d->get_viewport_size().to_vector2() * 0.5f;
						textAt.y = textAt.y * 0.3f;
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f * 0.4f;
						scale.y = scale.x * 0.8f;
						font->draw_text_at_with_border(v3d, String::printf(TXT("BULLSHOT MODE [%i]"), bullshotTrapIdx), Colour::zxCyanBright, Colour::blackWarm, 1.0f, textAt, scale, Vector2(0.5f, 1.0f));
					}
#endif
#ifdef AN_ALLOW_AUTO_TEST
					if (is_auto_test_active())
					{
						Vector2 at = v3d->get_viewport_size().to_vector2() * Vector2(0.5f, 0.25f);
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f * 0.5f;
						scale.y = scale.x * 0.8f;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%i:%04.0f"), autoTestIdx, max(0.0f, autoTestTimeLeft)), is_on_short_pause()? Colour::zxGreenBright : Colour::zxRedBright, Colour::blackWarm, 1.0f, at, scale, Vector2(0.5f, 1.0f));
					}
#endif
					if (System::Core::is_paused())
					{
						Vector2 pausedAt = v3d->get_viewport_size().to_vector2() * 0.5f;
						pausedAt.y = pausedAt.y * 0.4f + at.y * 0.6f;
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f;
						scale.y = scale.x * 0.8f;
						font->draw_text_at_with_border(v3d, String(TXT("SYSTEM PAUSE")), Colour::zxRedBright, Colour::blackWarm, 1.0f, pausedAt, scale, Vector2(0.5f, 1.0f));
					}
					else if (System::Core::is_vr_paused())
					{
						Vector2 pausedAt = v3d->get_viewport_size().to_vector2() * 0.5f;
						pausedAt.y = pausedAt.y * 0.4f + at.y * 0.6f;
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f;
						scale.y = scale.x * 0.8f;
						font->draw_text_at_with_border(v3d, String(TXT("VR PAUSE")), Colour::zxRedBright, Colour::blackWarm, 1.0f, pausedAt, scale, Vector2(0.5f, 1.0f));
					}
#ifdef AN_DEVELOPMENT_OR_PROFILER
					else if (System::Core::get_time_pace() != 1.0f)
					{
						Vector2 pausedAt = v3d->get_viewport_size().to_vector2() * 0.5f;
						pausedAt.y = pausedAt.y * 0.4f + v3d->get_viewport_size().to_vector2().y * 0.6f;
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f * 0.35f;
						scale.y = scale.x * 0.8f;
						font->draw_text_at_with_border(v3d, String::printf(TXT("x %.3f"), System::Core::get_time_pace()), Colour::zxRed, Colour::blackWarm, 1.0f, pausedAt, scale, Vector2(0.5f, 1.0f));
					}
#endif
#ifdef USE_RENDER_DOC
					{
						Vector2 at = v3d->get_viewport_size().to_vector2() * 0.5f;
						at.y = at.y * 0.4f;
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f * 0.35f;
						scale.y = scale.x * 0.8f;
						scale *= 0.5f;
						font->draw_text_at_with_border(v3d, String(TXT("no debug renderer while using render doc")), Colour::red, Colour::blackWarm, 1.0f, at, scale, Vector2(0.5f, 1.0f));
					}
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (autopilot)
					{
						Vector2 at = v3d->get_viewport_size().to_vector2() * Vector2(0.5f, 0.25f);
						Vector2 scale = v3d->get_viewport_size().to_vector2() * 0.01f * 0.5f;
						scale.y = scale.x * 0.8f;
						font->draw_text_at_with_border(v3d, String(TXT("autopilot")), Colour::zxGreen, Colour::blackWarm, 1.0f, at, scale, Vector2(0.5f, 1.0f));
					}
					if (debugShowMiniMap && ! does_want_to_cancel_creation())
					{
						if (auto* ow = PilgrimageInstanceOpenWorld::get())
						{
							Range2 mapAt;
							{
								float mapSize;
								Vector2 mapCentre;
								if (forVR)
								{
									mapSize = min(rtSize.x, rtSize.y) * 0.15f;
									mapCentre.x = rtSize.x * 0.37f;
									mapCentre.y = rtSize.y * 0.5f;
									mapCentre += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
								}
								else
								{
									mapSize = min(rtSize.x, rtSize.y) * 0.3f;
									mapCentre.x = rtSize.x - mapSize * 0.6f;
									mapCentre.y = mapSize * 0.6f;
								}
								mapCentre = round(mapCentre);
								mapAt = Range2(mapCentre);
								mapAt.expand_by(Vector2::one * round(mapSize * 0.5f));
							}
							ow->debug_render_mini_map(player.get_actor(), mapAt, 0.5f, 4, NP, debugHighLevelRegion.outputInfo ? debugHighLevelRegion.highLevelRegion : nullptr);
						}
					}
#endif

				}

#ifdef AN_DEVELOPMENT_OR_PROFILER
				float x = at.x;
				float y = at.y;

#ifdef SOUND_MASTERING
				if (false) // just skip this and render log
#endif
				if (!showDebugPanel)
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (showDebugInfo <= 2)
#endif
				{
#ifdef FPS_INFO
#ifdef FPS_INFO_SHOW_SIMPLE
					y -= fontSize.y;
					Colour expectedFPSColour = Colour::c64LightGreen;
					{
						if (fpsLastSecond < get_ideal_expected_frame_per_second() * 0.7f)
						{
							expectedFPSColour = Colour::c64Red;
						}
						else if (fpsLastSecond < get_ideal_expected_frame_per_second() * 0.9f)
						{
							expectedFPSColour = Colour::c64Orange;
						}
					}
					if (forVR)
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.0f"), get_ideal_expected_frame_per_second()), expectedFPSColour, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.0f"), fpsLastSecond), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						if (auto* vr = VR::IVR::get())
						{
							if (vr->get_render_scaling() < 1.0f)
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("[%2.0f]"), clamp(vr->get_render_scaling() * 100.0f, 0.0f, 99.0f)), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x + fontSize.x * 4.0f, y), fontSizeMultiplier);
							}
							else if (MainConfig::global().get_video().allowAutoAdjustForVR)
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("[ok]")), Colour::c64LightGreen, Colour::blackWarm, 1.0f, Vector2(x + fontSize.x * 4.0f, y), fontSizeMultiplier);
							}
							else
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("[--]")), Colour::c64Grey2, Colour::blackWarm, 1.0f, Vector2(x + fontSize.x * 4.0f, y), fontSizeMultiplier);
							}
						}
					}
					else
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.0f"), get_ideal_expected_frame_per_second()), expectedFPSColour, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.1f"), fpsLastSecond), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.1f -> %3.1f"), fpsRange.min, fpsRange.max), Colour::zxCyanBright, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.1f -> %3.1f (5s)"), fpsRange5show.min, fpsRange5show.max), Colour::zxYellowBright, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("%3.1f (avg)"), fpsTotalFrames / max(0.1f, fpsTotalTime)), Colour::zxRedBright, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);

						y -= fontSize.y; // empty line, to not run into other texts
					}
#else
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("last second : %5.1f"), fpsLastSecond), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("averaged    : %5.1f"), fpsAveraged), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("immediate   : %5.1f"), fpsImmediate), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("frame time  : %5.3f ms"), fpsFrameTime * 1000.0f), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
#endif
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
					if (debugShowGameJobs)
					{
						y = min(y, at.y - fontSize.y * 10.0f);
						{	// world manager
							y -= fontSize.y;
							String syncWJ = worldManager.get_current_sync_world_job_info();
							String asyncWJ = worldManager.get_current_async_world_job_info();
							font->draw_text_at_with_border(v3d, String::printf(TXT("woma: %S"),
								worldManager.has_something_to_do()? (!syncWJ.is_empty() ? syncWJ.to_char() : asyncWJ.to_char()) : TXT("[idle]")),
								worldManager.has_something_to_do()? (!syncWJ.is_empty() ? Colour::orange : Colour::green) : Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							y -= fontSize.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT(" qj#: %i"), worldManager.get_number_of_world_jobs()),
								worldManager.get_number_of_world_jobs() > 0? Colour::greyLight : Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							y -= fontSize.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT(" fic: %S"), is_forced_instant_object_creation()? TXT("FORCE") : TXT("no")),
								is_forced_instant_object_creation()? Colour::c64Red : Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						}
						{	// delayed object creations
							y -= fontSize.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT("doc#: %i"), delayedObjectCreations.get_size()),
								delayedObjectCreations.get_size() > 0 ? Colour::greyLight : Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						}
						{	// library
							y -= fontSize.y;
							Framework::LibraryState::Type libraryState = Library::get_current()->get_state();
							font->draw_text_at_with_border(v3d, String::printf(TXT("libr: %S"), libraryState == Framework::LibraryState::Idle? TXT("idle") : TXT("busy")),
								libraryState == Framework::LibraryState::Idle ? Colour::grey : Colour::orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						}
						if (auto* navSys = get_nav_system())
						{	// nav sys info
							y -= fontSize.y;
							if (navSys->get_pending_build_tasks_count())
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("nav : [%S %i]"),
									navSys->is_performing_task_or_queued() ? TXT("building") : TXT("build required"), navSys->get_pending_build_tasks_count()),
									navSys->is_performing_task_or_queued() ? Colour::orange : Colour::yellow,
									Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							}
							else
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("nav : [%S]"),
									navSys->is_performing_task_or_queued() ? TXT("busy") : TXT("idle")),
									navSys->is_performing_task_or_queued() ? Colour::green : Colour::grey,
									Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							}
						}
						{	// objects to advance
							y -= fontSize.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT("o2ad: %4i (%4i + %4i)")
								, get_world()? get_world()->get_objects_to_advanced_last_frame_count() : 0
								, get_world()? get_world()->get_objects_to_advance_count() : 0
								, get_world()? get_world()->get_objects_to_advance_once_count() : 0
								),
								get_world()? Colour::white : Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						}
						{
							y -= fontSize.y;
							if (removeUnusedTemporaryObjectsFromLibrary)
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("trem: removing (%i [%i])"), removedUnusedTemporaryObjectsFromLibrary, removeUnusedTemporaryObjectsFromLibraryBatchSize), Colour::red, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							}
							else if (removeUnusedTemporaryObjectsFromLibraryRequested)
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("trem: requested")), Colour::orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							}
							else
							{
								font->draw_text_at_with_border(v3d, String::printf(TXT("trem: %.1fs (rec:%i [%i])"),
									autoRemoveUnusedTemporaryObjectsFromLibraryInterval.is_set()? autoRemoveUnusedTemporaryObjectsFromLibraryInterval.get() - timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary : timeSinceLastRemoveUnusedTemporaryObjectsFromLibrary,
									removedUnusedTemporaryObjectsFromLibrary, removeUnusedTemporaryObjectsFromLibraryBatchSize), Colour::greyLight, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							}
						}
					}
#endif
#ifdef AN_CHECK_MEMORY_LEAKS
					y -= fontSize.y;
					{
						size_t memT = get_memory_allocated_total();
						size_t memC = get_memory_allocated_checked_in();
						font->draw_text_at_with_border(v3d, String::printf(TXT("memA:%5iMB"), memC >> 20), Colour::greyLight, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("    +%.2f%%"), 100.0f * ((float)memT / (float)memC - 1.0f)), Colour::greyLight, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("memT:%5iMB"), memT >> 20), Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					}
#endif
#ifdef AN_MEMORY_STATS
					if (System::SystemInfo::get())
					{
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("mem#: %06i"), System::SystemInfo::get()->get_allocated()), Colour::white, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					}
#endif
#ifdef DEBUG_GPU_MEMORY
					{
						size_t memC, memA;
						gpu_memory_info_get(memC, memA);

						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("gpuM:%5iMB"), memC >> 20), Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("gpuA:%5iMB"), memA >> 20), Colour::greyLight, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					}
#endif
#ifdef AN_RENDERER_STATS
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("d-c :%3i"), prevFrameInfo->systemInfo.get_draw_calls()), Colour::cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					{
						Colour colour = Colour::lerp(0.5f, Colour::white, Colour::cyan);
						int dt = prevFrameInfo->systemInfo.get_draw_triangles();
						if (::System::Core::get_system_tags().get_tag(NAME(lowGraphics)))
						{
							if (dt < 40000) colour = Colour::lerp(0.5f, Colour::white, Colour::green); else
							if (dt < 60000) colour = Colour::lerp(0.6f, Colour::white, Colour::yellow); else
							if (dt < 70000) colour = Colour::lerp(0.7f, Colour::white, Colour::orange); else
							if (dt < 80000) colour = Colour::lerp(0.8f, Colour::white, Colour::red); else
											colour = Colour::lerp(0.8f, Colour::purple, Colour::red);
						}
						font->draw_text_at_with_border(v3d, String::printf(TXT("d-t :%6i"), dt), colour, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					}
					Framework::Render::Stats const & stats = Framework::Render::Stats::get();
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("r-r :%3i"), stats.renderedRoomProxies.get()), Colour::cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("r-p :%3i"), stats.renderedPresenceLinkProxies.get()), Colour::cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("r-d :%3i"), stats.renderedDoorInRoomProxies.get()), Colour::cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, String::printf(TXT("p>r :%3i"), lastInRoomPresenceLinksCount), Colour::yellow, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					lastInRoomPresenceLinksCountRequired = true;
#endif

					// world manager full list
#ifdef WORLD_MANAGER_JOB_LIST
					if (debugShowGameJobs)
					{
						Array<tchar const*> worldJobNames;
						worldManager.get_async_world_job_infos(worldJobNames, 10);
						y -= fontSize.y;
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String(TXT("woma list:")), !worldJobNames.is_empty()? Colour::greyLight : Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						for_every_ptr(wj, worldJobNames)
						{
							y -= fontSize.y;
							font->draw_text_at_with_border(v3d, String::printf(TXT("    : %S"), wj), Colour::greyLight, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						}
					}
#endif
				}
#endif

				if (debugDrawVRControls == 1 && VR::IVR::get())
				{
					for_count(int, i, 2)
					{
						Vector2 at(rtSize.x * (0.43f + 0.14f * float(i)), rtSize.y * 0.5f);
						at += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
						float width = rtSize.x * 0.2f;
						at.x += (i == 0 ? -width : width) * 0.4f;
						at.y += rtSize.y * 0.2f;
						if (VR::IVR::get())
						{
							int h = i == 0 ? VR::IVR::get()->get_left_hand() : VR::IVR::get()->get_right_hand();
							VR::IVR::get()->draw_debug_controls(h, at, width, font->get_font());
						}
					}
				}
				if (debugDrawVRControls == 2 && VR::IVR::get())
				{
					for_count(int, i, 2)
					{
						Vector2 at(rtSize.x * (0.43f + 0.14f * float(i)), rtSize.y * 0.5f);
						at += VR::IVR::get()->get_render_info().lensCentreOffset[eyeIdx] * rtSize * 0.5f;
						float radius = min(rtSize.x * 0.05f, rtSize.y * 0.1f);
						::System::Video3DPrimitives::fill_circle_2d(Colour::blue.with_alpha(0.2f), at, radius);
						if (VR::IVR::get())
						{
							int h = i == 0 ? VR::IVR::get()->get_left_hand() : VR::IVR::get()->get_right_hand();
							Vector2 trackpad = VR::IVR::get()->get_controls().hands[h].get_stick(VR::Input::Stick::Trackpad);
							::System::Video3DPrimitives::fill_circle_2d(Colour::green.with_alpha(0.6f), at + radius * trackpad, radius * 0.1f);
							Vector2 trackpadDir = VR::IVR::get()->get_controls().hands[h].get_stick(VR::Input::Stick::TrackpadDir);
							::System::Video3DPrimitives::fill_circle_2d(Colour::yellow.with_alpha(0.6f), at + radius * trackpadDir, radius * 0.1f);
						}
					}
				}

#ifdef AN_DEVELOPMENT_OR_PROFILER
				maxLogLineY = min(maxLogLineY, y);
#endif
				System::RenderTarget::unbind_to_none();;
			}
			set_ignore_retro_rendering_for(font, false);
		}
	}

	// ----

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_DEVELOPMENT
	if (!_justInfo && showDebugInfo == 1)
#else
#ifdef AN_PROFILER
	if (!_justInfo && showDebugInfo)
#endif
#endif
	{
		if (Framework::Font* font = get_default_font())
		{
			set_ignore_retro_rendering_for(font, true);
			System::Video3D* v3d = System::Video3D::get();
			Vector2 rtSize = v3d->get_screen_size().to_vector2();
			System::RenderTarget::bind_none();
			v3d->set_default_viewport();
			v3d->setup_for_2d_display();
			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();
			float x = 0.0f;
			float y = (float)v3d->get_viewport_size().y;

			Vector2 fontSize;
			fontSize.x = rtSize.y * get_font_size_rel_to_rt_size();
			fontSize.y = fontSize.x;
			Vector2 fontSizeMultiplier;
			fontSizeMultiplier.x = fontSize.x / 8.0f;
			fontSizeMultiplier.y = fontSizeMultiplier.x;

			x = 100.0f * fontSizeMultiplier.x;
			y -= fontSize.y;
			{
				String hitInfo;
				if (debugSelectDebugSubject || debugQuickSelectDebugSubject)
				{
					hitInfo = TXT("SELECTING DEBUG SUBJECT : ");
				}
				else if (debugSelectDebugSubjectHit.is_set())
				{
					hitInfo = TXT("DEBUG SUBJECT : ");
				}
				else
				{
					hitInfo = TXT("NO DEBUG SUBJECT");
				}
				Colour colour = Colour::whiteWarm;
				{
					auto* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get());
					if (imo && imo->is_advancement_suspended())
					{
						hitInfo += TXT("SUSPENDED ");
						colour = Colour::lerp(0.5f, Colour::red, Colour::whiteWarm);
					}
				}
				if (auto * obj = fast_cast<Framework::Object>(debugSelectDebugSubjectHit.get()))
				{
					hitInfo += obj->get_name();
				}
				else if (debugSelectDebugSubjectHit.is_set())
				{
					hitInfo += TXT("hit something");
				}
				font->draw_text_at_with_border(v3d, hitInfo, colour, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
				y -= fontSize.y;
#ifdef AN_DEVELOPMENT_OR_PROFILER
				if (auto* obj = fast_cast<Framework::IAIObject>(debugSelectDebugSubjectHit.get()))
				{
					font->draw_text_at_with_border(v3d, String(TXT("> ")) + obj->ai_get_additional_info(), colour.mul_rgb(0.8f), Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
#endif
				if (auto* obj = fast_cast<Framework::Object>(debugSelectDebugSubjectHit.get()))
				{
					font->draw_text_at_with_border(v3d, String(TXT("tags ")) + obj->get_tags().to_string().to_char(), colour.mul_rgb(0.8f), Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
			}
			if (auto* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
			{
				if (auto* inRoom = imo->get_presence()->get_in_room())
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("  in %S"), inRoom->get_name().to_char()), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					if (inRoom->get_room_type())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("      %S"), inRoom->get_room_type()->get_name().to_string().to_char()), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
					}
					font->draw_text_at_with_border(v3d, String::printf(TXT("@ %S %S"), imo->get_presence()->get_placement().get_translation().to_string().to_char(), imo->get_presence()->get_placement().get_orientation().to_rotator().to_string().to_char()), Colour::whiteWarm, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
			}
			if (!showDebugPanel)
			{
				if (auto* actor = player.get_actor())
				{
					if (auto* inRoom = actor->get_presence()->get_in_room())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("act in %S"), inRoom->get_name().to_char()), Colour::c64Yellow, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("      %S"), inRoom->get_tags().to_string().to_char()), Colour::c64Yellow, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						if (inRoom->get_room_type())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("      %S"), inRoom->get_room_type()->get_name().to_string().to_char()), Colour::c64Yellow, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							y -= fontSize.y;
						}
						if (inRoom->get_environment())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("    e %i %S [%S]")
								, inRoom->get_environment()
								, inRoom->get_environment()? inRoom->get_environment()->get_name().to_string().to_char() : TXT("--")
								, inRoom->get_environment() && inRoom->get_environment()->get_environment_type()? inRoom->get_environment()->get_environment_type()->get_name().to_string().to_char() : TXT("--")
								), Colour::c64Yellow, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							y -= fontSize.y;
						}
						font->draw_text_at_with_border(v3d, String::printf(TXT("@ %S %S"), actor->get_presence()->get_placement().get_translation().to_string().to_char(), actor->get_presence()->get_placement().get_orientation().to_rotator().to_string().to_char()), Colour::c64Yellow, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
					}
				}
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
				if (onlyExternalRenderScene)
				{
					font->draw_text_at_with_border(v3d, String(TXT("only external camera")), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				else if (useExternalRenderScene)
				{
					font->draw_text_at_with_border(v3d, String(TXT("with external camera")), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
#endif
				if (debugCameraMode)
				{
					if (auto* inRoom = debugCameraInRoom.get())
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("cam in %S"), inRoom->get_name().to_char()), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("      %S"), inRoom->get_tags().to_string().to_char()), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						if (inRoom->get_room_type())
						{
							font->draw_text_at_with_border(v3d, String::printf(TXT("      %S"), inRoom->get_room_type()->get_name().to_string().to_char()), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
							y -= fontSize.y;
						}
						font->draw_text_at_with_border(v3d, String::printf(TXT("@ %S %S"), debugCameraPlacement.get_translation().to_string().to_char(), debugCameraPlacement.get_orientation().to_rotator().to_string().to_char()), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
	#ifdef AN_ALLOW_BULLSHOTS
						Transform eaPlacement = Transform::identity;
						DEFINE_STATIC_NAME_STR(environmentAnchor, TXT("environment anchor"));
						eaPlacement = inRoom->get_variables().get_value<Transform>(NAME(environmentAnchor), eaPlacement);
						font->draw_text_at_with_border(v3d, String::printf(TXT("@ea %S"), eaPlacement.to_local(debugCameraPlacement).get_translation().to_string().to_char()), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
						font->draw_text_at_with_border(v3d, String::printf(TXT("   %S"), eaPlacement.to_local(debugCameraPlacement).get_orientation().to_rotator().to_string().to_char()), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
	#endif
					}
				}
				if (does_debug_control_player())
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("control: player")), Colour::c64Green, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugControlsMode == 1 && debugCameraMode)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("control: camera")), Colour::c64LightBlue, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugCameraMode == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("camera: debug")), Colour::c64Cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugCameraMode == 2)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("camera: debug (through render offset)")), Colour::c64Orange, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawCollision == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("collisions (objects only, skip player)")), Colour::red, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawCollision == 2)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("collisions (objects only)")), Colour::red, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawCollision == 3)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("collisions")), Colour::red, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawCollision)
				{
					if (debugDrawCollisionMode == 1)
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("+- movement")), Colour::red, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
					}
					if (debugDrawCollisionMode == 2)
					{
						font->draw_text_at_with_border(v3d, String::printf(TXT("+- precise")), Colour::cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
						y -= fontSize.y;
					}
				}
				if (debugTestCollisions == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("test collisions (movement)")), Colour::red, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugTestCollisions == 2)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("test collisions (precise)")), Colour::cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugTestCollisions)
				{
					String hitInfo(TXT("+- hit"));
					String objFlagsInfo;
					String physMatInfo;
					String physMatFlagsInfo;
					if (auto * obj = fast_cast<Framework::Object>(debugTestCollisionsHit.get()))
					{
						hitInfo += TXT(" ");
						hitInfo += obj->get_name();
					}
					else if (debugTestCollisionsHit.get())
					{
						hitInfo += TXT(" ");
						hitInfo += TXT("hit something");
					}
					if (auto* imo = fast_cast<Framework::IModulesOwner>(debugTestCollisionsHit.get()))
					{
						if (auto* mc = imo->get_collision())
						{
							objFlagsInfo = mc->get_collision_flags().to_string();
						}
					}
					if (debugTestCollisionsHitModel && debugTestCollisionsHitShape)
					{
						if (auto * model = debugTestCollisionsHitModel)
						{
							for_every(el, model->get_boxes())
							{
								if (el == debugTestCollisionsHitShape)
								{
									hitInfo += String::printf(TXT(", box %i"), for_everys_index(el));
								}
							}
							for_every(el, model->get_capsules())
							{
								if (el == debugTestCollisionsHitShape)
								{
									hitInfo += String::printf(TXT(", capsule %i"), for_everys_index(el));
								}
							}
							for_every(el, model->get_spheres())
							{
								if (el == debugTestCollisionsHitShape)
								{
									hitInfo += String::printf(TXT(", sphere %i"), for_everys_index(el));
								}
							}
							for_every(el, model->get_hulls())
							{
								if (el == debugTestCollisionsHitShape)
								{
									hitInfo += String::printf(TXT(", hull %i"), for_everys_index(el));
								}
							}
							for_every(el, model->get_meshes())
							{
								if (el == debugTestCollisionsHitShape)
								{
									hitInfo += String::printf(TXT(", mesh %i"), for_everys_index(el));
								}
							}
						}
					}
					font->draw_text_at_with_border(v3d, hitInfo, Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					Collision::PhysicalMaterial const * physMat = nullptr;
					Colour physMatColour = Colour::grey;
					if (!physMat && debugTestCollisionsHitShape)
					{
						physMat = debugTestCollisionsHitShape->get_material();
					}
					if (!physMat && debugTestCollisionsHitModel)
					{
						physMat = debugTestCollisionsHitModel->get_material();
					}
					if (physMat)
					{
						physMatColour = Colour::greyLight;
					}
					if (!physMat)
					{
						physMatColour = Colour::lerp(0.5f, Colour::red, physMatColour);
						physMat = Framework::PhysicalMaterial::get();
					}
					if (physMat)
					{
						physMatFlagsInfo = physMat->get_collision_flags().to_string();
					}
					if (Framework::PhysicalMaterial const * pm = fast_cast<Framework::PhysicalMaterial>(physMat))
					{
						physMatInfo = String::printf(TXT("  phys mat : %S "), pm->get_name().to_string().to_char());
					}
					else
					{
						physMatInfo = String(TXT("  phys mat : <no material>"));
					}
					objFlagsInfo = String(TXT("  object cf: ")) + objFlagsInfo;
					physMatFlagsInfo = String(TXT("  phys mat cf: ")) + physMatFlagsInfo;
					font->draw_text_at_with_border(v3d, objFlagsInfo, Colour::grey, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, physMatInfo, physMatColour, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
					font->draw_text_at_with_border(v3d, physMatFlagsInfo, physMatColour, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawPresence == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("presence")), Colour::c64Blue, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawPresenceBaseInfo == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("presence base info")), Colour::c64Blue, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawRGI == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("play area (rgi)")), Colour::c64LightBlue, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawRGI == 2)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("room generation tiles zone")), Colour::c64LightBlue, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawVRAnchor == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("vr anchor")), Colour::c64LightBlue, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawVRPlayArea == 1)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("used play area (vr)")), Colour::c64Cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugDrawVRPlayArea == 2)
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("play area setup (vr)")), Colour::c64Cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
				if (debugPanel->get_option(NAME(dboDrawNavMesh)))
				{
					font->draw_text_at_with_border(v3d, String::printf(TXT("nav mesh")), Colour::c64Cyan, Colour::blackWarm, 1.0f, Vector2(x, y), fontSizeMultiplier);
					y -= fontSize.y;
				}
			}

			maxLogLineY = min(maxLogLineY, y);
			set_ignore_retro_rendering_for(font, false);
		}
	}
#endif
#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (!showDebugPanel)
	if (!_justInfo && (showDebugInfo >= 1 || showVRLogPanel))
	{
		if (Framework::Font* font = get_default_font())
		{
			set_ignore_retro_rendering_for(font, true);
			System::Video3D* v3d = System::Video3D::get();
			Vector2 rtSize = v3d->get_screen_size().to_vector2();
			System::RenderTarget::bind_none();
			v3d->set_default_viewport();
			v3d->setup_for_2d_display();
			v3d->set_2d_projection_matrix_ortho();
			v3d->access_model_view_matrix_stack().clear();
			
			Vector2 fontSize;
			fontSize.x = rtSize.y * get_font_size_rel_to_rt_size();
			fontSize.y = fontSize.x;
			Vector2 fontSizeMultiplier;
			fontSizeMultiplier.x = fontSize.x / 8.0f;
			fontSizeMultiplier.y = fontSizeMultiplier.x;

			{
				float x = 0.0f;
				float y = (float)v3d->get_viewport_size().y - 100.0f * fontSizeMultiplier.y;
				// huge output stuff
				static int aiLogBackwards = 0;
				if (debugAILog)
				{
					if (showDebugInfo >= 1)
					{
						Colour defaultColour = Colour::lerp(0.7f, Colour::greyLight, Colour::white);
						Colour currentOutline = Colour::black;

						Vector2 scale = fontSizeMultiplier;
						float linesSeparation = font->get_height() * scale.y;
#ifdef AN_USE_AI_LOG
						int linesToShow = (int)((float)v3d->get_viewport_size().y / linesSeparation) + 1;
#endif
						float aiY = 0.0f;

#ifdef AN_STANDARD_INPUT
						{
							int amountToScroll = 4;
							if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ||
								::System::Input::get()->is_key_pressed(::System::Key::RightShift))
							{
								if (::System::Input::get()->is_key_pressed(::System::Key::LeftAlt) ||
									::System::Input::get()->is_key_pressed(::System::Key::RightAlt))
								{
									amountToScroll = 50;
								}
								else
								{
									amountToScroll = 10;
								}
							}
							if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
								::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
							{
								amountToScroll = 1;
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelDown))
							{
								aiLogBackwards -= amountToScroll;
							}
							if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelUp))
							{
								aiLogBackwards += amountToScroll;
							}
						}
#endif

						aiLogBackwards = max(aiLogBackwards, 0);

						tchar const* problem = nullptr;
#ifdef AN_USE_AI_LOG
						if (auto * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (imo->get_ai() &&
								imo->get_ai()->get_mind() &&
								imo->get_ai()->get_mind()->get_logic())
							{
								auto const& lines = imo->get_ai()->get_mind()->get_logic()->access_log().get_lines();
								System::Video3DPrimitives::fill_rect_2d(Colour::blue.with_alpha(0.15f), Vector2::zero, v3d->get_viewport_size().to_vector2(), true);
								aiLogBackwards = clamp(aiLogBackwards, 0, max(0, lines.get_size() - 1));
								// show backwards
								int atLine = 0;
								for_every_reverse(line, lines)
								{
									if (atLine >= aiLogBackwards &&
										atLine <= aiLogBackwards + linesToShow)
									{
										font->draw_text_at_with_border(v3d, line->text, line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, aiY), scale);
										aiY += linesSeparation;
									}
									++atLine;
								}
							}
							else
							{
								problem = TXT("<non ai debug subject or no logic available>");
							}
						}
						else
						{
							problem = TXT("<debug subject not selected>");
						}
#endif
						if (problem)
						{
							font->draw_text_at_with_border(v3d, String(problem), defaultColour, currentOutline, 1.0f, Vector2(x, aiY), scale);
							aiY += linesSeparation;
						}
					}
					if (showLogPanelVR)
					{
						if (auto * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
#ifdef AN_USE_AI_LOG
							if (imo->get_ai() &&
								imo->get_ai()->get_mind() &&
								imo->get_ai()->get_mind()->get_logic())
							{
								if (debugLogPanel && showVRLogPanel)
								{
									debugLogPanel->show_log_title(imo->ai_get_name().to_char());
									debugLogPanel->show_log(&(imo->get_ai()->get_mind()->get_logic()->access_log()), true);
								}
							}
#endif
						}
					}
				}
				else
				{
					LogInfoContext & log = debugLog;
					log.clear();
					Colour headerColour = Colour::yellow;
					Colour subHeaderColour = Colour::orange;

					Framework::Room* debugRoom = nullptr;
					{
						if (auto* actor = player.get_actor())
						{
							if (auto* inRoom = actor->get_presence()->get_in_room())
							{
								debugRoom = inRoom;
							}
						}
						if (auto* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto* inRoom = imo->get_presence()->get_in_room())
							{
								debugRoom = inRoom;
							}
						}
						if (debugCameraMode)
						{
							if (auto* inRoom = debugCameraInRoom.get())
							{
								debugRoom = inRoom;
							}
						}
					}

					int mouseWheel = 0;
#ifdef AN_STANDARD_INPUT
					bool ctrlHold = System::Input::get()->is_key_pressed(System::Key::LeftCtrl) ||
									System::Input::get()->is_key_pressed(System::Key::RightCtrl);
					{
						int amountToScroll = 4;
						if (::System::Input::get()->is_key_pressed(::System::Key::LeftShift) ||
							::System::Input::get()->is_key_pressed(::System::Key::RightShift))
						{
							amountToScroll = 10;
						}
						if (::System::Input::get()->is_key_pressed(::System::Key::LeftCtrl) ||
							::System::Input::get()->is_key_pressed(::System::Key::RightCtrl))
						{
							amountToScroll = 1;
						}
						if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelDown))
						{
							mouseWheel += amountToScroll;
						}
						if (::System::Input::get()->has_key_been_pressed(::System::Key::MouseWheelUp))
						{
							mouseWheel -= amountToScroll;
						}
					}
#else
					bool ctrlHold = false;
#endif

					if (!performanceLog.get_lines().is_empty())
					{
						log.set_colour(headerColour);
						log.log(TXT("PERFORMANCE"));
						log.set_colour();
						{
							LOG_INDENT(log);
							log.log(performanceLog);
						}
						log.log(TXT(""));
					}

					if (debugLogNavTasks)
					{
						log.set_colour(headerColour);
						log.log(TXT("NAV TASKS"));
						log.set_colour();
						LOG_INDENT(log);
						{
							if (auto* ns = get_nav_system())
							{
								ns->log_tasks(log);
							}
						}
					}

					if (debugInspectMovement)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							log.set_colour(headerColour);
							log.log(TXT("MOVEMENT"));
							log.set_colour();
							LOG_INDENT(log);
							if (auto* m = imo->get_movement())
							{
								log.log(TXT("name: %S"), m->get_name().to_char());
								if (auto* p = imo->get_presence())
								{
									log.log(TXT("in room: %S"), p->get_in_room()? p->get_in_room()->get_name().to_char() : TXT("--"));
									log.log(TXT("velocity: %S"), p->get_velocity_linear().to_string().to_char());
									log.log(TXT("speed: %.3f"), p->get_velocity_linear().length());
									log.log(TXT("rotation: %S"), p->get_velocity_rotation().to_string().to_char());
								}
							}
							if (auto* c = imo->get_controller())
							{
								{
									auto& mp = c->get_requested_movement_parameters();
									log.log(TXT("requested movement parameters"));
									{
										LOG_INDENT(log);
										log.log(TXT("gait: %S"), mp.gaitName.to_char());
										if (mp.absoluteSpeed.is_set())
										{
											log.log(TXT("speed: absolute %.3f"), mp.absoluteSpeed.get());
										}
										else if (mp.relativeSpeed.is_set())
										{
											log.log(TXT("speed: relative %.3f"), mp.relativeSpeed.get());
										}
										else
										{
											log.log(TXT("speed: none"));
										}
									}
								}
								struct LogUtils
								{
									LogInfoContext& lic;
									LogUtils(LogInfoContext& _log) : lic(_log) {}
									void log(Optional<float> const& _v, tchar const* _text)
									{
										if (_v.is_set())
										{
											lic.log(TXT("%S: %.3f"), _text, _v.get());
										}
									}
									void log(Optional<Vector3> const& _v, tchar const* _text)
									{
										if (_v.is_set())
										{
											lic.log(TXT("%S: x:%.3f y:%.3f z:%.3f"), _text, _v.get().x, _v.get().y, _v.get().z);
										}
									}
									void log(Optional<Rotator3> const& _v, tchar const* _text)
									{
										if (_v.is_set())
										{
											lic.log(TXT("%S: p:%.3f y:%.3f w:%.3f"), _text, _v.get().pitch, _v.get().yaw, _v.get().roll);
										}
									}
									void log(Optional<Quat> const& _v, tchar const* _text)
									{
										if (_v.is_set())
										{
											log(_v.get().to_rotator(), _text);
										}
									}
									void log(Optional<bool> const& _v, tchar const* _text)
									{
										if (_v.is_set())
										{
											lic.log(TXT("%S: %S"), _text, _v.get()? TXT("++true") : TXT("--false"));
										}
									}
								} logUtils(log);
								logUtils.log(c->get_distance_to_stop(), TXT("distance to stop"));
								logUtils.log(c->get_distance_to_slow_down(), TXT("distance to slow down"));
								logUtils.log(c->get_slow_down_percentage(), TXT("slow down percentage"));
								logUtils.log(c->get_requested_movement_direction(), TXT("req movement direction"));
								logUtils.log(c->get_relative_requested_movement_direction(), TXT("rel req movement direction"));
								logUtils.log(c->get_requested_velocity_linear(), TXT("req velocity linear"));
								logUtils.log(c->get_requested_orientation(), TXT("req orientation"));
								logUtils.log(c->get_requested_velocity_orientation(), TXT("req velocity orientation"));
								logUtils.log(c->get_requested_relative_velocity_orientation(), TXT("req rel velocity orientation"));
								logUtils.log(c->get_follow_yaw_to_look_orientation(), TXT("follow yaw to look orientation"));
							}
							log.log(TXT(""));
						}
					}

					if (debugAILogic)
					{
						if (Framework::IModulesOwner const* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto* ai = imo->get_ai())
							{
								if (auto* mind = ai->get_mind())
								{
									if (auto* logic = fast_cast<Framework::AI::Logic>(mind->get_logic()))
									{
										logic->debug_draw(debugRoom);
									}
								}
							}
						}
					}

					if (debugDrawCollisionGradient)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							log.set_colour(headerColour);
							log.log(TXT("COLLISIONS"));
							log.set_colour();
							LOG_INDENT(log);
							if (auto* c = imo->get_collision())
							{
								int idx = 0;
								for_every(c, c->get_collided_with())
								{
									log.log(TXT("%i"), idx);
									LOG_INDENT(log);
									if (auto* r = c->collidedInRoom.get())
									{
										log.log(TXT("in room: %S"), r->get_name().to_char());
									}
									if (Framework::IModulesOwner const * cimo = fast_cast<Framework::IModulesOwner>(c->collidedWithObject.get()))
									{
										log.log(TXT("imo: %S"), cimo->ai_get_name().to_char());
									}
									else if (Framework::Room const * r = fast_cast<Framework::Room>(c->collidedWithObject.get()))
									{
										log.log(TXT("room: %S"), r->get_name().to_char());
									}
									if (c->collisionLocation.is_set())
									{
										log.log(TXT("lol: %S"), c->collisionLocation.get().to_string().to_char());
									}
									if (c->collisionNormal.is_set())
									{
										log.log(TXT("lol: %S"), c->collisionNormal.get().to_string().to_char());
									}
									++idx;
								}
							}
							log.log(TXT(""));
						}
					}

					if (debugDrawPresence)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							log.set_colour(headerColour);
							log.log(TXT("PRESENCE"));
							log.set_colour();
							LOG_INDENT(log);
							if (auto* p = imo->get_presence())
							{
								p->log(log);
								log.log(TXT("in rooms"));
								{
									LOG_INDENT(log);
									auto* link = p->get_presence_links();
									while (link)
									{
										if (auto* r = link->get_in_room())
										{
											log.log(TXT("%S"), r->get_name().to_char());
										}
										link = link->get_next_in_object();
									}
								}
							}
							log.log(TXT(""));
						}
					}

					if (debugDrawPresenceBaseInfo)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							log.set_colour(headerColour);
							log.log(TXT("PRESENCE BASE INFO"));
							log.set_colour();
							LOG_INDENT(log);
							if (auto* p = imo->get_presence())
							{
								p->log_base_info(log);
							}
							log.log(TXT(""));
						}
					}

					if (debugTestCollisions)
					{
						log.set_colour(headerColour);
						log.log(TXT("TEST COLLISIONS"));
						log.set_colour();
						LOG_INDENT(log);
						Collision::PhysicalMaterial const * physMat = nullptr;
						Colour physMatColour = Colour::grey;
						if (!physMat && debugTestCollisionsHitShape)
						{
							physMat = debugTestCollisionsHitShape->get_material();
						}
						if (!physMat && debugTestCollisionsHitModel)
						{
							physMat = debugTestCollisionsHitModel->get_material();
						}
						if (physMat)
						{
							physMatColour = Colour::greyLight;
						}
						if (!physMat)
						{
							physMatColour = Colour::lerp(0.5f, Colour::red, physMatColour);
							physMat = Framework::PhysicalMaterial::get();
						}
						if (debugTestCollisionsHit.get())
						{
							log.log(TXT("col obj cf : %S"), debugTestCollisionsHit.get()->get_collision_flags().to_string().to_char());
						}
						else
						{
							log.log(TXT("col obj cf : <no object>"));
						}
						if (fast_cast<Framework::IModulesOwner>(debugTestCollisionsHit.get()) &&
							fast_cast<Framework::IModulesOwner>(debugTestCollisionsHit.get())->get_collision())
						{
							auto * col = fast_cast<Framework::IModulesOwner>(debugTestCollisionsHit.get())->get_collision();
							log.log(TXT("col wit cf : %S"), col->get_collides_with_flags().to_string().to_char());
						}
						else
						{
							log.log(TXT("col wit cf : <no object>"));
						}
						if (Framework::PhysicalMaterial const * pm = fast_cast<Framework::PhysicalMaterial>(physMat))
						{
							log.log(TXT("phys mat : %S "), pm->get_name().to_string().to_char());
						}
						else
						{
							log.log(TXT("phys mat : <no material>"));
						}
						if (physMat)
						{
							log.log(TXT("phys mat cf : %S"), physMat->get_collision_flags().to_string().to_char());
						}
						if (auto * obj = fast_cast<Framework::Object>(debugTestCollisionsHit.get()))
						{
							log.log(TXT("hit obj: %S"), obj->get_name().to_char());
						}
						else if (auto * dir = fast_cast<Framework::DoorInRoom>(debugTestCollisionsHit.get()))
						{
							log.log(TXT("hit door: %S"), dir->ai_get_name().to_char());
						}
						else if (debugTestCollisionsHit.get())
						{
							log.log(TXT("hit something"));
						}
						{
							LOG_INDENT(log);
							if (auto* imo = fast_cast<Framework::IModulesOwner>(debugTestCollisionsHit.get()))
							{
								if (auto* mc = imo->get_collision())
								{
									log.log(TXT("collision module collision flags"));
									LOG_INDENT(log);
									log.log(mc->get_collision_flags().to_string().to_char());
								}
							}
						}
						if (debugTestCollisionsHitModel && debugTestCollisionsHitShape)
						{
							if (auto * model = debugTestCollisionsHitModel)
							{
								for_every(el, model->get_boxes())
								{
									if (el == debugTestCollisionsHitShape)
									{
										log.log(TXT("hit collision shape: box %i"), for_everys_index(el));
										LOG_INDENT(log);
										el->log(log);
									}
								}
								for_every(el, model->get_capsules())
								{
									if (el == debugTestCollisionsHitShape)
									{
										log.log(TXT("hit collision shape: capsule %i"), for_everys_index(el));
										LOG_INDENT(log);
										el->log(log);
									}
								}
								for_every(el, model->get_spheres())
								{
									if (el == debugTestCollisionsHitShape)
									{
										log.log(TXT("hit collision shape: sphere %i"), for_everys_index(el));
										LOG_INDENT(log);
										el->log(log);
									}
								}
								for_every(el, model->get_hulls())
								{
									if (el == debugTestCollisionsHitShape)
									{
										log.log(TXT("hit collision shape: hull %i"), for_everys_index(el));
										LOG_INDENT(log);
										el->log(log);
									}
								}
								for_every(el, model->get_meshes())
								{
									if (el == debugTestCollisionsHitShape)
									{
										log.log(TXT("hit collision shape: mesh %i"), for_everys_index(el));
										LOG_INDENT(log);
										el->log(log);
									}
								}
							}
						}
						log.log(TXT(""));
					}

					if (debugLogRoomVariables)
					{
						if (debugRoom)
						{
							log.set_colour(headerColour);
							log.log(TXT("ROOM VARIABLES"));
							log.set_colour();
							{
								LOG_INDENT(log);
								{
									log.set_colour(subHeaderColour);
									log.log(TXT("== this room \"%S\""), debugRoom->get_name().to_char());
									log.set_colour();
									{
										LOG_INDENT(log);
										{
											log.log(TXT("== vars"));
											LOG_INDENT(log);
											debugRoom->get_variables().log(log);
										}
										if (auto* rt = debugRoom->get_room_type())
										{
											log.log(TXT("== room type vars"));
											LOG_INDENT(log);
											rt->get_custom_parameters().log(log);
										}
									}
								}
								if (debugRoom->get_origin_room() != debugRoom)
								{
									log.set_colour(subHeaderColour);
									log.log(TXT("== origin room \"%S\""), debugRoom->get_origin_room()->get_name().to_char());
									log.set_colour();
									{
										LOG_INDENT(log);
										{
											log.log(TXT("== vars"));
											LOG_INDENT(log);
											debugRoom->get_origin_room()->get_variables().log(log);
										}
										if (auto* rt = debugRoom->get_origin_room()->get_room_type())
										{
											log.log(TXT("== room type vars"));
											LOG_INDENT(log);
											rt->get_custom_parameters().log(log);
										}
									}
								}
								auto* r = debugRoom->get_in_region();
								while (r)
								{
									log.set_colour(subHeaderColour);
									log.log(TXT("== region \"%S\""), r->get_region_type()? r->get_region_type()->get_name().to_string().to_char() : TXT("??"));
									log.set_colour();
									{
										LOG_INDENT(log);
										{
											log.log(TXT("== vars"));
											LOG_INDENT(log);
											r->get_variables().log(log);
										}
										if (auto* rt = r->get_region_type())
										{
											log.log(TXT("== region type vars"));
											LOG_INDENT(log);
											rt->get_custom_parameters().log(log);
										}
									}
									r = r->get_in_region();
								}
							}
						}
					}

					if (debugRoomInfo)
					{
						if (debugRoom)
						{
							log.set_colour(headerColour);
							log.log(TXT("ROOM INFO"));
							log.set_colour();
							{
								LOG_INDENT(log);
								log.log(TXT("r%p"), debugRoom);
								log.log(TXT("name: %S"), debugRoom->get_name().to_char());
								log.log(TXT("type: %S"), debugRoom->get_room_type()? debugRoom->get_room_type()->get_name().to_string().to_char() : TXT("--"));
								log.log(TXT("tags: %S"), debugRoom->get_tags().to_string(true).to_char());
								log.log(TXT("tags (all): %S"), debugRoom->get_tags().to_string_all(true).to_char());
								log.log(TXT("door vr corridor priority: %S"), debugRoom->get_door_vr_corridor_priority_range().to_string().to_char());
								log.log(TXT("door type replacer: %S"), debugRoom->get_door_type_replacer()? debugRoom->get_door_type_replacer()->get_name().to_string().to_char() : TXT("--"));
#ifdef AN_DEVELOPMENT_OR_PROFILER
								log.log(TXT("rgi %S"), debugRoom->get_generated_with_room_generator()? debugRoom->get_generated_with_room_generator()->get_name().to_char() : TXT("--"));
#endif
								if (debugRoom->get_vr_elevator_altitude().is_set())
								{
									log.log(TXT("vr elevator altitude : %.3f"), debugRoom->get_vr_elevator_altitude().get());
								}
								else
								{
									log.log(TXT("no vr elevator altitude provided"));
								}
								{
									LOG_INDENT(log);
									for_every_ptr(d, debugRoom->get_all_doors())
									{
										if (d->get_vr_elevator_altitude().is_set())
										{
											log.log(TXT("+- dir : %.3f"), d->get_vr_elevator_altitude().get());
										}
										if (d->get_door()->get_vr_elevator_altitude().is_set())
										{
											log.log(TXT("+- door : %.3f"), d->get_door()->get_vr_elevator_altitude().get());
										}
									}
								}
								if (debugRoom->get_origin_room() && debugRoom->get_origin_room() != debugRoom)
								{
									auto* originRoom = debugRoom->get_origin_room();
									log.log(TXT("origin room"));
									LOG_INDENT(log);
									log.log(TXT("r%p"), originRoom);
									log.log(TXT("name: %S"), originRoom->get_name().to_char());
									log.log(TXT("type: %S"), originRoom->get_room_type() ? originRoom->get_room_type()->get_name().to_string().to_char() : TXT("--"));
									log.log(TXT("tags: %S"), originRoom->get_tags().to_string(true).to_char());
									log.log(TXT("tags (all): %S"), originRoom->get_tags().to_string_all(true).to_char());
								}
								else
								{
									log.log(TXT("no origin room"));
								}
								{
									Optional<Vector3> gravityDir;
									Optional<float> killGravityDistance;
									debugRoom->update_kill_gravity_distance(REF_ gravityDir, REF_ killGravityDistance);
									if (gravityDir.is_set() && killGravityDistance.is_set())
									{
										log.log(TXT("kill gravity"));
										LOG_INDENT(log);
										log.log(TXT("dir:  %S"), gravityDir.get().to_string().to_char());
										log.log(TXT("dist: %.3f"), killGravityDistance.get());
										log.log(TXT(" =at: %S"), (gravityDir.get() * killGravityDistance.get()).to_string().to_char());
									}
									else
									{
										log.log(TXT("no kill gravity"));
									}
								}
								{
									log.log(TXT("environment"));
									LOG_INDENT(log);
									if (auto* e = debugRoom->get_environment())
									{
										log.log(TXT("environment \"%S\""), e->get_environment_type()? e->get_environment_type()->get_name().to_string().to_char() : TXT("--"));
									}
									for_every(eo, debugRoom->get_environment_overlays())
									{
										log.log(TXT("+ overlay \"%S\" [%.3f]"), eo->get_name().to_string().to_char(), eo->get()? eo->get()->calculate_power(debugRoom) : 0.0f);
									}
									auto* reg = debugRoom->get_in_region();
									while (reg)
									{
										for_every(eo, reg->get_environment_overlays())
										{
											log.log(TXT("+ (reg) overlay \"%S\" [%.3f]"), eo->get_name().to_string().to_char(), eo->get() ? eo->get()->calculate_power(debugRoom) : 0.0f);
										}
										reg = reg->get_in_region();
									}
								}
							}
						}
					}
					
					if (debugDrawPOIs == 3)
					{
						if (debugRoom)
						{
							log.set_colour(headerColour);
							log.log(TXT("POIs IN ROOM"));
							log.set_colour();
							{
								LOG_INDENT(log);
								log.log(TXT("r%p"), debugRoom);
								debugRoom->for_every_point_of_interest(NP,
									[&log](Framework::PointOfInterestInstance* _fpoi)
									{
										log.log(TXT("name \"%S\""), _fpoi->get_name().to_char());
										LOG_INDENT(log);
										log.log(TXT("tags \"%S\""), _fpoi->get_tags().to_string_all().to_char());
									});
							}
						}
					}

					if (debugDrawDoors)
					{
						if (debugRoom)
						{
							log.set_colour(headerColour);
							log.log(TXT("DOORS"));
							log.set_colour();
							{
								LOG_INDENT(log);
								log.log(TXT("%S [%S]"), debugRoom->get_name().to_char(), debugRoom->get_tags().to_string(true).to_char());
								for_every_ptr(d, debugRoom->get_all_doors())
								{
									log.log(TXT("dr'%p"), d);
									log.log(TXT("d%p"), d->get_door());
									log.log(TXT("%i type \"%S\""), for_everys_index(d), d->get_door()->get_door_type()->get_name().to_string().to_char());
									LOG_INDENT(log);
									if (!d->is_visible())
									{
										log.set_colour(Colour::red);
										log.log(TXT("HIDDEN / NOT VISIBLE / NOT ACTIVE"));
										log.set_colour();
									}
									if (d->get_door()->get_vr_elevator_altitude().is_set())
									{
										log.log(TXT("vr elevator altitude : %.3f"), d->get_door()->get_vr_elevator_altitude().get());
									}
									else
									{
										log.log(TXT("no vr elevator altitude provided"));
									}
									log.log(TXT("relative size %.3f x %.3f"), d->get_door()->get_relative_size().x, d->get_door()->get_relative_size().y);
									log.log(TXT("hole scale %.3f x %.3f"), d->get_door()->get_hole_scale().x, d->get_door()->get_hole_scale().y);
									if (auto* dt = d->get_door()->get_original_door_type())
									{
										log.log(TXT("original type \"%S\""), dt->get_name().to_string().to_char());
									}
									if (auto* dt = d->get_door()->get_forced_door_type())
									{
										log.log(TXT("forced type \"%S\""), dt->get_name().to_string().to_char());
									}
									if (auto* dt = d->get_door()->get_secondary_door_type())
									{
										log.log(TXT("secondary type \"%S\""), dt->get_name().to_string().to_char());
									}
									log.log(TXT("group id %i"), d->get_group_id());
									if (d->get_door()->is_important_vr_door())
									{
										log.set_colour(Colour::cyan);
										log.log(TXT("VR IMPORTANT"));
										log.set_colour();
									}
									if (d->get_door()->is_world_separator_door())
									{
										log.set_colour(Colour::yellow);
										log.log(TXT("WORLD SEPARATOR"));
										log.set_colour();
									}
									if (d->is_vr_space_placement_set())
									{
										if (d->is_vr_placement_immovable())
										{
											log.log(TXT("vr placement set and immovable"));
										}
										else
										{
											log.log(TXT("vr placement set"));
										}
										log.log(TXT("   at %S"), d->get_vr_space_placement().get_translation().to_string().to_char());
										log.log(TXT("      %S"), d->get_vr_space_placement().get_orientation().to_rotator().to_string().to_char());
									}
									log.log(TXT("place %S"), d->get_placement().get_translation().to_string().to_char());
									log.log(TXT("      %S"), d->get_placement().get_orientation().to_rotator().to_string().to_char());
									log.log(TXT("tags    %S"), d->get_tags().to_string(true).to_char());
									log.log(TXT("operate %S"), Framework::DoorOperation::to_char(d->get_door()->get_operation()));
									if (d->get_door())
									{
										if (d->get_door()->get_vr_elevator_altitude().is_set())
										{
											log.log(TXT("vralt   %.3f"), d->get_door()->get_vr_elevator_altitude().get());
										}
										else
										{
											log.log(TXT("vralt   --"));
										}
									}
									log.log(TXT("d-tags  %S"), d->get_door()? d->get_door()->get_tags().to_string(true).to_char() : TXT(""));
									log.log(TXT("op.lock %S%S"), d->get_door()->is_operation_locked() ? TXT("locked -> ") : TXT("not locked"),
										d->get_door()->is_operation_locked() ? Framework::DoorOperation::to_char(d->get_door()->get_operation_if_unlocked()) : TXT(""));
									log.log(TXT("in room r%p (r%p)"), d->get_in_room(), debugRoom);
									log.log(TXT("this    dr'%p in r%p"), d, d->get_in_room());
									log.log(TXT("other   dr'%p in r%p"), d->get_door_on_other_side(), d->get_door_on_other_side()? d->get_door_on_other_side()->get_in_room() : nullptr);
									log.log(TXT("part of d%p"), d->get_door());
									if (d->are_meshes_ready())
									{
										log.log(TXT("meshes done for:"));
									}
									else
									{
										log.set_colour(Colour::red);
										log.log(TXT("MESHES NOT READY, done? for:"));
										log.set_colour();
									}
									log.log(TXT("  this  %S (%S)"), d->get_meshes_done_for_door_type()? d->get_meshes_done_for_door_type()->get_name().to_string().to_char() : TXT("??")
										, d->get_meshes_done_for_secondary_door_type() ? d->get_meshes_done_for_secondary_door_type()->get_name().to_string().to_char() : TXT("??"));
									if (auto* o = d->get_door_on_other_side())
									{
										log.log(TXT("  other %S (%S)"), o->get_meshes_done_for_door_type() ? o->get_meshes_done_for_door_type()->get_name().to_string().to_char() : TXT("??")
											, o->get_meshes_done_for_secondary_door_type() ? o->get_meshes_done_for_secondary_door_type()->get_name().to_string().to_char() : TXT("??"));
									}

									log.log(TXT("to room r%p %S%S [%S]"),
										d->get_room_on_other_side(),
										d->get_room_on_other_side()? d->get_room_on_other_side()->get_name().to_char() : TXT("--"),
										d->get_room_on_other_side() && ! d->get_room_on_other_side()->is_world_active()? TXT(" [inactive]") : TXT(""),
										d->get_room_on_other_side()? d->get_room_on_other_side()->get_tags().to_string(true).to_char() : TXT(""));
									if (d->get_room_on_other_side())
									{
										if (d->get_room_on_other_side()->get_all_doors().does_contain(d->get_door_on_other_side()))
										{
											log.log(TXT("door on the other side is in the right room"));
										}
										else
										{
											log.set_colour(Colour::red);
											log.log(TXT("door on the other side NOT PRESENT in the other room"));
											log.set_colour();
										}
									}
									if (!d->get_door()->get_variables().is_empty())
									{
										log.log(TXT("door's variables"));
										LOG_INDENT(log);
										d->get_door()->get_variables().log(log);
									}
#ifdef WITH_DOOR_IN_ROOM_HISTORY
#ifdef SHOW_DOOR_IN_ROOM_HISTORY
									{
										log.log(TXT("history"));
										LOG_INDENT(log);
										int idx = 0;
										for_every(entry, d->history)
										{
											log.log(TXT("%02i %S"), idx, entry->to_char());
											++idx;
										}
									}
									if (auto* o = d->get_door_on_other_side())
									{
										log.log(TXT("other history"));
										LOG_INDENT(log);
										int idx = 0;
										for_every(entry, o->history)
										{
											log.log(TXT("%02i %S"), idx, entry->to_char());
											++idx;
										}
									}
#endif
#endif

								}
							}
							log.log(TXT(""));
						}
					}

					if (debugLocomotion)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto * ai = imo->get_ai())
							{
								if (auto * mind = ai->get_mind())
								{
									log.set_colour(headerColour);
									log.log(TXT("LOCOMOTION"));
									log.set_colour();
									mind->access_locomotion().debug_log(log);
									log.log(TXT(""));
								}
							}
						}
					}

					if (debugVariables)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							log.set_colour(headerColour);
							log.log(TXT("VARIABLES"));
							log.set_colour();
							imo->get_variables().log(log);
							log.log(TXT(""));
						}
					}
					
					if (debugAICommonVariables)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto * ai = imo->get_ai())
							{
								if (auto * mind = ai->get_mind())
								{
									log.set_colour(headerColour);
									log.log(TXT("AI COMMON VARIABLES"));
									log.set_colour();
									AI::log_common_variables(log, mind);
									log.log(TXT(""));
								}
							}
						}
					}

					if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
					{
						if (auto * a = imo->get_appearance())
						{
							if (debugDrawSkeletons && a->get_skeleton())
							{
								log.set_colour(headerColour);
								log.log(TXT("SKELETON"));
								log.set_colour();
								a->get_skeleton()->log(log);
							}

							if (debugACList)
							{
								log.set_colour(headerColour);
								log.log(TXT("APPEARANCE CONTROLLERS"));
								log.set_colour();
								Framework::AppearanceControllerPoseContext poseContext;
								poseContext.reset_processing();
								while (poseContext.is_more_processing_required())
								{
									poseContext.start_next_processing();
									for_every_ref(ac, a->get_controllers())
									{
										if (ac->check_processing_order(poseContext))
										{
											ac->log(log);
										}
									}
								}
							}
						}
					}

					if (debugAIHunches)
					{
						if (Framework::IModulesOwner const* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto* ai = imo->get_ai())
							{
								log.set_colour(headerColour);
								log.log(TXT("AI HUNCHES"));
								log.set_colour();
								{
									ai->log_hunches(log);
								}
								log.log(TXT(""));
							}
						}
					}

					if (debugAILogic)
					{
						if (Framework::IModulesOwner const* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto* ai = imo->get_ai())
							{
								if (auto* mind = ai->get_mind())
								{
									if (auto* logic = fast_cast<Framework::AI::Logic>(mind->get_logic()))
									{
										log.set_colour(headerColour);
										log.log(TXT("AI LOGIC INFO"));
										log.set_colour();
										{
											logic->log_logic_info(log);
										}
										log.log(TXT(""));
									}
								}

								if (auto* mai = fast_cast<ModuleAI>(ai))
								{
									if (auto* gse = mai->get_game_script_execution())
									{
										gse->log(log);
										log.log(TXT(""));
									}
								}
							}
						}
					}

					if (debugAILatent)
					{
						if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto * ai = imo->get_ai())
							{
								if (auto * mind = ai->get_mind())
								{
									if (auto* logic = fast_cast<Framework::AI::LogicWithLatentTask>(mind->get_logic()))
									{
										log.set_colour(headerColour);
										log.log(TXT("AI LATENT"));
										log.set_colour();
										{
											LOG_INDENT(log);
											debugAILatentFrameInspection.ready();
											logic->log_latent_funcs(log, debugAILatentFrameInspection);
											if (ctrlHold)
											{
												debugAILatentFrameInspection.change_frame(mouseWheel);
												mouseWheel = 0; // consume
											}
										}
										if (auto* frame = debugAILatentFrameInspection.get_frame_to_inspect())
										{
											LOG_INDENT(log);
											log.log(TXT(""));
											log.log(TXT("FRAME"));
											LOG_INDENT(log);
											frame->log(log);
										}
										log.log(TXT(""));
									}
								}
							}
						}
					}

#ifdef AN_USE_FRAME_INFO
					if (debugOutputRenderDetails)
					{
						log.set_colour(headerColour);
						log.log(TXT("RENDER DETAILS")); // previous frame because we're drawing current one
						log.set_colour();
						auto const* frameInfo = debugOutputRenderDetailsLocked ? &debugOutputRenderDetailsLockedFrame : prevFrameInfo;
						log.log(TXT("frame [%05i]"), frameInfo->frameNo);
						frameInfo->systemInfo.log(log, true);
						log.log(TXT(""));
					}
#endif

					if (debugOutputMusicPlayer)
					{
						log.set_colour(headerColour);
						log.log(TXT("MUSIC PLAYER"));
						log.set_colour();
						if (musicPlayer)
						{
							musicPlayer->log(log);
						}
						log.log(TXT(""));
					}

					if (debugOutputEnvironmentPlayer)
					{
						log.set_colour(headerColour);
						log.log(TXT("ENVIRONMENT PLAYER"));
						log.set_colour();
						if (environmentPlayer)
						{
							environmentPlayer->log(log);
						}
						log.log(TXT(""));
					}

					if (debugOutputVoiceoverSystem)
					{
						log.set_colour(headerColour);
						log.log(TXT("VOICEOVER SYSTEM"));
						log.set_colour();
						if (voiceoverSystem)
						{
							voiceoverSystem->log(log);
						}
						log.log(TXT(""));
					}

					if (debugOutputSubtitleSystem)
					{
						log.set_colour(headerColour);
						log.log(TXT("SUBTITLE SYSTEM"));
						log.set_colour();
						if (subtitleSystem)
						{
							subtitleSystem->log(log);
						}
						log.log(TXT(""));
					}

					if (debugOutputRenderScene)
					{
						log.set_colour(headerColour);
						log.log(TXT("RENDER SCENES"));
						log.set_colour();
						for_every_ref(renderScene, renderScenes)
						{
							LOG_INDENT(log);
							log.log(TXT("RENDER SCENE %i"), for_everys_index(renderScene));
							LOG_INDENT(log);
							renderScene->log(log);
							log.log(TXT(""));
						}
					}

					if (debugOutputSoundSystem)
					{
						log.set_colour(headerColour);
						log.log(TXT("SOUND SYSTEM"));
						log.set_colour();
						LOG_INDENT(log);
						if (auto* ss = ::System::Sound::get())
						{
							ss->log(log);
						}
						log.log(TXT(""));
					}

					if (debugOutputSoundScene)
					{
						log.set_colour(headerColour);
						log.log(TXT("SOUND SCENE"));
						log.set_colour();
						LOG_INDENT(log);
						soundScene->log(log);
						log.log(TXT(""));
					}

					if (debugOutputPilgrim)
					{
						log.set_colour(headerColour);
						log.log(TXT("PILGRIM"));
						LOG_INDENT(log);
						log.set_colour();
						if (auto* pa = access_player().get_actor())
						{
							if (auto* mp = pa->get_gameplay_as<ModulePilgrim>())
							{
								mp->log(log);
							}
						}
						log.log(TXT(""));
					}
					
					if (debugOutputGameplayStuff)
					{
						log.set_colour(headerColour);
						log.log(TXT("GAMEPLAY STUFF"));
						LOG_INDENT(log);
						log.set_colour();
						if (Framework::IModulesOwner const* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							if (auto* h = imo->get_custom<CustomModules::Health>())
							{
								log.log(TXT("health: %S"), h->get_health().as_string().to_char());
							}							
						}
						log.log(TXT(""));
					}

					if (debugOutputGameDirector)
					{
						log.set_colour(headerColour);
						log.log(TXT("GAME DIRECTOR"));
						LOG_INDENT(log);
						log.set_colour();
						if (auto* gd = GameDirector::get())
						{
							gd->log(log);
						}
						log.log(TXT(""));
					}

					if (debugOutputLockerRoom)
					{
						log.set_colour(headerColour);
						log.log(TXT("LOCKER ROOM"));
						LOG_INDENT(log);
						log.set_colour();
						if (auto* lr = get_locker_room())
						{
							LOG_INDENT(log);
							FOR_EVERY_ALL_OBJECT_IN_ROOM(o, lr)
							{
								log.log(TXT("[%S] %S /%S/"), o->get_object_type()->get_name().to_string().to_char(),
									o->get_name().to_char(),
									o->get_tags().to_string().to_char());
							}
						}
						log.log(TXT(""));
					}

#ifdef AN_ALLOW_BULLSHOTS
					if (debugOutputBullshotSystem)
					{
						log.set_colour(headerColour);
						log.log(TXT("BULLSHOT SYSTEM"));
						LOG_INDENT(log);
						log.set_colour();
						Framework::BullshotSystem::log(log);
						log.log(TXT(""));
					}
#endif

					if (debugHighLevelRegion.outputInfo)
					{
						log.set_colour(headerColour);
						log.log(TXT("HIGH LEVEL REGION INFO"));
						LOG_INDENT(log);
						log.set_colour();
						log_high_level_info_region(log, debugHighLevelRegion, NAME(highLevelRegion), NAME(highLevelRegionInfo));
						log.log(TXT(""));
					}

					if (debugCellLevelRegion.outputInfo)
					{
						log.set_colour(headerColour);
						log.log(TXT("CELL LEVEL REGION INFO"));
						LOG_INDENT(log);
						log.set_colour();
						log_high_level_info_region(log, debugCellLevelRegion, NAME(cellLevelRegion), NAME(cellLevelRegionInfo));
						log.log(TXT(""));
					}

					if (debugRoomRegion.outputInfo)
					{
						log.set_colour(headerColour);
						log.log(TXT("ROOM REGION"));
						LOG_INDENT(log);
						log.set_colour();
						log_room_region(log, debugRoomRegion);
						log.log(TXT(""));
					}

					if (debugOutputPilgrimageInfo)
					{
						log.set_colour(headerColour);
						log.log(TXT("PILGRIMAGE"));
						log.set_colour(subHeaderColour);
						if (debugOutputPilgrimageInfo == 1)
						{
							log.log(TXT("pilgrimage instance"));
						}
						if (debugOutputPilgrimageInfo == 2)
						{
							log.log(TXT("environments"));
						}
						log.set_colour();
						if (auto* pm = fast_cast<GameModes::Pilgrimage>(get_mode()))
						{
							auto& pi = pm->access_pilgrimage_instance();
							if (debugOutputPilgrimageInfo == 1)
							{
								pi.log(log);
							}
							if (debugOutputPilgrimageInfo == 2)
							{
								pi.debug_log_environments(log);
							}
						}
						log.log(TXT(""));
					}

					if (debugShowWorldLog)
					{
						log.set_colour(headerColour);
						log.log(TXT("WORLD"));
						log.set_colour();
						log.log(worldLog);
						log.log(TXT(""));
					}

					if (showLogPanelVR)
					{
						if (Framework::IModulesOwner const* imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
						{
							debugLogPanel->show_log_title(imo->ai_get_name().to_char());
						}
						else
						{
							debugLogPanel->show_log_title();
						}
						debugLogPanel->show_log(&debugLog);
					}

					if (showDebugInfo)
					{
						Colour defaultColour = Colour::lerp(0.7f, Colour::greyLight, Colour::white);

						Vector2 scale = fontSizeMultiplier;
						float linesSeparation = font->get_height() * scale.y;

						y = maxLogLineY;
						y -= linesSeparation;

						static int outputScroll = 0;

						if (!ctrlHold)
						{
							outputScroll += mouseWheel;
						}
						outputScroll = clamp(outputScroll, 0, log.get_lines().get_size() - 1);

						if (!log.get_lines().is_empty())
						{
							//System::Video3DPrimitives::fill_rect_2d(Colour::black.with_alpha(0.15f), Vector2::zero, v3d->get_viewport_size().to_vector2(), true);
						}

						int lineLength = max(10, (int)(0.7f * ((v3d->get_screen_size().to_vector2().x / font->calculate_char_size().x - 1.0f) / fontSizeMultiplier.x)));
						int lineIdx = 0;
						for_every(line, log.get_lines())
						{
							if (y > -linesSeparation)
							{
								Colour currentOutline = Colour::black;
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
										if (lineIdx >= outputScroll)
										{
											font->draw_text_at_with_border(v3d, startLineWith + logLine.get_left(spaceAt), line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
											y -= linesSeparation;
										}
										logLine = logLine.get_sub(spaceAt + 1);
										if (startLineWith.is_empty())
										{
											startLineWith = TXT(" ~ ");
										}
										++lineIdx;
									}
									// leftover
									if (lineIdx >= outputScroll)
									{
										font->draw_text_at_with_border(v3d, startLineWith + logLine, line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
										y -= linesSeparation;
									}
									++lineIdx;
								}
								else
								{
									if (lineIdx >= outputScroll)
									{
										font->draw_text_at_with_border(v3d, line->text, line->colour.is_set() ? line->colour.get() : defaultColour, currentOutline, 1.0f, Vector2(x, y), scale);
										y -= linesSeparation;
									}
									++lineIdx;
								}
							}
						}
					}
				}
			}
			set_ignore_retro_rendering_for(font, false);
		}
	}
#endif

	// in case we bound to anything, unbind
	System::RenderTarget::unbind_to_none();

	RESTORE_STORE_DIRECT_GL_CALL();
	RESTORE_STORE_DRAW_CALL();

	MEASURE_PERFORMANCE_FINISH_RENDERING();
}

void Game::debug_draw()
{
#ifdef AN_DEBUG_RENDERER
	Framework::Room* showInRoom = nullptr;
	Transform showPlacement = Transform::identity;
	if (auto * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
	{
		bool drawNow = true;
		if (imo == player.get_actor() &&
			debugCameraMode == 0)
		{
			drawNow = false;
		}
		if (drawNow)
		{
			if (auto* p = imo->get_presence())
			{
				// highlight
				p->debug_draw(get_debug_highlight_colour(), 0.1f, true);
				debug_context(p->get_in_room());
				debug_draw_transform_size(true, p->get_placement(), 0.5f);
				debug_no_context();
			}
		}
	}
	if (player.get_actor())
	{
		showInRoom = player.get_actor()->get_presence()->get_in_room();
		showPlacement = player.get_actor()->get_presence()->get_placement();
	}
	if (debugCameraMode)
	{
		showInRoom = debugCameraInRoom.get();
		showPlacement = debugCameraPlacement;
	}
	if (!world->does_have_active_room(showInRoom))
	{
		showInRoom = nullptr;
	}
	if (debugDrawCollision != 0 && showInRoom)
	{
		int collisionFlags = 0;
		if (debugDrawCollisionMode == 0)
		{
			collisionFlags = NONE;
		}
		if (debugDrawCollisionMode == 1)
		{
			collisionFlags = AgainstCollision::Movement;
		}
		if (debugDrawCollisionMode == 2)
		{
			collisionFlags = AgainstCollision::Precise;
		}
		showInRoom->debug_draw_collision(collisionFlags, debugDrawCollision <= 2, debugDrawCollision == 1 ? player.get_actor() : nullptr);
	}
	if (debugDrawDoors != 0 && showInRoom)
	{
		showInRoom->debug_draw_door_holes();
	}
	if (debugDrawVolumetricLimit != 0 && showInRoom)
	{
		showInRoom->debug_draw_volumetric_limit();
	}
	if (debugDrawSkeletons != 0 && showInRoom)
	{
		showInRoom->debug_draw_skeletons();
	}
	if (debugDrawSockets != 0 && showInRoom)
	{
		showInRoom->debug_draw_sockets();
	}
	if (debugDrawPOIs != 0 && showInRoom)
	{
		showInRoom->debug_draw_pois();
		if (debugDrawPOIs == 2 && showInRoom)
		{
			if (auto* r = showInRoom->get_room_for_rendering_additional_objects())
			{
				debug_context(showInRoom);
				r->debug_draw_pois(false);
				debug_no_context();
			}
		}
	}
	if (debugDrawMeshNodes != 0 && showInRoom)
	{
		showInRoom->debug_draw_mesh_nodes();
	}
	if (debugDrawPresence != 0 && showInRoom)
	{
		//FOR_EVERY_OBJECT_IN_ROOM(object, showInRoom)
		for_every_ptr(object, showInRoom->get_objects())
		{
			object->get_presence()->debug_draw();
		}
	}
	if (debugDrawPresenceBaseInfo != 0 && showInRoom)
	{
		//FOR_EVERY_OBJECT_IN_ROOM(object, showInRoom)
		for_every_ptr(object, showInRoom->get_objects())
		{
			object->get_presence()->debug_draw_base_info();
		}
	}	
	if (debugLocomotionPath)
	{
		if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
		{
			if (auto * ai = imo->get_ai())
			{
				ai->get_mind()->access_locomotion().debug_draw();
			}
		}
		// else all?
	}
	if (debugAIState)
	{
		if (Framework::IModulesOwner const * imo = fast_cast<Framework::IModulesOwner>(debugSelectDebugSubjectHit.get()))
		{
			if (auto * ai = imo->get_ai())
			{
				if (auto* l = ai->get_mind()->get_logic())
				{
					l->debug_draw_state();
				}
			}
		}
		else if (subWorld)
		{
			for_every_ptr(o, subWorld->get_all_objects())
			{
				if (auto * ai = o->get_ai())
				{
					if (auto* l = ai->get_mind()->get_logic())
					{
						l->debug_draw_state();
					}
				}
			}
		}
	}
	if (showInRoom)
	{
		showInRoom->debug_draw_nav_mesh();
	}
	if (debugDrawVRAnchor != 0 && showInRoom)
	{
		Transform vrAnchor;
		Optional<Vector3> from;
		bool notHere = false;
		if (player.get_actor() && showInRoom == player.get_actor()->get_presence()->get_in_room())
		{
			vrAnchor = player.get_actor()->get_presence()->get_vr_anchor();
			from = player.get_actor()->get_presence()->get_placement().get_translation();
		}
		else
		{
			notHere = true;
			vrAnchor = showInRoom->get_vr_anchor(Transform::identity);
		}
		RoomGenerationInfo const& rgi = RoomGenerationInfo::get();
		Colour colour = Colour::lerp(notHere ? 0.85f : 0.0f, Colour::green, Colour::grey);
		{
			float centreSize = 1.0f;
			debug_draw_arrow(true, colour, vrAnchor.get_translation() - vrAnchor.get_axis(Axis::X) * centreSize * 0.5f, vrAnchor.get_translation() + vrAnchor.get_axis(Axis::X) * centreSize * 0.5f);
			debug_draw_arrow(true, colour, vrAnchor.get_translation() - vrAnchor.get_axis(Axis::Y) * centreSize * 0.5f, vrAnchor.get_translation() + vrAnchor.get_axis(Axis::Y) * centreSize * 0.5f);
		}
		if (from.is_set())
		{
			debug_draw_arrow(true, colour, from.get(), vrAnchor.get_translation());
		}

		if (auto* vr = VR::IVR::get())
		{
			if (vr->is_controls_view_valid())
			{
				Transform viewVR = vr->get_controls_view();
				debug_push_transform(vrAnchor);
				debug_draw_transform_size_coloured_lerp(true, viewVR, 0.3f, Colour::blue, 0.8f);
				debug_pop_transform();
			}
		}
	}
	if (debugDrawRGI != 0 && showInRoom)
	{
		Transform vrAnchor;
		bool notHere = false;
		if (player.get_actor() && showInRoom == player.get_actor()->get_presence()->get_in_room())
		{
			vrAnchor = player.get_actor()->get_presence()->get_vr_anchor();
		}
		else
		{
			notHere = true;
			vrAnchor = showInRoom->get_vr_anchor(Transform::identity);
		}
		RoomGenerationInfo const & rgi = RoomGenerationInfo::get();
		Colour colour = Colour::lerp(notHere ? 0.85f : 0.0f, Colour::green, Colour::grey);
		{
			float centreSize = 1.0f;
			debug_draw_arrow(true, colour, vrAnchor.get_translation() - vrAnchor.get_axis(Axis::X) * centreSize * 0.5f, vrAnchor.get_translation() + vrAnchor.get_axis(Axis::X) * centreSize * 0.5f);
			debug_draw_arrow(true, colour, vrAnchor.get_translation() - vrAnchor.get_axis(Axis::Y) * centreSize * 0.5f, vrAnchor.get_translation() + vrAnchor.get_axis(Axis::Y) * centreSize * 0.5f);
		}
		if (debugDrawRGI == 1)
		{
			rgi.get_play_area_zone().debug_render(colour, vrAnchor);
		}
		if (debugDrawRGI == 2)
		{
			rgi.get_tiles_zone().debug_render(Colour::lerp(notHere ? 0.85f : 0.0f, Colour::yellow, Colour::grey), vrAnchor);
			for_count(int, x, rgi.get_tile_count().x)
			{
				for_count(int, y, rgi.get_tile_count().y)
				{
					Transform at = Transform::identity;
					at.set_translation(rgi.get_first_tile_centre_offset().to_vector3() + Vector3(rgi.get_tile_size() * (float)x, rgi.get_tile_size() * (float)y, 0.0f));
					rgi.get_tile_zone().debug_render(Colour::yellow.with_alpha(0.3f), vrAnchor.to_world(at));
				}
			}
		}
	}
	if (debugDrawVRPlayArea != 0)
	{
		Transform vrAnchor;
		if (player.get_actor() && player.get_actor()->get_presence()->get_in_room())
		{
			vrAnchor = player.get_actor()->get_presence()->get_vr_anchor();
#ifdef AN_DEVELOPMENT
			if (auto* vr = VR::IVR::get())
			{
				if (debugDrawVRPlayArea == 2)
				{
					vr->debug_render_whole_play_area_rect(NP, vrAnchor, true);
				}
				if (debugDrawVRPlayArea >= 1)
				{
					vr->debug_render_play_area_rect(NP, vrAnchor);
				}
			}
#endif
		}
	}
	if (debugDrawSpaceBlockers && showInRoom)
	{
		showInRoom->debug_draw_space_blockers();
	}
	if (debugFindLocationOnNavMesh && showInRoom)
	{
		auto result = showInRoom->find_nav_location(showPlacement);

		debug_context(showInRoom);

		Colour colour = Colour::red;
		Transform placementWS = showPlacement;
		if (result.is_valid())
		{
			placementWS = result.get_current_placement();
			colour = Colour::blue;
		}
		debug_push_transform(placementWS);
		Vector3 offset(0.1f, 0.0f, 0.1f);
		debug_draw_line(true, colour, Vector3::zero, Vector3( offset.x,  offset.y, offset.z));
		debug_draw_line(true, colour, Vector3::zero, Vector3(-offset.x, -offset.y, offset.z));
		debug_draw_line(true, colour, Vector3::zero, Vector3( offset.y, -offset.x, offset.z));
		debug_draw_line(true, colour, Vector3::zero, Vector3(-offset.y,  offset.x, offset.z));
		debug_pop_transform();

		debug_no_context();
	}
	if (debugFindPathOnNavMesh && player.get_actor() && showInRoom)
	{
		if (findPathTask.is_set())
		{
			debugFindPathOnNavMeshBlockedFor = 0.1f;
		}
		else
		{
			debugFindPathOnNavMeshBlockedFor = max(0.0f, debugFindPathOnNavMeshBlockedFor - ::System::Core::get_raw_delta_time());
		}
		if (debugFindPathOnNavMeshBlockedFor == 0.0f)
		{
			// !@#
			findPathTask = Framework::Nav::Tasks::FindPath::new_task(
				player.get_actor()->get_presence()->get_in_room()->find_nav_location(player.get_actor()->get_presence()->get_placement()),
				showInRoom->find_nav_location(showPlacement),
				Framework::Nav::PathRequestInfo(player.get_actor()));
			get_nav_system()->add(findPathTask.get());
		}
		if (findPathTask.is_set() && findPathTask->is_done())
		{
			foundPathTask = findPathTask;
			findPathTask.clear();
		}
		if (foundPathTask.is_set())
		{
			if (auto * foundPath = fast_cast<Framework::Nav::Tasks::PathTask>(foundPathTask.get()))
			{
				if (auto const * path = foundPath->get_path().get())
				{
					Framework::Nav::PathNode const * prev = nullptr;
					for_every(pathNode, path->get_path_nodes())
					{
						if (prev &&
							prev->placementAtNode.node->get_room() == pathNode->placementAtNode.node->get_room())
						{
							debug_context(prev->placementAtNode.node->get_room());
							Colour colour = Colour::yellow;
							if (!pathNode->is_outdated() && pathNode->connectionData.is_set() && pathNode->connectionData->is_blocked_temporarily())
							{
								colour = Colour::magenta;
							}
							if (path->is_path_blocked_temporarily(for_everys_index(pathNode), prev->placementAtNode.get_current_placement().get_translation()))
							{
								colour = Colour::magenta;
							}
							debug_draw_arrow(true, colour, prev->placementAtNode.get_current_placement().get_translation(),
														   pathNode->placementAtNode.get_current_placement().get_translation());
							debug_no_context();
						}
						prev = pathNode;
					}
				}
			}
		}
	}
	else
	{
		debugFindPathOnNavMeshBlockedFor = 0.0f;
		findPathTask.clear();
	}
#endif
}

bool Game::process_debug_controls(bool _forLoaderOnly)
{
	static int lastFrame = 0;
	int thisFrame = ::System::Core::get_frame();
	if (lastFrame == thisFrame)
	{
		return debugControlsMode && debugCameraMode;
	}
	lastFrame = thisFrame;

	MEASURE_PERFORMANCE(processDebugControls);

#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
	/*
	if (System::Input::get()->has_key_been_pressed(System::Key::LeftBracket))
	{
		add_sync_world_job(TXT("destroy testRoom1's neighbours"), [this]()
		{
			// this is used to test checking if attached objects remain
			DEFINE_STATIC_NAME(testRoom2);
			for_every_ptr(r, world->get_all_rooms())
			{
				if (r->get_tags().get_tag(NAME(testRoom2)))
				{
					for_every_ptr(dir, r->get_all_doors())
					{
						if (auto* r = dir->get_room_on_other_side())
						{
							delete r;
							break;
						}
					}
					break;
				}
			}
		});
	}
	*/

	if (System::Input::get()->has_key_been_pressed(System::Key::Tilde))
	{
		bool shiftHold = System::Input::get()->is_key_pressed(System::Key::LeftShift) ||
						 System::Input::get()->is_key_pressed(System::Key::RightShift);
#ifdef RENDER_HITCH_FRAME_INDICATOR
		if (shiftHold)
		{
			allowShowHitchFrameIndicator = !allowShowHitchFrameIndicator;
		}
		else
#endif
		{
			debugRenderingSuspended = !debugRenderingSuspended;
		}
	}
#endif
#endif

	/*
	// to test the system
	if (System::Input::get()->has_key_been_pressed(System::Key::N7))
	{
		Framework::JobSystem::use_batches(false);
	}
	if (System::Input::get()->has_key_been_pressed(System::Key::N8))
	{
		Framework::JobSystem::use_batches(true);
	}
	*/

#ifdef AN_STANDARD_INPUT
	if (System::Input::get()->has_key_been_pressed(System::Key::Space) &&
		System::Input::get()->is_key_pressed(System::Key::LeftCtrl))
	{
		output(TXT("MARKER"));
	}
#endif

	if (_forLoaderOnly)
	{
		return false;
	}

#ifdef AN_STANDARD_INPUT
	bool ctrlHold = System::Input::get()->is_key_pressed(System::Key::LeftCtrl) ||
					System::Input::get()->is_key_pressed(System::Key::RightCtrl);
	bool shiftHold = System::Input::get()->is_key_pressed(System::Key::LeftShift) ||
					 System::Input::get()->is_key_pressed(System::Key::RightShift);
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (world)
	{
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef AN_STANDARD_INPUT
		if (System::Input::get()->has_key_been_pressed(System::Key::X))
		{
			autopilot = !autopilot;
		}
#endif
		if (autopilot)
		{
			// with autopilot kill on collision
			if (auto* a = player.get_actor())
			{
				if (auto* c = a->get_collision())
				{
					for_every(ci, c->get_collided_with())
					{
						if (auto* cwo = fast_cast<Framework::Object>(ci->collidedWithObject.get()))
						{
							if (auto* h = cwo->get_custom<CustomModules::Health>())
							{
								h->perform_death();
							}
						}
					}
				}
			}
		}
#ifdef AN_STANDARD_INPUT
		if (System::Input::get()->has_key_been_pressed(System::Key::K))
		{
			if (auto* a = player.get_actor())
			{
				if (System::Input::get()->is_key_pressed(System::Key::LeftShift))
				{
					if (auto* h = a->get_custom<CustomModules::Health>())
					{
						h->perform_death();
					}
				}
				else
				{
					if (auto* r = a->get_presence()->get_in_room())
					{
						for_every_ptr(o, r->get_objects())
						{
							if (o != a)
							{
								if (auto* h = o->get_custom<CustomModules::Health>())
								{
									h->perform_death();
									//h->spawn_particles_on_decompose();
								}
							}
						}
					}
				}
			}
		}
#endif
#endif
	}
#endif

#ifndef AN_RECORD_VIDEO
#ifdef AN_STANDARD_INPUT
	if (System::Input::get()->has_key_been_pressed(System::Key::C))
	{
		change_show_debug_panel(GameSettings::get().update_and_get_debug_panel(), false);
	}
	if (System::Input::get()->has_key_been_pressed(System::Key::Z))
	{
		change_show_debug_panel(debugPanel, false);
	}
#endif

#ifdef AN_DEVELOPMENT_OR_PROFILER
	if (does_use_vr())
	{
		if (allowVRDebugPanel && allTimeControlsInput.has_button_been_pressed(NAME(leftVRDebugPanel)))
		{
			change_show_debug_panel(debugPanel, true);
		}
	}
#endif

	if (does_use_vr())
	{
		if (allowVRDebugPanel)
		{
			if (allTimeControlsInput.has_button_been_pressed(NAME(rightVRDebugPanel))
#ifndef AN_DEVELOPMENT_OR_PROFILER
				|| allTimeControlsInput.has_button_been_pressed(NAME(leftVRDebugPanel))
#endif			
				)
			{
				change_show_debug_panel(GameSettings::get().update_and_get_debug_panel(), true);
			}
		}
#ifdef AN_DEVELOPMENT_OR_PROFILER
		if (showDebugPanelVR)
		{
			debugQuickSelectDebugSubject = allTimeControlsInput.is_button_pressed(NAME(quickSelectDebugSubject));
		}
		else
		{
			debugQuickSelectDebugSubject = false;
		}
#endif
	}
#ifdef AN_DEVELOPMENT_OR_PROFILER
	else
	{
		debugQuickSelectDebugSubject = false;
	}
#endif
#endif

	//
#ifdef AN_DEBUG_KEYS
#ifdef AN_STANDARD_INPUT
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (System::Input::get()->has_key_been_pressed(System::Key::G))
	{
		if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
		{
			c->access_external_view_info().showBoundary = c->access_external_view_info().showBoundary == ExternalViewShowBoundary::No ? ExternalViewShowBoundary::AllWalls : ExternalViewShowBoundary::No;
		}
	}
	if (System::Input::get()->is_key_pressed(System::Key::LeftCtrl) &&
		System::Input::get()->has_key_been_pressed(System::Key::F2))
	{
		if (System::Input::get()->is_key_pressed(System::Key::LeftAlt))
		{
			if (System::Input::get()->is_key_pressed(System::Key::LeftShift))
			{
				if (auto* c = TeaForGodEmperor::GameConfig::get_as<TeaForGodEmperor::GameConfig>())
				{
					int i = c->access_external_view_info().showBoundary + 1;
					i = mod<int>(i, ExternalViewShowBoundary::MAX);
					c->access_external_view_info().showBoundary = (ExternalViewShowBoundary::Type)i;
				}
			}
			else
			{
				externalRenderPlayArea.render = !externalRenderPlayArea.render;
			}
		}
		else
		{
			overrideExternalRenderScene = true;
			if (System::Input::get()->is_key_pressed(System::Key::LeftShift))
			{
				onlyExternalRenderScene = !onlyExternalRenderScene;
			}
			else
			{
				useExternalRenderScene = !useExternalRenderScene;
				onlyExternalRenderScene = false;
			}
		}
	}
	else
#endif
	{
		if (System::Input::get()->has_key_been_pressed(System::Key::F1))
		{
			if (System::Input::get()->is_key_pressed(System::Key::F2))
			{
				// F2->F1 player controls, player view, keep debug camera as is
				debugControlsMode = 0;
				debugCameraMode = 0;
			}
			else
			{
				debugControlsMode = 0;
			}
		}
		else if (System::Input::get()->is_key_pressed(System::Key::F1))
		{
			if (System::Input::get()->has_key_been_pressed(System::Key::F2))
			{
				// F1->F2 camera controls, camera view + reset
				debugControlsMode = 1;
				if (debugCameraMode == 0)
				{
					debugCameraMode = 1;
				}
				resetDebugCamera = true;
			}
		}
		else
		{
			if (System::Input::get()->has_key_been_pressed(System::Key::F2))
			{
				// F2 only, cycle camera view
				if (debugControlsMode == 0)
				{
					if (debugCameraMode == 0)
					{
						debugCameraMode = 1;
					}
				}
				else if (debugControlsMode == 1)
				{
					debugCameraMode = mod(debugCameraMode + (shiftHold ? -1 : 1), 3);
				}
				debugControlsMode = 1;
			}
		}
	}
#endif

#ifdef AN_ALLOW_BULLSHOTS
#ifdef AN_STANDARD_INPUT
	if (::System::Input::get()->has_key_been_pressed(::System::Key::B))
	{
		hard_copy_bullshot(true);
	}
	bool triggerBullshotTrap = false;
	if (::System::Input::get()->has_key_been_pressed(::System::Key::PageDown))
	{
		++bullshotTrapIdx;
		triggerBullshotTrap = true;
	}
	if (::System::Input::get()->has_key_been_pressed(::System::Key::PageUp))
	{
		bullshotTrapIdx = max(0, bullshotTrapIdx - 1);
		triggerBullshotTrap = true;
	}
	if (::System::Input::get()->has_key_been_pressed(::System::Key::Home))
	{
		bullshotTrapIdx = 0;
		triggerBullshotTrap = true;
	}
	if (triggerBullshotTrap)
	{
		Name trap = Name(String::printf(TXT("bullshot %i"), bullshotTrapIdx));
#ifdef AN_ALLOW_EXTENSIVE_LOGS
		output(TXT("[BULLSHOT] trigger game script trap \"%S\""), trap.to_char());
#endif
		Framework::GameScript::ScriptExecution::trigger_execution_trap(trap);
	}
	if (::System::Input::get()->has_key_been_pressed(::System::Key::Insert))
	{
		Framework::BullshotSystem::change_title(-1);
	}
	if (::System::Input::get()->has_key_been_pressed(::System::Key::Delete))
	{
		Framework::BullshotSystem::change_title(1);
	}
	if (::System::Input::get()->is_key_pressed(::System::Key::Insert) ||
		::System::Input::get()->is_key_pressed(::System::Key::Delete))
	{
		Framework::BullshotSystem::set_title_time(0.0f);
	}
#endif
#endif

#ifndef AN_DEVELOPMENT
#ifdef RENDER_SCENE_EXTERNAL_VIEW_SUPPORT
	if (onlyExternalRenderScene)
	{
		debugCameraMode = 0;
	}
#endif
	debugCameraMode = mod(debugCameraMode, 2);
#endif
#endif

#ifdef AN_DEVELOPMENT
	if (debugDrawCollision &&
		debugDrawCollisionMode &&
		debugTestCollisions)
	{
		debugTestCollisions = debugDrawCollisionMode;
		debugPanel->set_option(NAME(dboTestCollisions), debugTestCollisions);
	}
#endif

	// for debug this is ok
	// but now it resides in gameSceneLayers::World
	{
		Vector3 velocityLocal = Vector3::zero;
		Vector3 movementDirectionLocal = Vector3::zero;
		if (::System::Input::get()->is_grabbed())
		{
#ifdef AN_DEVELOPMENT_OR_PROFILER
			{
				if (::System::Input::get()->has_key_been_pressed(::System::Key::I) && debugCameraInRoom.get())
				{
					if (auto* objectType = Library::get_current()->get_object_types().find(Framework::LibraryName(String(TXT("weapon parts:gun")))))
					{
						Game::get_as<Game>()->add_async_world_job_top_priority(TXT("spawn test object"), [objectType, this]()
							{
								Framework::ScopedAutoActivationGroup saag(world);
								Framework::Object* object;
								Game::get_as<Game>()->perform_sync_world_job(TXT("spawn"), [&object, objectType, this]()
									{
										object = objectType->create(String(TXT("test object")));
										object->init(subWorld);
									});
								object->access_individual_random_generator() = Random::Generator();
								{
									DEFINE_STATIC_NAME(useRandomParts);
									object->access_variables().access<bool>(NAME(useRandomParts)) = true;
								}
								object->initialise_modules();
								Game::get()->perform_sync_world_job(TXT("place spawned"),
									[object, this]()
									{
										object->get_presence()->place_in_room(debugCameraInRoom.get(), debugCameraPlacement);
									});
							});
					}
				}
				if (allTimeControlsInput.has_button_been_pressed(NAME(debugStoreCameraPlacement)))
				{
					StoredDebugCameraPlacement dc;
					dc.placement = debugCameraPlacement;
					if (auto* r = debugCameraInRoom.get())
					{
						dc.openWorld.x = r->get_tags().get_tag_as_int(NAME(openWorldX));
						dc.openWorld.y = r->get_tags().get_tag_as_int(NAME(openWorldY));
						dc.cellRoomIdx = r->get_tags().get_tag_as_int(NAME(cellRoomIdx));
					}
					{	// always save a new one
						debugCameraPlacements.push_back(dc);
						save_debug_camera_placements();
						debugCameraPlacementIdx = debugCameraPlacements.get_size() - 1;
					}
				}
				if (allTimeControlsInput.has_button_been_pressed(NAME(debugNextStoredCameraPlacement)))
				{
					// hold both to repeat last one
					// hold grip to reset to 0
					// release both to cycle
					if (!allTimeControlsInput.is_button_pressed(NAME(debugRotationSlow)) ||
						!allTimeControlsInput.is_button_pressed(NAME(debugRotationSlowAlt)))
					{
						--debugCameraPlacementIdx;
						if (debugCameraPlacementIdx < 0)
						{
							debugCameraPlacementIdx = debugCameraPlacements.get_size() - 1;
						}
						if (allTimeControlsInput.is_button_pressed(NAME(debugRotationSlow)) &&
							!allTimeControlsInput.is_button_pressed(NAME(debugRotationSlowAlt)))
						{
							// force first one
							debugCameraPlacementIdx = 0;
						}
					}
					if (debugCameraPlacements.is_index_valid(debugCameraPlacementIdx))
					{
						auto dc = debugCameraPlacements[debugCameraPlacementIdx];
						debugCameraPlacement = dc.placement;
						for_every_ptr(r, world->get_rooms())
						{
							if (r->get_tags().has_tag(NAME(openWorldX)) &&
								r->get_tags().has_tag(NAME(openWorldY)) &&
								r->get_tags().has_tag(NAME(cellRoomIdx)))
							{
								int openWorldX = r->get_tags().get_tag_as_int(NAME(openWorldX));
								int openWorldY = r->get_tags().get_tag_as_int(NAME(openWorldY));
								int cellRoomIdx = r->get_tags().get_tag_as_int(NAME(cellRoomIdx));
								if (dc.openWorld.x == openWorldX &&
									dc.openWorld.y == openWorldY &&
									dc.cellRoomIdx == cellRoomIdx)
								{
									debugCameraInRoom = r;
								}
							}
						}
					}
				}
			}
#endif
			{
				Vector2 movementStick = allTimeControlsInput.get_stick(NAME(debugMovement));
				bool verticalMovement = allTimeControlsInput.is_button_pressed(NAME(debugMovementVerticalSwitch));
				// clamp to sane values
				movementStick.x = clamp(movementStick.x, -1.0f, 1.0f);
				movementStick.y = clamp(movementStick.y, -1.0f, 1.0f);
				velocityLocal.x += movementStick.x;
				movementDirectionLocal.x += movementStick.x;
				if (verticalMovement)
				{
					velocityLocal.z += movementStick.y;
					movementDirectionLocal.z += movementStick.y;
				}
				else
				{
					velocityLocal.y += movementStick.y;
					movementDirectionLocal.y += movementStick.y;
				}
			}
			// this is for debug functionality, those keys maybe should not be defined in game input definition
#ifdef AN_STANDARD_INPUT
			if (System::Input::get()->is_key_pressed(System::Key::Q))
			{
				velocityLocal.z -= 1.0f;
				movementDirectionLocal.z -= 1.0f;
			}
			if (System::Input::get()->is_key_pressed(System::Key::E))
			{
				velocityLocal.z += 1.0f;
				movementDirectionLocal.z += 1.0f;
			}
#endif
			DEFINE_STATIC_NAME(debugCameraSlow);
			DEFINE_STATIC_NAME(debugCameraFast);
			DEFINE_STATIC_NAME(debugCameraSpeedSwitch);
			if (allTimeControlsInput.is_button_pressed(NAME(debugCameraFast)))
			{
				velocityLocal *= 30.0f;
				if (allTimeControlsInput.is_button_pressed(NAME(debugCameraSpeedSwitch)))
				{
					velocityLocal *= 3.0f;
					if (allTimeControlsInput.is_button_pressed(NAME(debugCameraSlow)))
					{
						velocityLocal *= 10.0f;
					}
				}
			}
			else if (allTimeControlsInput.is_button_pressed(NAME(debugCameraSlow)))
			{
				velocityLocal *= 0.1f;
				if (allTimeControlsInput.is_button_pressed(NAME(debugCameraSpeedSwitch)))
				{
					velocityLocal *= 0.1f;
				}
			}
			else
			{
				velocityLocal *= 2.0f;
			}
			if (allTimeControlsInput.is_button_pressed(NAME(debugMovementSlow)))
			{
				velocityLocal *= 0.02f;
			}
			if (allTimeControlsInput.is_button_pressed(NAME(debugMovementSlowAlt)))
			{
				velocityLocal *= 0.02f;
			}
			if (allTimeControlsInput.is_button_pressed(NAME(debugMovementSlowAlt2)))
			{
				velocityLocal *= 0.02f;
			}

			velocityLocal *= debugCameraSpeed;
		}
		if (debugControlsMode && debugCameraMode)
		{
			{
#ifdef AN_DEVELOPMENT
				if (world)
				{
					world->multithread_check__set__accessing_rooms_doors(Framework::World::MultithreadCheck::OnlyRead);
				}
#endif

#ifdef AN_ALLOW_BULLSHOTS
				{
					Framework::Room* room = nullptr;
					bool forceReset = false;
					static bool forceResetOnce = true;
#ifdef AN_STANDARD_INPUT
					if (System::Input::get()->is_key_pressed(System::Key::R))
					{
						forceReset = true;
					}
#endif
					if (forceResetOnce)
					{
						forceReset = forceResetOnce;
						forceResetOnce = false;
					}
					if (forceReset)
					{
						resetDebugCamera = false;
					}
					Framework::BullshotSystem::get_camera(room, debugCameraPlacement, nonVRCameraFov, forceReset);
					if (room)
					{
						debugCameraInRoom = room;
					}
				}
#endif

				Transform newPlacement = debugCameraPlacement;
				newPlacement.set_translation(debugCameraPlacement.get_translation() + debugCameraPlacement.vector_to_world(velocityLocal * ::System::Core::get_raw_delta_time()));
				Transform intoRoomTransform;
				if (debugCameraInRoom.is_set())
				{
					if (debugCameraMode != 2) // for render offset keep it in same room
					{
						Framework::Room * room = debugCameraInRoom.get();
						// allow to go through closed doors etc
						if (debugCameraInRoom->move_through_doors(debugCameraPlacement, newPlacement, room, Framework::Room::MoveThroughDoorsFlag::ForceMoveThrough))
						{
							debugCameraPlacement = newPlacement;
							debugCameraInRoom = room;
						}
					}
				}
				else
				{
					// force reset to get camera in room
					resetDebugCamera = true;
				}
				debugCameraPlacement = newPlacement;
#ifdef AN_DEVELOPMENT
				if (world)
				{
					world->multithread_check__set__accessing_rooms_doors();
				}
#endif
			}
#ifdef AN_STANDARD_INPUT
			if (System::Input::get()->is_key_pressed(System::Key::R))
			{
#ifdef AN_ALLOW_BULLSHOTS
				if (!Framework::BullshotSystem::is_active())
#endif
				resetDebugCamera = true;
			}
#ifdef AN_ALLOW_BULLSHOTS
			if ((!Framework::BullshotSystem::is_active() && !System::Input::get()->is_key_pressed(System::Key::T)) ||
				(Framework::BullshotSystem::is_active() && System::Input::get()->is_key_pressed(System::Key::T)))
#else
			if (!System::Input::get()->is_key_pressed(System::Key::T))
#endif
#endif
			{
				Rotator3 resetOrientationABit = Rotator3::zero;
				float rollCurrent = Rotator3::normalise_axis(debugCameraPlacement.get_orientation().to_rotator().roll);
				float rollTarget = 0.0f;
				//if (rollCurrent > 90.0f) rollTarget = 180.0f;
				//if (rollCurrent < -90.0f) rollTarget = -180.0f;
				resetOrientationABit.roll = rollTarget - rollCurrent;
				debugCameraPlacement.set_orientation(debugCameraPlacement.get_orientation().rotate_by(resetOrientationABit.to_quat()));
			}
			if (::System::Input::get()->is_grabbed())
			{
				if (allTimeControlsInput.is_button_pressed(NAME(debugRequestInGameMenu)))
				{
					request_in_game_menu(true, false);
				}
			}
		}
	}
	//
	{
		float yawControl = 0.0f;
		float pitchControl = 0.0f;
		float rollControl = 0.0f;
#ifdef AN_STANDARD_INPUT
		if (::System::Input::get()->is_grabbed())
		{
			Vector2 mouseRotation = allTimeControlsInput.get_mouse(NAME(debugRotation));
			if (::System::Input::get()->is_mouse_button_pressed(1))
			{
				nonVRCameraFov += mouseRotation.y;
				nonVRCameraFov = clamp(nonVRCameraFov, 10.0f, 150.0f);
				if (::System::Input::get()->is_mouse_button_pressed(2))
				{
					nonVRCameraFov = 80.0f;
				}
			}
			else if (::System::Input::get()->is_mouse_button_pressed(2))
			{
				rollControl += mouseRotation.x;
			}
			else
			{
				yawControl += mouseRotation.x;
				pitchControl += mouseRotation.y;
			}
		}
#endif
		{
			Vector2 stickRotation = allTimeControlsInput.get_stick(NAME(debugRotation));
			float rotationSpeed = 180.0f * ::System::Core::get_raw_delta_time();
			if (::System::Input::get()->is_grabbed())
			{
				if (allTimeControlsInput.is_button_pressed(NAME(debugRotationSlow)))
				{
					rotationSpeed *= 0.1f;
				}
				if (allTimeControlsInput.is_button_pressed(NAME(debugRotationSlowAlt)))
				{
					rotationSpeed *= 0.1f;
				}
				if (allTimeControlsInput.is_button_pressed(NAME(debugRotationSlowAlt2)))
				{
					rotationSpeed *= 0.1f;
				}
			}
			yawControl += stickRotation.x * rotationSpeed;
			pitchControl += stickRotation.y * rotationSpeed;
		}
		//
		if (debugControlsMode && debugCameraMode)
		{
			debugCameraPlacement.set_orientation(debugCameraPlacement.get_orientation().rotate_by(Rotator3(pitchControl, yawControl, rollControl).to_quat()));
			debug_renderer_set_fov(nonVRCameraFov);
		}
#ifndef AN_CLANG
		else if (useForcedCamera)
		{
			debug_renderer_set_fov(forcedCameraFOV);
		}
#endif
	}

#ifdef AN_LOG_POST_PROCESS
#ifdef AN_LOG_POST_PROCESS_ALL_FRAMES
	Framework::PostProcessDebugSettings::log = true;
	Framework::PostProcessDebugSettings::logRT = false;
	Framework::PostProcessDebugSettings::logRTDetailed = false;
#else
	Framework::PostProcessDebugSettings::log = false;
	Framework::PostProcessDebugSettings::logRT = false;
	Framework::PostProcessDebugSettings::logRTDetailed = false;
	if (allTimeControlsInput.is_button_pressed(NAME(logPostProcess)))
	{
		output(TXT("LOG POST PROCESS FOR FRAME #%06i #--------------------------------------------------#"), ::System::Core::get_frame());
		Framework::PostProcessDebugSettings::log = true;
		if (allTimeControlsInput.is_button_pressed(NAME(logPostProcessDetailedSwitch)))
		{
			Framework::PostProcessDebugSettings::logRT = true;
			Framework::PostProcessDebugSettings::logRTDetailed = true;
		}
	}
#endif
#endif

	return debugControlsMode && debugCameraMode;
}

