#include "lhs_beAtRightPlace.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\screens\lhc_question.h"
#include "..\screens\lhc_messageSetBrowser.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_customDraw.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\gameSettings.h"
#include "..\..\..\library\library.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\..\framework\library\library.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//#define TEST_WITH_NO_VR

//

// global reference ids
DEFINE_STATIC_NAME_STR(barpAtRightPlace, TXT("hub; be at right place; at right place"));
DEFINE_STATIC_NAME_STR(barpTurnRight, TXT("hub; be at right place; turn right"));
DEFINE_STATIC_NAME_STR(barpTurnLeft, TXT("hub; be at right place; turn left"));
DEFINE_STATIC_NAME_STR(barpLower, TXT("hub; be at right place; lower"));
DEFINE_STATIC_NAME_STR(barpHigher, TXT("hub; be at right place; higher"));
DEFINE_STATIC_NAME_STR(barpMoveLeft, TXT("hub; be at right place; move left"));
DEFINE_STATIC_NAME_STR(barpMoveRight, TXT("hub; be at right place; move right"));
DEFINE_STATIC_NAME_STR(barpMoveForward, TXT("hub; be at right place; move forward"));
DEFINE_STATIC_NAME_STR(barpMoveBackward, TXT("hub; be at right place; move backward"));
DEFINE_STATIC_NAME_STR(barpUnknown, TXT("hub; be at right place; unknown"));

// localised strings
DEFINE_STATIC_NAME_STR(lsBack, TXT("hub; be at right place; back"));
DEFINE_STATIC_NAME_STR(lsAtRightPlace, TXT("hub; be at right place; at right place"));
DEFINE_STATIC_NAME_STR(lsTurnRight, TXT("hub; be at right place; turn right"));
DEFINE_STATIC_NAME_STR(lsTurnLeft, TXT("hub; be at right place; turn left"));
DEFINE_STATIC_NAME_STR(lsLower, TXT("hub; be at right place; lower"));
DEFINE_STATIC_NAME_STR(lsHigher, TXT("hub; be at right place; higher"));
DEFINE_STATIC_NAME_STR(lsMoveLeft, TXT("hub; be at right place; move left"));
DEFINE_STATIC_NAME_STR(lsMoveRight, TXT("hub; be at right place; move right"));
DEFINE_STATIC_NAME_STR(lsMoveForward, TXT("hub; be at right place; move forward"));
DEFINE_STATIC_NAME_STR(lsMoveBackward, TXT("hub; be at right place; move backward"));
DEFINE_STATIC_NAME_STR(lsUnknown, TXT("hub; be at right place; unknown"));
DEFINE_STATIC_NAME_STR(lsLoadingTips, TXT("hub; common; loading tips"));

// screens
DEFINE_STATIC_NAME(beAtRightPlace);
DEFINE_STATIC_NAME(beAtRightPlaceMessages);
DEFINE_STATIC_NAME(idBARPHelp);

// fade reasons
DEFINE_STATIC_NAME(backFromInGameMenu);

//

using namespace Loader;
using namespace HubScenes;

//

BeAtRightPlace::AtRightPlaceInfo::AtRightPlaceInfo(Name const& _globalReferenceId, Name const& _locStrId)
{
	texturePart = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(_globalReferenceId, true);
	locStrId = _locStrId;
}

//

BeAtRightPlace* BeAtRightPlace::s_current = nullptr;
Optional<Transform> BeAtRightPlace::beAt;
bool BeAtRightPlace::beAtAnyHeight = false;
Concurrency::SpinLock BeAtRightPlace::beAtLock = Concurrency::SpinLock(TXT("Loader.HubScenes.BeAtRightPlace.beAtLock"));
Optional<float> BeAtRightPlace::maxDist;

REGISTER_FOR_FAST_CAST(BeAtRightPlace);

void BeAtRightPlace::be_at(Optional<Transform> const& _beAt, bool _anyHeight)
{
	Concurrency::ScopedSpinLock lock(beAtLock);
	beAt = _beAt;
	beAtAnyHeight = _anyHeight;
}

void BeAtRightPlace::set_max_dist(Optional<float> const& _maxDist)
{
	maxDist = _maxDist;
}

BeAtRightPlace::BeAtRightPlace(bool _requiresToBeDeactivated, bool _testShowBoundary)
: testShowBoundary(_testShowBoundary)
, requiresToBeDeactivated(_requiresToBeDeactivated)
{
	an_assert(!s_current);
	s_current = this;
}

BeAtRightPlace::~BeAtRightPlace()
{
	an_assert(s_current == this);
	s_current = nullptr;
}

Optional<Transform> BeAtRightPlace::get_be_at(OPTIONAL_ OUT_ bool* _waitForBeAt, OPTIONAL_ OUT_ bool * _beAtAnyHeight, OPTIONAL_ OUT_ bool* _ignoreRotation)
{
	if (_waitForBeAt)
	{
		if (s_current)
		{
			*_waitForBeAt = s_current->waitForBeAt;
		}
		else
		{
			*_waitForBeAt = false;
		}
	}
	Concurrency::ScopedSpinLock lock(beAtLock);
	if (_beAtAnyHeight)
	{
		if (s_current)
		{
			*_beAtAnyHeight = beAtAnyHeight;
		}
		else
		{
			*_beAtAnyHeight = true;
		}
	}
	if (_ignoreRotation)
	{
		if (s_current)
		{
			*_ignoreRotation = s_current->ignoreRotation;
		}
		else
		{
			*_ignoreRotation = false;
		}
	}
	return s_current ? beAt : NP;
}

int BeAtRightPlace::iars_to_index(int _iars)
{
	if (_iars & AtRightPlace) return 0;
	if (_iars & TurnRight) return 1;
	if (_iars & TurnLeft) return 2;
	if (_iars & MoveLeft) return 5;
	if (_iars & MoveRight) return 6;
	if (_iars & MoveForward) return 7;
	if (_iars & MoveBackward) return 8;
	// at the end
	if (_iars & Lower) return 3;
	if (_iars & Higher) return 4;
	return 9;
}

bool BeAtRightPlace::is_required(Hub* _hub)
{
	return is_at_right_place(_hub) != AtRightPlace;
}

bool BeAtRightPlace::is_required() const
{
	return is_at_right_place(get_hub(), !ignoreRotation, waitForBeAt) != AtRightPlace;
}

Optional<Transform> BeAtRightPlace::get_start_movement_centre(Hub* _hub, Optional<bool> const& _waitForBeAt)
{
	{
		Concurrency::ScopedSpinLock lock(beAtLock);
		if (beAt.is_set())
		{
			return beAt.get();
		}
	}

	if (!_waitForBeAt.get(false) && _hub->get_start_movement_centre().is_set())
	{
		return _hub->get_start_movement_centre().get();
	}

	return NP;
}

int BeAtRightPlace::is_at_right_place(Hub* _hub, Optional<bool> const& _checkRotation, Optional<bool> const& _waitForBeAt, OPTIONAL_ OUT_ bool * _beaconRActive, OPTIONAL_ OUT_ float* _beaconZ, OPTIONAL_ OUT_ Optional<float>* _beAtExplitYaw)
{
#ifdef AN_DEVELOPMENT_OR_PROFILER
#ifdef TEST_WITH_NO_VR
	static bool returnLower = true;
	if (returnLower)
	{
		return Lower;
	}
#endif
	static bool returnUnknown = false;
	if (returnUnknown)
	{
		return Unknown;
	}
#endif
	if (TeaForGodEmperor::Game::is_using_sliding_locomotion())
	{
		// should be ignored at all
		return AtRightPlace;
	}
	assign_optional_out_param(_beaconRActive, false);
	assign_optional_out_param(_beaconZ, 1.6f);
	assign_optional_out_param(_beAtExplitYaw, NP);
	if (!VR::IVR::get())
	{
		return AtRightPlace;
	}
	if ((!_hub || !_hub->get_start_movement_centre().is_set()) && ! beAt.is_set() && !_waitForBeAt.get(false))
	{
		warn(TXT("no start movemen centre pose! assuming it is ok?"));
		return AtRightPlace;
	}
	if (auto* vr = VR::IVR::get())
	{
		if (vr->is_controls_view_valid())
		{
			Transform controlsMovementCentre = vr->get_controls_movement_centre();
			Optional<Transform> startMovementCentre = get_start_movement_centre(_hub, _waitForBeAt);
			
			if (!startMovementCentre.is_set())
			{
				return Unknown;
			}

			Transform controlsMovementCentreFlat(controlsMovementCentre.get_translation(), (controlsMovementCentre.get_orientation().to_rotator() * Rotator3(0.0f, 1.0f, 0.0f)).to_quat());
			Vector3 startCS = controlsMovementCentreFlat.location_to_local(startMovementCentre.get().get_translation());

			float const useMaxDist = maxDist.get(0.10f);
			float const useMaxYawDiff = 20.0f;
			float const useMaxHeightDiff = 0.10f;

			if (startCS.length_2d() > 0.5f)
			{
				float yaw = Rotator3::get_yaw(startCS);
				float threshold = 30.0f;
				if (yaw > threshold)
				{
					return TurnRight;
				}
				if (yaw < -threshold)
				{
					return TurnLeft;
				}
			}

			float beaconZ = controlsMovementCentre.get_translation().z;
			if (!beAtAnyHeight)
			{
				beaconZ = startMovementCentre.get().get_translation().z;
			}

			if (startCS.length_2d() < hardcoded 0.3f &&
				_checkRotation.get(true))
			{
				// if inside, show direction
				assign_optional_out_param(_beaconRActive, true);
				assign_optional_out_param(_beaconZ, beaconZ);

				// if we should check rotation, force the screen to appear at explicit yaw so we face it the right way
				if (_checkRotation.get(true) && _beAtExplitYaw)
				{
					Optional<float> yawDiff;
					// check orientation only if movement centre is looking rather at horizon
					Rotator3 startMovementCentreRot = startMovementCentre.get().get_orientation().to_rotator();
					Optional<float> beAtYaw;
					if (abs(startMovementCentreRot.pitch) < 60.0f)
					{
						Rotator3 startMovementCentreRotCS = controlsMovementCentreFlat.to_local(startMovementCentre.get()).get_orientation().to_rotator();
						beAtYaw = startMovementCentreRot.yaw;
						yawDiff = Rotator3::normalise_axis(startMovementCentreRotCS.yaw);
					}
					else
					{
						Vector3 fwdDir = startMovementCentre.get().vector_to_world(Vector3(0.0f, 1.0f, startMovementCentreRot.pitch > 0.0f ? -1.0f : 1.0f));
						Rotator3 fwdDirCS = controlsMovementCentreFlat.vector_to_local(fwdDir).to_rotator();
						beAtYaw = fwdDir.to_rotator().yaw;
						yawDiff = Rotator3::normalise_axis(fwdDirCS.yaw);
					}
					if (yawDiff.is_set() &&
						abs(yawDiff.get()) < 15.0f)
					{
						assign_optional_out_param(_beAtExplitYaw, beAtYaw);
					}
				}
			}

			{
				int result = 0;
				if (startCS.x > useMaxDist)
				{
					result |= MoveRight;
				}
				if (startCS.x < -useMaxDist)
				{
					result |= MoveLeft;
				}
				if (startCS.y > useMaxDist)
				{
					result |= MoveForward;
				}
				if (startCS.y < -useMaxDist)
				{
					result |= MoveBackward;
				}
				if (result != 0)
				{
					return result;
				}
			}

			if (_checkRotation.get(true))
			{
				// check orientation only if MovementCentre is looking rather at horizon
				Rotator3 startMovementCentreRot = startMovementCentre.get().get_orientation().to_rotator();
				if (abs(startMovementCentreRot.pitch) < 60.0f)
				{
					Rotator3 startMovementCentreRotCS = controlsMovementCentreFlat.to_local(startMovementCentre.get()).get_orientation().to_rotator();
					float yawDiff = Rotator3::normalise_axis(startMovementCentreRotCS.yaw);
					if (yawDiff > useMaxYawDiff)
					{
						return TurnRight;
					}
					if (yawDiff < -useMaxYawDiff)
					{
						return TurnLeft;
					}
				}
			}

			if (startCS.z < -useMaxHeightDiff && (! beAt.is_set() || ! beAtAnyHeight))
			{
				assign_optional_out_param(_beaconRActive, true);
				assign_optional_out_param(_beaconZ, beaconZ);

				return Lower;
			}

			return AtRightPlace;
		}
	}
	return Unknown;
}

void BeAtRightPlace::on_activate(HubScene* _prev)
{
	forceRightPlace = false;
	backRequested = false;

	if (!goBackToScene.is_set())
	{
		goBackToScene = _prev;
	}
	
	base::on_activate(_prev);

	atRightPlaceInfos.set_size(iars_to_index(Unknown) + 1);
	atRightPlaceInfos[iars_to_index(AtRightPlace)] = AtRightPlaceInfo(NAME(barpAtRightPlace), NAME(lsAtRightPlace));
	atRightPlaceInfos[iars_to_index(TurnRight)] = AtRightPlaceInfo(NAME(barpTurnRight), NAME(lsTurnRight));
	atRightPlaceInfos[iars_to_index(TurnLeft)] = AtRightPlaceInfo(NAME(barpTurnLeft), NAME(lsTurnLeft));
	atRightPlaceInfos[iars_to_index(Lower)] = AtRightPlaceInfo(NAME(barpLower), NAME(lsLower));
	atRightPlaceInfos[iars_to_index(Higher)] = AtRightPlaceInfo(NAME(barpHigher), NAME(lsHigher));
	atRightPlaceInfos[iars_to_index(MoveLeft)] = AtRightPlaceInfo(NAME(barpMoveLeft), NAME(lsMoveLeft));
	atRightPlaceInfos[iars_to_index(MoveRight)] = AtRightPlaceInfo(NAME(barpMoveRight), NAME(lsMoveRight));
	atRightPlaceInfos[iars_to_index(MoveForward)] = AtRightPlaceInfo(NAME(barpMoveForward), NAME(lsMoveForward));
	atRightPlaceInfos[iars_to_index(MoveBackward)] = AtRightPlaceInfo(NAME(barpMoveBackward), NAME(lsMoveBackward));
	atRightPlaceInfos[iars_to_index(Unknown)] = AtRightPlaceInfo(NAME(barpUnknown), NAME(lsUnknown));

	loaderDeactivated = false;
	isAtRightPlace = false;
	get_hub()->show_start_movement_centre(! testShowBoundary);
	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	show_info();
}

void BeAtRightPlace::on_deactivate(HubScene* _next)
{
	base::on_deactivate(_next);
	get_hub()->show_start_movement_centre(false);
	get_hub()->set_beacon_active(false);
	reset_be_at();
}

void BeAtRightPlace::on_loader_deactivate()
{
	base::on_loader_deactivate();

	loaderDeactivated = true;
}

void BeAtRightPlace::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	{
		highlightInfoPt += _deltaTime / highlightInfoPeriod;
		highlightInfoPt = mod(highlightInfoPt, 1.0f);
	}

	bool beaconRActive = false;
	float beaconZ = 1.6f;
	Optional<float> beAtExplicitYaw;
	int iars = is_at_right_place(get_hub(), !ignoreRotation, waitForBeAt, &beaconRActive, &beaconZ, &beAtExplicitYaw);

	if (forceRightPlace)
	{
		iars = AtRightPlace;
	}
	if (testShowBoundary)
	{
		iars = Unknown;
	}

	get_hub()->set_beacon_active(iars != AtRightPlace && iars != Unknown, beaconRActive, beaconZ);

	if (!ignoreRotation)
	{
		if (HubScreen* screen = get_hub()->get_screen(NAME(beAtRightPlace)))
		{
			screen->beAtYaw = beAtExplicitYaw;
		}
	}

	if (lastIARS != iars || iars == AtRightPlace)
	{
		lastIARS = iars;
		if (iars == AtRightPlace)
		{
			highlightInfoPt = 0.0f;
			if (auto* image = fast_cast<HubWidgets::Image>(imageWidget.get()))
			{
				image->texturePart = nullptr;
			}
			if (auto* text = fast_cast<HubWidgets::Text>(textWidget.get()))
			{
				/*
				if (requiresToBeDeactivated && !loaderDeactivated)
				{
					text->set(waitingMessage);
				}
				else
				*/
				{
					if (!text->get_text().is_empty())
					{
						text->clear();
					}
				}
			}
			isAtRightPlace = true;
			if (!requiresToBeDeactivated || loaderDeactivated)
			{
				get_hub()->deactivate_all_screens();
			}
		}
		else if (iars == Unknown)
		{
			isAtRightPlace = false;
			if (auto* image = fast_cast<HubWidgets::Image>(imageWidget.get()))
			{
				image->texturePart = nullptr;
			}
			if (auto* text = fast_cast<HubWidgets::Text>(textWidget.get()))
			{
				/*
				if (requiresToBeDeactivated && !loaderDeactivated)
				{
					text->set(waitingMessage);
				}
				else
				*/
				{
					if (!text->get_text().is_empty())
					{
						text->clear();
					}
				}
			}
		}
		else
		{
			isAtRightPlace = false;
			int idx = iars_to_index(iars);
			bool dontShowAnything = false;

			if (idx != 3 && idx != 4)
			{
				dontShowAnything = true;
			}

			if (dontShowAnything)
			{
				if (auto* image = fast_cast<HubWidgets::Image>(imageWidget.get()))
				{
					image->texturePart = nullptr;
				}
				if (auto* text = fast_cast<HubWidgets::Text>(textWidget.get()))
				{
					if (!text->get_text().is_empty())
					{
						text->clear();
					}
				}
			}
			else
			{
				if (auto* image = fast_cast<HubWidgets::Image>(imageWidget.get()))
				{
					image->texturePart = atRightPlaceInfos[idx].texturePart;
				}
				if (auto* text = fast_cast<HubWidgets::Text>(textWidget.get()))
				{
					text->set(atRightPlaceInfos[idx].locStrId);
				}
			}
		}
	}

	// update mesh? screen?
}

void BeAtRightPlace::show_info()
{
	get_hub()->keep_only_screen(NAME(beAtRightPlace));

	bool showTipsOnLoading = false;
#ifdef ALLOW_LOADING_TIPS_WITH_BE_AT_RIGHT_PLACE
	if (allowLoadingTips)
	{
		if (auto* psp = TeaForGodEmperor::PlayerSetup::get_current_if_exists())
		{
			showTipsOnLoading = psp->get_preferences().showTipsOnLoading;
		}
	}
#endif

	// add buttons: go back to game, exit to main menu, quit game
	HubScreen* screen = get_hub()->get_screen(NAME(beAtRightPlace));
	if (! screen)
	{
		screen = new HubScreen(get_hub(), NAME(beAtRightPlace), Vector2(40.0f, 40.0f), get_hub()->get_current_forward(), get_hub()->get_radius() * 0.7f, true, Rotator3(showTipsOnLoading? 10.0f : 0.0f, 0.0f, 0.0f));
		screen->followYawDeadZone = 15.0f;
		screen->followHead = true;
		screen->alwaysRender = true;

		Vector2 const fs = HubScreen::s_fontSizeInPixels;

		float const insideSpacing = 0.5f;
		float const spacingBetweenButtons = 0.2f;
		float const linePlusSpacing = 1.0f /* text */ + insideSpacing * 2.0f + spacingBetweenButtons;
		Range2 availableSpace = screen->mainResolutionInPixels;
		availableSpace.y.min += fs.y;
		if (goBackButton || !specialButtons.is_empty())
		{
			availableSpace.y.min += fs.y * (linePlusSpacing * (float)(max(1, specialButtons.get_size())) - spacingBetweenButtons);
		}
		availableSpace.y.max -= fs.y;
		availableSpace.x.expand_by(-max(fs.x, fs.y));

		float const messageFromBottom = fs.y * 1.8f;
		float const messageDistance = fs.y;

		{
			Range2 at = availableSpace;
			at.y.min += messageFromBottom;
			at.y.min += messageDistance;
			//at.y.max = at.y.centre(); // bottom half
			//at.y.min += fs.y;
			{
				Range2 tat = at;
				tat.y.min -= fs.y * 2.0f;
				tat.y.max -= fs.y * 2.0f;
				auto* w = new HubWidgets::Text(Name::invalid(), tat, String::empty());
				w->alignPt = Vector2(0.5f, 0.5f); // bottom
				textWidget = w;
			}
			{
				auto* w = new HubWidgets::Image(Name::invalid(), at, nullptr);
				w->alignPt = Vector2(0.5f, 0.5f);
				w->scale = Vector2(2.0f, 2.0f);
				imageWidget = w;
			}
		}

		{
			Range2 at = availableSpace;

			RefCountObjectPtr<HubScene> keepThis(this);
			Hub* hub = get_hub();
			auto* w = new HubWidgets::CustomDraw(NAME(beAtRightPlace), at, [hub, keepThis, this, messageFromBottom, messageDistance](Framework::Display* _display, Range2 const& _at)
			{
				float messageAtY = _at.y.min + messageFromBottom;

				{
					Range2 at = _at;

					at.y.min = messageAtY + messageDistance;

#ifdef AN_DEVELOPMENT_OR_PROFILER
					::System::Video3DPrimitives::rect_2d(Colour::white, at.get_at(Vector2(0.0f, 0.0f)), at.get_at(Vector2(1.0f, 1.0f)), false);
#endif

					if (auto* vr = VR::IVR::get())
					{
						auto const& mc = MainConfig::global();
						Vector3 mcManualOffset = mc.get_vr_map_space_offset();
						float mcManualRotate = mc.get_vr_map_space_rotate();
						//float borderAsFloat = (float)playAreaOptions.border / 100.0f;

						Transform mapVRSpace = vr->calculate_map_vr_space(mcManualOffset, mcManualRotate);

						float horizontalScaling = mc.should_be_immobile_vr()? 1.0f : mc.get_vr_horizontal_scaling();
						float invHorizontalScaling = horizontalScaling != 0.0f? 1.0f / horizontalScaling : 1.0f;

						auto const& rawBoundaryPoints = vr->get_raw_boundary();
						ARRAY_STACK(Vector2, boundaryPoints, max(4, rawBoundaryPoints.get_size()));
						for_every(rbp, rawBoundaryPoints)
						{
							boundaryPoints.push_back(mapVRSpace.location_to_local(rbp->to_vector3()).to_vector2());
						}

						Range2 bpRange = Range2::empty;
						for_every(bp, boundaryPoints)
						{
							bpRange.include(*bp);
						}

						if (bpRange.x.is_empty() || bpRange.y.is_empty())
						{
							Vector2 half = Vector2::zero;
							half.x = vr->get_whole_play_area_rect_half_right().length();
							half.y = vr->get_whole_play_area_rect_half_forward().length();

							boundaryPoints.clear();
							boundaryPoints.push_back(Vector2( half.x,  half.y));
							boundaryPoints.push_back(Vector2( half.x, -half.y));
							boundaryPoints.push_back(Vector2(-half.x, -half.y));
							boundaryPoints.push_back(Vector2(-half.x,  half.y));

							for_every(bp, boundaryPoints)
							{
								bpRange.include(*bp);
							}
						}

						{
							float dist = 0.08f;
							Vector2 offx = Vector2::xAxis * dist;
							Vector2 offy = Vector2::yAxis * dist;
							if (vr->is_controls_view_valid())
							{
								auto placement = vr->get_controls_movement_centre();
								Vector2 loc = placement.get_translation().to_vector2() * invHorizontalScaling;
								bpRange.include(loc + offx);
								bpRange.include(loc - offx);
								bpRange.include(loc + offy);
								bpRange.include(loc - offy);
							}
							{
								Optional<Transform> startMovementCentre = get_start_movement_centre(hub, waitForBeAt);
								if (startMovementCentre.is_set())
								{
									auto placement = startMovementCentre.get();
									Vector2 loc = placement.get_translation().to_vector2() * invHorizontalScaling;
									bpRange.include(loc + offx);
									bpRange.include(loc - offx);
									bpRange.include(loc + offy);
									bpRange.include(loc - offy);
								}
							}
						}

						ARRAY_STACK(Vector2, playAreaPoints, 4);

						{
							Vector2 half = Vector2::zero;
							half.x = vr->get_whole_play_area_rect_half_right().length();
							half.y = vr->get_whole_play_area_rect_half_forward().length();
							//half.x -= borderAsFloat;
							//half.y -= borderAsFloat;

							playAreaPoints.push_back(Vector2(-half.x, half.y));
							playAreaPoints.push_back(Vector2(half.x, half.y));
							playAreaPoints.push_back(Vector2(half.x, -half.y));
							playAreaPoints.push_back(Vector2(-half.x, -half.y));

							for_every(pap, playAreaPoints)
							{
								bpRange.include(*pap);
							}
						}

						if (!bpRange.x.is_empty() &&
							!bpRange.y.is_empty())
						{
							float scales = min(at.x.length() / bpRange.x.length(), at.y.length() / bpRange.y.length());
							Vector2 scale(scales, scales);
							Vector2 centre = bpRange.centre();

							if (testShowBoundary)
							{
								// draw test cross to show where play area is
								float crossHalfLength = 10.0f;
								float arrowLength = 1.0f;
								float arrowSide = 0.3f;
								float arrowSpacing = 0.1f;
								::System::Video3DPrimitives::line_2d(Colour::yellow, (Vector2::xAxis * crossHalfLength - centre) * scale + at.centre(), (Vector2(arrowSpacing, 0.0f) - centre) * scale + at.centre(), false);
								::System::Video3DPrimitives::line_2d(Colour::yellow, (-Vector2::xAxis * crossHalfLength - centre) * scale + at.centre(), (Vector2(-arrowSpacing, 0.0f) - centre) * scale + at.centre(), false);
								::System::Video3DPrimitives::line_2d(Colour::yellow, (-Vector2::yAxis * crossHalfLength - centre) * scale + at.centre(), (Vector2(0.0f, -arrowSpacing) - centre) * scale + at.centre(), false);
								::System::Video3DPrimitives::line_2d(Colour::yellow, (Vector2::yAxis * crossHalfLength - centre) * scale + at.centre(), (Vector2(0.0f, arrowLength + arrowSpacing) - centre) * scale + at.centre(), false);
								::System::Video3DPrimitives::line_2d(Colour::orange, (Vector2::zero - centre) * scale + at.centre(), (Vector2(0.0f, arrowLength) - centre) * scale + at.centre(), false);
								::System::Video3DPrimitives::line_2d(Colour::orange, (Vector2(arrowSide, arrowLength - arrowSide) - centre) * scale + at.centre(), (Vector2(0.0f, arrowLength) - centre) * scale + at.centre(), false);
								::System::Video3DPrimitives::line_2d(Colour::orange, (Vector2(-arrowSide, arrowLength - arrowSide) - centre) * scale + at.centre(), (Vector2(0.0f, arrowLength) - centre) * scale + at.centre(), false);
							}

							if (!boundaryPoints.is_empty())
							{
								Colour lineColour = Colour::blue;
								if (testShowBoundary)
								{
									lineColour = Colour::greyLight;
								}
								Vector2 lastPoint = boundaryPoints.get_last();
								for_every(point, boundaryPoints)
								{
									::System::Video3DPrimitives::line_2d(lineColour, (lastPoint - centre) * scale + at.centre(), (*point - centre) * scale + at.centre(), false);
									lastPoint = *point;
								}
							}

							{

								{
									Vector2 lastPoint = playAreaPoints.get_last();
									for_every(point, playAreaPoints)
									{
										::System::Video3DPrimitives::line_2d(Colour::lerp(0.5f, Colour::blue, Colour::cyan), (lastPoint - centre) * scale + at.centre(), (*point - centre) * scale + at.centre(), false);
										lastPoint = *point;
									}
								}
							}

							{
								for_count(int, locIdx, 2)
								{
									Transform placement = Transform::identity;
									if (locIdx == 1)
									{
										if (!vr->is_controls_view_valid())
										{
											continue;
										}
										auto mc = vr->get_controls_movement_centre();
										placement = mc;
									}
									else
									{
										if (testShowBoundary)
										{
											continue;
										}
										Optional<Transform> startMovementCentre = get_start_movement_centre(hub, waitForBeAt);
										if (!startMovementCentre.is_set())
										{
											continue;
										}
										placement = startMovementCentre.get();
									}

									Vector2 loc = placement.get_translation().to_vector2() * invHorizontalScaling;
									Vector2 fwd = placement.get_axis(Axis::Forward).normal_2d().to_vector2();
									Vector2 right = fwd.rotated_right();
									ARRAY_STACK(Vector2, placementPoints, 8);
									float placementScale = 0.22f;
									if (ignoreRotation && locIdx == 0)
									{
										Vector2 absfwd = Vector2::yAxis;
										for_count(int, i, placementPoints.get_max_size())
										{
											placementPoints.push_back(loc + absfwd.rotated_by_angle(360.0f * (float)i / (float)placementPoints.get_max_size()) * placementScale * 0.7f);
										}
									}
									else
									{
										placementPoints.push_back(loc + fwd * placementScale);
										placementPoints.push_back(loc - fwd * placementScale * 0.5f + right * placementScale * 0.8f);
										placementPoints.push_back(loc - fwd * placementScale * 0.3f);
										placementPoints.push_back(loc - fwd * placementScale * 0.5f - right * placementScale * 0.8f);
									}
									Vector2 lastPoint = placementPoints.get_last();
									for_every(point, placementPoints)
									{
										::System::Video3DPrimitives::line_2d(locIdx == 1? Colour::green : Colour::red, (lastPoint - centre) * scale + at.centre(), (*point - centre) * scale + at.centre(), false);
										lastPoint = *point;
									}
								}
							}
							{
								auto controlsPose = vr->get_controls_pose_set();
								for_count(int, handIdx, Hand::MAX)
								{
									auto& handOpt = controlsPose.hands[handIdx].placement;
									if (handOpt.is_set())
									{
										auto hand = handOpt.get();
										Vector2 loc = hand.get_translation().to_vector2() * invHorizontalScaling;
										float handScale = 0.07f;
										int const handPointsCount = 32;
										ARRAY_STACK(Vector2, handPoints, handPointsCount);
										for_count(int, i, handPointsCount)
										{
											handPoints.push_back(loc + Vector2::yAxis.rotated_by_angle(360.0f * (float)i / (float)handPointsCount) * handScale * 0.5f);
										}
										Vector2 lastPoint = handPoints.get_last();
										for_every(point, handPoints)
										{
											::System::Video3DPrimitives::line_2d(Colour::green, (lastPoint - centre) * scale + at.centre(), (*point - centre) * scale + at.centre(), false);
											lastPoint = *point;
										}
									}
								}
							}
						}
					}
				}

				struct ScaleUtil
				{
					static float calculate(Framework::Font* _font, String const& _text, Optional<float> const& _scale, float _availableWidth)
					{
						if (_scale.is_set())
						{
							return _scale.get();
						}
						float requiredWidth = _font->get_font()->calculate_text_size(_text).x;
						_availableWidth *= 0.99f;
						if (requiredWidth * 2.0f < _availableWidth)
						{
							return 2.0f;
						}
						else
						{
							return 1.0f;
						}
					}
				};
				if (requiresToBeDeactivated && !loaderDeactivated && !waitingMessage.is_empty() &&
					(lastIARS == AtRightPlace /* we need to wait for the level to end generating */ ||
					 lastIARS == Unknown /* we need to wait for the position to be set */))
				{
					if (auto* font = _display->get_font())
					{
						Colour textColour = HubColours::text();
						System::FontDrawingContext fdContext;
						Vector2 textAt(_at.x.centre(), messageAtY);
						Vector2 scale = ScaleUtil::calculate(font, waitingMessage, waitingMessageScale, _at.x.length()) * Vector2::one;
						font->draw_text_at(::System::Video3D::get(), waitingMessage, textColour, textAt, scale, Vector2(0.5f, 1.0f), NP, fdContext);
					}
				}
				else if (! infoMessage.is_empty() && lastIARS != Unknown)
				{
					if (auto* font = _display->get_font())
					{
						Colour textColour = HubColours::text();
						if (highlightInfoPt <= highlightInfoActivePt)
						{
							textColour = HubColours::text_bright();
						}
						System::FontDrawingContext fdContext;
						Vector2 textAt(_at.x.centre(), messageAtY);
						Vector2 scale = ScaleUtil::calculate(font, infoMessage, infoMessageScale, _at.x.length()) * Vector2::one;
						font->draw_text_at(::System::Video3D::get(), infoMessage, textColour, textAt, scale, Vector2(0.5f, 1.0f), NP, fdContext);
					}
				}
			});
			w->always_requires_rendering();

			screen->add_widget(w);
		}

		screen->add_widget(textWidget.get());
		screen->add_widget(imageWidget.get());

		screen->activate();
		get_hub()->add_screen(screen);

		if (! specialButtons.is_empty())
		{
			for_every(sp, specialButtons)
			{
				auto perform = sp->perform;
				screen->add_button_widget(HubScreenButtonInfo(Name::invalid(), sp->label, [this, perform]() { if (!perform()) forceRightPlace = true; }),
					Vector2(screen->mainResolutionInPixels.x.centre(), screen->mainResolutionInPixels.y.min + fs.y * (0.6f + linePlusSpacing * (float)for_everys_index(sp))),
					Vector2(0.5f, 0.0f), fs * Vector2(2.0f, insideSpacing));
			}
		}
		else if (goBackButton)
		{
			screen->add_button_widget(HubScreenButtonInfo(Name::invalid(), NAME(lsBack), [this]()
				{
					backRequested = true;
					if (goBackDo)
					{
						goBackDo();
					}
					else
					{
						go_back();
					}
				}),
				Vector2(screen->mainResolutionInPixels.x.centre(), screen->mainResolutionInPixels.y.min + fs.y * 0.6f),
				Vector2(0.5f, 0.0f), fs * Vector2(2.0f, insideSpacing));
		}

		if (helpMessageSet)
		{
			auto* b = screen->add_help_button();
			b->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
			{
				show_help();
			};
		}

	}

	if (showTipsOnLoading)
	{
		TagCondition loadingTipsCondition;
		loadingTipsCondition.load_from_string(String(TXT("loadingTips")));
	
		Array<TeaForGodEmperor::MessageSet*> messageSets;
		auto* library = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>();
		for_every_ptr(ms, library->get_message_sets().get_tagged(loadingTipsCondition))
		{
			messageSets.push_back(ms);
		}

		Vector2 ppa(24.0f, 24.0f);
		Vector2 size(40.0f, 27.0f);

		HubScreens::MessageSetBrowser::BrowseParams params;
		params.in_random_order(true);
		params.start_with_random_message(true);
		params.with_title(LOC_STR(NAME(lsLoadingTips)));
		params.cant_be_closed();
		auto* msb = HubScreens::MessageSetBrowser::browse(messageSets, params, get_hub(), NAME(beAtRightPlaceMessages) , false, size, screen->at * Rotator3(0.0f, 1.0f, 0.0f) + Rotator3(-25.0f, 0.0f, 0.0f), get_hub()->get_radius() * 0.6f, false, Rotator3(-30.0f, 0.0f, 0.0f), ppa, 1.0f, Vector2(0.0f, 1.0f));
		msb->followScreen = screen->id;
		msb->followHead = true;
		msb->alwaysProcessInput = true;
	}
}

void BeAtRightPlace::go_back()
{
	an_assert(goBackToScene.is_set());
	get_hub()->set_scene(goBackToScene.get());
}

void BeAtRightPlace::show_help()
{
	if (HubScreen* screen = get_hub()->get_screen(NAME(beAtRightPlace)))
	{
		if (helpMessageSet)
		{
			Vector2 ppa(20.0f, 20.0f);
			Vector2 size(40.0f, 20.0f);

			Rotator3 hAt = screen->at;
			HubScreens::MessageSetBrowser* msb = HubScreens::MessageSetBrowser::browse(helpMessageSet, HubScreens::MessageSetBrowser::BrowseParams(), get_hub(), NAME(idBARPHelp), true, size, hAt, screen->radius, true, NP, ppa, 1.0f, Vector2(0.0f, 1.0f));
			msb->dont_follow();
		}
	}
}
