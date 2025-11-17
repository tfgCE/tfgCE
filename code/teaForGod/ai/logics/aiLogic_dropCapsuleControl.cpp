#include "aiLogic_dropCapsuleControl.h"

#include "utils\aiLogicUtil_deviceDisplayStart.h"

#include "..\..\teaForGodTest.h"
#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\mc_emissiveControl.h"
#include "..\..\modules\custom\mc_itemHolder.h"
#include "..\..\pilgrimage\pilgrimageInstanceOpenWorld.h"

#include "..\..\..\core\random\randomUtils.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\appearance\controllers\ac_particlesUtils.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\library\library.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\moduleAppearance.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\actor.h"

#ifdef AN_DEVELOPMENT
#include "..\..\..\core\system\input.h"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#define DIAL_AMOUNT 6

#define ALLOW_SELECTING_CHASSIS
#ifdef AN_ALLOW_EXTENSIVE_LOGS
#endif

//#define MEASURE_SHOW_INFO_SIZES 20
//#define TEST_SHOW_INFO_SIZES

#ifndef MEASURE_SHOW_INFO_SIZES
	#define SCALE_SHOW_INFO
#else
	#ifdef TEST_SHOW_INFO_SIZES
		#define SCALE_SHOW_INFO
	#endif
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// messages
DEFINE_STATIC_NAME_STR(aim_dcc_startUp, TXT("drop capsule control; start up"));
DEFINE_STATIC_NAME_STR(aim_dcc_requiresCalibration, TXT("drop capsule control; requires calibration"));
DEFINE_STATIC_NAME_STR(aim_dcc_calibrated, TXT("drop capsule control; calibrated"));
DEFINE_STATIC_NAME_STR(aim_dcc_launch, TXT("drop capsule control; launch"));
DEFINE_STATIC_NAME_STR(aim_dcc_allowLaunch, TXT("drop capsule control; allow launch"));
DEFINE_STATIC_NAME_STR(aim_dcc_disallowLaunch, TXT("drop capsule control; disallow launch"));
DEFINE_STATIC_NAME_STR(aim_dcc_setTimeToHit, TXT("drop capsule control; set time to hit"));
DEFINE_STATIC_NAME_STR(aim_dcc_crashed, TXT("drop capsule control; crashed"));
DEFINE_STATIC_NAME_STR(aim_dcc_docked, TXT("drop capsule control; docked"));
DEFINE_STATIC_NAME(timeToHit);

// variables
DEFINE_STATIC_NAME(dropCapsuleControlId);
DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(dropCapsuleControlStartUpSequenceIdx);

// tags
DEFINE_STATIC_NAME(dropCapsuleLaunchControl);

// emissives
DEFINE_STATIC_NAME(busy);
DEFINE_STATIC_NAME(bad);
DEFINE_STATIC_NAME(operating);

// game script traps
DEFINE_STATIC_NAME_STR(gst_dcc_operational, TXT("drop capsule control; operational"));
DEFINE_STATIC_NAME_STR(gst_dcc_calibrated, TXT("drop capsule control; calibrated"));
DEFINE_STATIC_NAME_STR(gst_dcc_readyToLaunch, TXT("drop capsule control; ready to launch"));
DEFINE_STATIC_NAME_STR(gst_dcc_crashed, TXT("drop capsule control; crashed"));

// sounds
DEFINE_STATIC_NAME(calibrated);
DEFINE_STATIC_NAME_STR(calibratedAll, TXT("calibrated all"));
DEFINE_STATIC_NAME_STR(requiresCalibration, TXT("requires calibration"));
DEFINE_STATIC_NAME_STR(proximityWarning2s, TXT("proximity warning 2s"));
DEFINE_STATIC_NAME(launch);
DEFINE_STATIC_NAME(startup);
DEFINE_STATIC_NAME(operational);
DEFINE_STATIC_NAME_STR(engineInside, TXT("engine inside"));
DEFINE_STATIC_NAME_STR(crashIntoShip, TXT("crash into ship"));

//

static void draw_line(Colour const & _colour, Vector2 _a, Vector2 _b)
{
	::System::Video3DPrimitives::line_2d(_colour, _a, _b);
}

//

REGISTER_FOR_FAST_CAST(DropCapsuleControl);

DropCapsuleControl::DropCapsuleControl(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	dropCapsuleControlData = fast_cast<DropCapsuleControlData>(_logicData);
	executionData = new ExecutionData();
	executionData->display.otherBar.set_size(5);
	executionData->display.otherBarTarget.set_size(executionData->display.otherBar.get_size());
}

DropCapsuleControl::~DropCapsuleControl()
{
	if (auto* imo = get_imo())
	{
		if (display && imo)
		{
			display->use_background(nullptr);
			display->set_on_update_display(this, nullptr);
		}
	}
}

void DropCapsuleControl::play_sound(Name const& _sound)
{
	if (auto* imo = get_imo())
	{
		if (auto* s = imo->get_sound())
		{
			s->play_sound(_sound);
		}
	}
}

void DropCapsuleControl::stop_sound(Name const& _sound)
{
	if (auto* imo = get_imo())
	{
		if (auto* s = imo->get_sound())
		{
			s->stop_sound(_sound);
		}
	}
}

void DropCapsuleControl::draw_line_model(Framework::Display* _display, ::System::Video3D* _v3d, LineModel* _lm, Vector2 const & _atPt, float _relScale, bool _allowColourOverride)
{
	if (auto *lm = _lm)
	{ 
		Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
		Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

		Vector2 at = Vector2(lerp(_atPt.x, bl.x, tr.x), lerp(_atPt.y, bl.y, tr.y));
		at = round(at) - Vector2::half;
		float scale = max(tr.x - bl.x, tr.y - bl.y) * _relScale;

		Colour ink = _display->get_current_ink();
		Optional<Colour> overrideColour;
		if (executionData->currentState == DropCapsuleControlState::Crashed &&
			mod(executionData->inStateTime, 1.0f) < 0.75f)
		{
			overrideColour = Colour::red;
		}
		for_every(l, lm->get_lines())
		{
			draw_line(overrideColour.get(l->colour.get(ink)),
				TypeConversions::Normal::f_i_cells(at + scale * l->a.to_vector2()).to_vector2() + Vector2::half,
				TypeConversions::Normal::f_i_cells(at + scale * l->b.to_vector2()).to_vector2() + Vector2::half);
		}
	}
}

void DropCapsuleControl::draw_controls(Framework::Display* _display, ::System::Video3D* _v3d)
{
	struct DrawBar
	{
		static void draw(int _where, float _pt, Colour const& _c, Framework::Display* _display, ::System::Video3D* _v3d)
		{
			_pt = clamp(_pt, 0.0f, 1.0f);

			Vector2 bl = _display->get_left_bottom_of_screen() + Vector2::half;
			Vector2 tr = _display->get_right_top_of_screen() - Vector2::half;

			float width = 2.0f;
			float sep = 2.0f;
			Range2 r;
			if (_where >= 0)
			{
				r.x.min = bl.x;
				r.x.max = r.x.min + width;
				for (int i = 1; i < _where; ++i)
				{
					r.x.min += width + sep;
					r.x.max += width + sep;
				}
			}
			else
			{
				r.x.max = tr.x;
				r.x.min = r.x.max - width;
				for(int i = 1; i < -_where; ++ i)
				{
					r.x.min -= width + sep;
					r.x.max -= width + sep;
				}
			}
			r.y.min = bl.y;
			r.y.max = lerp(_pt, r.y.min, tr.y);

			::System::Video3DPrimitives::fill_rect_2d(_c, r, false);
		}
	};

	Colour colourOk = _display->get_current_ink();
	Colour colourBad = Colour::red;
	Colour colourBusy = Colour::blue;

	Colour otherBarColour = colourOk;
	Colour statusColour = colourOk;
	if (executionData->currentState == DropCapsuleControlState::Crashed)
	{
		otherBarColour = mod(executionData->inStateTime, 1.0f) < 0.75f ? colourBad : _display->get_current_paper();
		statusColour = otherBarColour;
	}
	if (executionData->currentState == DropCapsuleControlState::RequiresCalibration)
	{
		otherBarColour = colourBusy;
	}

	DrawBar::draw(1, executionData->display.otherBar[0], otherBarColour, _display, _v3d);
	DrawBar::draw(2, executionData->display.otherBar[1], otherBarColour, _display, _v3d);
	DrawBar::draw(3, executionData->display.otherBar[2], otherBarColour, _display, _v3d);
	DrawBar::draw(-1, executionData->display.otherBar[3], otherBarColour, _display, _v3d);
	DrawBar::draw(-2, executionData->display.otherBar[4], otherBarColour, _display, _v3d);
	DrawBar::draw(-3, executionData->display.velocityBar, statusColour, _display, _v3d);
	DrawBar::draw(-4, executionData->display.accelerationBar, statusColour, _display, _v3d);
	if (executionData->display.proximityBar > 0.0f)
	{
		Colour proximityBarColour = executionData->timeToHit.is_set() && executionData->timeToHit.get() <= 2.0f && mod(executionData->inStateTime, 0.5f) > executionData->display.proximityBar * 0.7f? colourBad : colourOk;
		DrawBar::draw(4, executionData->display.proximityBar, proximityBarColour, _display, _v3d);
	}
}

void DropCapsuleControl::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	// wait till anything available
	if (executionData->interactiveControls.is_empty())
	{
		return;
	}

	bool updateBarsOnce = false;

	if (display && !displaySetup)
	{
		displaySetup = true;
		display->draw_all_commands_immediately();
		display->set_on_update_display(this,
			[this](Framework::Display* _display)
			{
				if (!executionData->display.redrawNow)
				{
					return;
				}
				executionData->display.redrawNow = false;

				_display->drop_all_draw_commands();
				//if (state != Start)
				//{
				//	_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
				//}
				_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
				{
					_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
						[this](Framework::Display* _display, ::System::Video3D* _v3d)
						{
							executionData->display.redrawNow = false;
							if (executionData->currentState == DropCapsuleControlState::StartUp)
							{
								if (executionData->display.inStateDrawnFrames == 0) _display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
								executionData->display.timeToRedraw = 0.06f;
								int frames = executionData->display.inStateDrawnFrames;
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[frames](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										int const minf = 10;
										if (frames >= minf)
										{
											Utils::draw_device_display_start(_display, _v3d, frames - minf);
										}
									}));
							}
							if (executionData->currentState == DropCapsuleControlState::Operational ||
								executionData->currentState == DropCapsuleControlState::RequiresCalibration || 
								executionData->currentState == DropCapsuleControlState::Calibrated ||
								executionData->currentState == DropCapsuleControlState::ReadyToLaunch ||
								executionData->currentState == DropCapsuleControlState::Launched ||
								executionData->currentState == DropCapsuleControlState::Crashed ||
								executionData->currentState == DropCapsuleControlState::Docked)
							{
								bool justHit = false;
								if (executionData->currentState == DropCapsuleControlState::Crashed &&
									executionData->inStateTime < 1.0f)
								{
									_display->add((new Framework::DisplayDrawCommands::CLS(Colour::red.mul_rgb(0.5f))));
									justHit = true;
								}
								else
								{
									_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
								}
								executionData->display.timeToRedraw = 0.06f;
								_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
									[this, justHit](Framework::Display* _display, ::System::Video3D* _v3d)
									{
										if (dropCapsuleControlData)
										{
											auto* dclm = dropCapsuleControlData->dropCapsuleLineModel.get();
											auto* olm = dropCapsuleControlData->obstacleLineModel.get();

											if ((executionData->timeToHit.is_set() && executionData->timeToHit.get() > 0.0f) || justHit)
											{
												draw_line_model(_display, _v3d, dclm, Vector2(0.5f, 0.3f), 0.6f);
												if (mod(executionData->inStateTime, 0.5f) < 0.4f || justHit)
												{
													draw_line_model(_display, _v3d, olm, Vector2(0.5f, 0.8f), 0.6f, false);
												}
											}
											else
											{
												draw_line_model(_display, _v3d, dclm, Vector2::half, 1.0f);
											}
										}
										draw_controls(_display, _v3d);
									}));
							}
						}));
				}
			});
	}

	// advance controls
	{
		executionData->launchControl.buttonHandler.advance(_deltaTime);
		executionData->launchControl.dialHandler.advance(_deltaTime);
		executionData->launchControl.switchHandler.advance(_deltaTime);
		for_every(ic, executionData->interactiveControls)
		{
			ic->buttonHandler.advance(_deltaTime);
			ic->dialHandler.advance(_deltaTime);
			ic->switchHandler.advance(_deltaTime);
		}
	}

	// advance time
	float prevInStateTime = executionData->inStateTime;
	{
		executionData->inStateTime += _deltaTime;
		executionData->display.timeSinceLastDraw += _deltaTime;
		if (executionData->display.timeSinceLastDraw > executionData->display.timeToRedraw)
		{
			executionData->display.redrawNow = true;
			if (executionData->display.timeToRedraw > 0.0f)
			{
				executionData->display.timeSinceLastDraw = mod(executionData->display.timeSinceLastDraw, executionData->display.timeToRedraw);
			}
			else
			{
				executionData->display.timeSinceLastDraw = 0.0f;
			}
		}

		if (executionData->display.redrawNow)
		{
			++executionData->display.inStateDrawnFrames;
		}
	}

	if (executionData->currentState !=
		executionData->requestedState)
	{
		auto prevState = executionData->currentState;
		executionData->currentState = executionData->requestedState;
		executionData->inStateTime = 0.0f;
		executionData->customStateValue.clear();
		executionData->display.redrawNow = true;
		executionData->display.inStateDrawnFrames = 0;
		executionData->display.timeSinceLastDraw = 0.0f;

		if (executionData->currentState == DropCapsuleControlState::Inactive ||
			executionData->currentState == DropCapsuleControlState::StartUp)
		{
			for_every(ic, executionData->interactiveControls)
			{
				ic->requestedEmissive = Name::invalid();
			}
			executionData->display.otherBar[0] = 0.0f;
			executionData->display.otherBar[1] = 0.0f;
			executionData->display.otherBar[2] = 0.0f;
		}

		if (executionData->currentState == DropCapsuleControlState::Operational)
		{
			play_sound(NAME(operational));
			for_every(ic, executionData->interactiveControls)
			{
				ic->requestedEmissive = NAME(operating);
			}
		}

		if (executionData->currentState == DropCapsuleControlState::RequiresCalibration)
		{
			play_sound(NAME(requiresCalibration));

			executionData->calibrationPending = true;

			// clear required calibration
			for_every(ic, executionData->interactiveControls)
			{
				ic->requiresCalibration = false;
			}

			// mark to calibrate
			{
				int left = executionData->interactiveControls.get_size();
				int amountToCalibrate = min(left * 2 / 3, left);
				executionData->calibrationPendingCount = amountToCalibrate;
				while (amountToCalibrate > 0 && left > 0)
				{
					int idx = rg.get_int(left);
					auto* ic = executionData->interactiveControls.begin();
					while (idx > 0)
					{
						if (ic->requiresCalibration)
						{
							++ic;
							continue;
						}
						++ic;
						--idx;
					}
					ic->requiresCalibration = true;
					--left;
					--amountToCalibrate;
				}
			}

			// setup calibration
			for_every(ic, executionData->interactiveControls)
			{
				if (ic->requiresCalibration)
				{
					ic->calibratedOk = false;
					if (ic->buttonHandler.is_valid())
					{
						ic->calibratedAt = 1;
					}
					if (ic->dialHandler.is_valid())
					{
						ic->dialHandler.reset_dial_zero(0);
						ic->calibratedAt = mod(1 + rg.get_int(DIAL_AMOUNT - 1), DIAL_AMOUNT);
					}
					if (ic->switchHandler.is_valid())
					{
						int pos = TypeConversions::Normal::f_i_closest(ic->switchHandler.get_output());
						pos += 1; // from -1;1 to 0;2
						pos = mod(pos + (rg.get_chance(0.5f) ? 1 : -1), 3); // move in one dir
						pos -= 1; // fom 0;2 to -1;1
						ic->calibratedAt = pos;
					}
					ic->requestedEmissive = NAME(bad);
				}
				else
				{
					ic->requestedEmissive = NAME(operating);
				}
			}
		}

		if (executionData->currentState == DropCapsuleControlState::Calibrated)
		{
			executionData->calibrationPending = false;
			executionData->calibrationPendingCount = 0;

			// setup calibration
			for_every(ic, executionData->interactiveControls)
			{
				if (ic->requiresCalibration)
				{
					ic->requestedEmissive = NAME(busy);
				}
				else
				{
					ic->requestedEmissive = NAME(operating);
				}
			}
		}

		if (executionData->currentState == DropCapsuleControlState::Crashed)
		{
			updateBarsOnce = prevState == DropCapsuleControlState::Inactive;
			stop_sound(NAME(engineInside));
			play_sound(NAME(crashIntoShip));
			// setup calibration
			for_every(ic, executionData->interactiveControls)
			{
				ic->inoperable = rg.get_chance(0.4f);
				ic->crashedTimeLeft = rg.get_float(0.2f, 5.0f);
				if (ic->inoperable)
				{
					ic->requestedEmissive = Name::invalid();
				}
				else
				{
					ic->requestedEmissive = NAME(bad);
				}
			}
		}

		if (executionData->currentState == DropCapsuleControlState::Docked)
		{
			updateBarsOnce = prevState == DropCapsuleControlState::Inactive;
			stop_sound(NAME(engineInside));
			// setup calibration
			for_every(ic, executionData->interactiveControls)
			{
				ic->requestedEmissive = Name::invalid();
			}
		}

		if (executionData->currentState == DropCapsuleControlState::Launched)
		{
			play_sound(NAME(launch));
			play_sound(NAME(engineInside));
		}

		if (executionData->currentState == DropCapsuleControlState::ReadyToLaunch ||
			executionData->currentState == DropCapsuleControlState::Launched)
		{
			// setup calibration
			for_every(ic, executionData->interactiveControls)
			{
				ic->requestedEmissive = NAME(operating);
			}
		}
	}

	if (executionData->currentState == DropCapsuleControlState::StartUp)
	{
		int seqCount = executionData->dropCapsuleControlStartUpSequences;
		int atIdx = TypeConversions::Normal::f_i_cells(executionData->inStateTime / 0.08f);
		int reverseAt = seqCount + 5;
		if (!executionData->customStateValue.is_set() ||
			executionData->customStateValue.get() != atIdx)
		{
			if (atIdx == 0/* ||
				atIdx == reverseAt*/)
			{
				play_sound(NAME(startup));
			}
		}
		executionData->customStateValue = atIdx;
		if (atIdx < reverseAt)
		{
			for_every(ic, executionData->interactiveControls)
			{
				ic->requestedEmissive = atIdx == (seqCount - 1 - ic->dropCapsuleControlStartUpSequenceIdx) ? NAME(operating) : Name::invalid();
			}
		}
		else
		{
			for_every(ic, executionData->interactiveControls)
			{
				ic->requestedEmissive = atIdx - reverseAt >= ic->dropCapsuleControlStartUpSequenceIdx ? NAME(operating) : Name::invalid();
			}
		}
		if (atIdx > reverseAt + seqCount + 2)
		{
			executionData->requestedState = DropCapsuleControlState::Operational;
			Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gst_dcc_operational));
		}
	}

	// update requested emissives for those who require calibration
	if (executionData->currentState == DropCapsuleControlState::RequiresCalibration)
	{
		executionData->display.otherBarTimeLeft = 1000.0f; // stop for a moment
		executionData->calibrationPending = false;
		executionData->calibrationPendingCount = 0;
		for_every(ic, executionData->interactiveControls)
		{
			if (ic->requiresCalibration)
			{
				// apply controls
				{
					bool newCalibratedOk = ic->calibratedOk;
					if (ic->buttonHandler.is_valid())
					{
						if (ic->buttonHandler.has_button_been_pressed())
						{
							newCalibratedOk = !ic->calibratedOk;
						}
					}
					if (ic->dialHandler.is_valid())
					{
						int dat = mod(ic->dialHandler.get_dial_at(), DIAL_AMOUNT);
						newCalibratedOk = dat == ic->calibratedAt;
					}
					if (ic->switchHandler.is_valid())
					{
						int pos = TypeConversions::Normal::f_i_closest(ic->switchHandler.get_output());
						newCalibratedOk = pos == ic->calibratedAt;
					}
					if (ic->calibratedOk != newCalibratedOk)
					{
						// to change setup
						executionData->display.otherBarTimeLeft = 0.0f;
						if (newCalibratedOk)
						{
							play_sound(NAME(calibrated));
						}
					}
					ic->calibratedOk = newCalibratedOk;
				}
				if (ic->calibratedOk)
				{
					ic->requestedEmissive = NAME(busy);
				}
				else
				{
					executionData->calibrationPending = true;
					++executionData->calibrationPendingCount;
					ic->requestedEmissive = mod(executionData->inStateTime, 1.0f) < 0.75f ? NAME(bad) : Name::invalid();
				}
			}
		}

		if (!executionData->calibrationPending)
		{
			executionData->requestedState = DropCapsuleControlState::Calibrated;
			Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gst_dcc_calibrated));
		}
	}

	if (executionData->currentState == DropCapsuleControlState::Calibrated)
	{
		float playAt = 0.15f;
		if (executionData->inStateTime > playAt &&
			prevInStateTime <= playAt)
		{
			play_sound(NAME(calibratedAll));
		}
	}

	if (executionData->currentState == DropCapsuleControlState::Launched)
	{
		if (executionData->timeToHit.is_set() && executionData->timeToHit.get() > 0.0f)
		{
			float prevTimeToHit = executionData->timeToHit.get();
			executionData->timeToHit = max(0.0f, executionData->timeToHit.get() - _deltaTime);
			if (executionData->timeToHit.get() <= 2.0f && prevTimeToHit > 2.0f)
			{
				play_sound(NAME(proximityWarning2s));
			}
			if (executionData->timeToHit.get() <= 0.0f)
			{
				executionData->requestedState = DropCapsuleControlState::Crashed;

				Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gst_dcc_crashed));
			}
		}
	}

	if (executionData->currentState == DropCapsuleControlState::Crashed)
	{
		// setup calibration
		for_every(ic, executionData->interactiveControls)
		{
			ic->crashedTimeLeft -= _deltaTime;
			if (ic->crashedTimeLeft < -0.1f)
			{
				ic->crashedTimeLeft = rg.get_float(0.2f, 5.0f);
			}
			if (ic->inoperable || ic->crashedTimeLeft < 0.0f)
			{
				ic->requestedEmissive = Name::invalid();
			}
			else
			{
				ic->requestedEmissive = NAME(bad);
			}
		}
	}

	if (executionData->currentState == DropCapsuleControlState::Docked)
	{
		for_every(ic, executionData->interactiveControls)
		{
			ic->requestedEmissive = Name::invalid();
		}
	}

	//

	{
		executionData->launchControl.requestedEmissive = executionData->launchAllowed? (mod(executionData->inStateTime, 1.0f) < 0.75f ? NAME(bad) : Name::invalid()) : Name::invalid();

		if (executionData->launchAllowed)
		{
			float const onAt = 0.95f;
			if (executionData->launchControl.buttonHandler.has_button_been_pressed() ||
				(executionData->launchControl.switchHandler.get_switch_at() >= onAt && executionData->launchControl.switchHandler.get_prev_switch_at() < onAt))
			{
				executionData->requestedState = DropCapsuleControlState::ReadyToLaunch;

				Framework::GameScript::ScriptExecution::trigger_execution_trap(NAME(gst_dcc_readyToLaunch));
			}
		}
	}

	// bars
	{
		if (executionData->currentState == DropCapsuleControlState::Launched)
		{
			executionData->display.otherBarTimeLeft = min(0.1f, executionData->display.otherBarTimeLeft);
		}
		executionData->display.otherBarTimeLeft -= _deltaTime;
		if (executionData->display.otherBarTimeLeft < 0.0f || updateBarsOnce)
		{
			executionData->display.otherBarTimeLeft = rg.get_float(0.05f, 1.5f);
			for_every(obt, executionData->display.otherBarTarget)
			{
				*obt = rg.get_float(0.1f, 1.0f);
			}
		}
		if (executionData->currentState != DropCapsuleControlState::Crashed || updateBarsOnce)
		{
			float blendTimes[] = { 0.2f, 0.6f, 0.35f, 0.25f, 0.4f, 0.15f };
			int blendTimesCount = 5;
			for_count(int, i, executionData->display.otherBar.get_size())
			{
				executionData->display.otherBar[i] = blend_to_using_speed_based_on_time(executionData->display.otherBar[i], executionData->display.otherBarTarget[i], blendTimes[min(blendTimesCount - 1, i)], _deltaTime);
				if (updateBarsOnce)
				{
					executionData->display.otherBar[i] = executionData->display.otherBarTarget[i];
				}
			}
			float targetAccelerationBar = executionData->currentState == DropCapsuleControlState::Launched? 1.0f : 0.0f;
			executionData->display.accelerationBar = blend_to_using_speed_based_on_time(executionData->display.accelerationBar, targetAccelerationBar, 0.3f, _deltaTime);
			if (updateBarsOnce)
			{
				executionData->display.accelerationBar = 1.0f;
				executionData->display.velocityBar = 0.7f;
			}
			if (executionData->currentState == DropCapsuleControlState::Launched)
			{
				executionData->display.velocityBar += _deltaTime * 0.07f;
			}
			if (executionData->timeToHit.is_set() && executionData->timeToHit.get() > 0.0f)
			{
				executionData->display.proximityBar = clamp(executionData->timeToHit.get() / 6.0f, 0.0f, 1.0f);
				executionData->display.proximityBar = sqrt(executionData->display.proximityBar);
			}
			else
			{
				executionData->display.proximityBar = 0.0f;
			}
		}
	}

	//

	// update actual emissives
	{
		struct ApplyEmissive
		{
			static void to(Name const& _emissive, Array<SafePtr<Framework::IModulesOwner>> const& imos)
			{
				for_every_ref(imo, imos)
				{
					if (auto* e = imo->get_custom<CustomModules::EmissiveControl>())
					{
						e->emissive_deactivate_all();
						if (_emissive.is_valid())
						{
							e->emissive_activate(_emissive);
						}
					}
				}
			}
		};

		if (executionData->launchControl.activeEmissive != executionData->launchControl.requestedEmissive)
		{
			executionData->launchControl.activeEmissive = executionData->launchControl.requestedEmissive;
			ApplyEmissive::to(executionData->launchControl.activeEmissive, executionData->launchControl.buttonHandler.get_buttons());
			ApplyEmissive::to(executionData->launchControl.activeEmissive, executionData->launchControl.dialHandler.get_dials());
			ApplyEmissive::to(executionData->launchControl.activeEmissive, executionData->launchControl.switchHandler.get_switches());
		}
		for_every(ic, executionData->interactiveControls)
		{
			if (ic->activeEmissive != ic->requestedEmissive)
			{
				ic->activeEmissive = ic->requestedEmissive;
				ApplyEmissive::to(ic->activeEmissive, ic->buttonHandler.get_buttons());
				ApplyEmissive::to(ic->activeEmissive, ic->dialHandler.get_dials());
				ApplyEmissive::to(ic->activeEmissive, ic->switchHandler.get_switches());
			}
		}
	}
}

void DropCapsuleControl::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);
}

LATENT_FUNCTION(DropCapsuleControl::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai drop capsule control] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<DropCapsuleControl>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("drop capsule control, hello!"));

	messageHandler.use_with(mind);
	{
		RefCountObjectPtr<ExecutionData> keepExecutionData;
		keepExecutionData = self->executionData;
		messageHandler.set(NAME(aim_dcc_startUp), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->requestedState = DropCapsuleControlState::StartUp;
			}
		);
		messageHandler.set(NAME(aim_dcc_requiresCalibration), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->requestedState = DropCapsuleControlState::RequiresCalibration;
			}
		);
		messageHandler.set(NAME(aim_dcc_calibrated), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->requestedState = DropCapsuleControlState::Calibrated;
			}
		);
		messageHandler.set(NAME(aim_dcc_launch), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->requestedState = DropCapsuleControlState::Launched;
			}
		);
		messageHandler.set(NAME(aim_dcc_crashed), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->requestedState = DropCapsuleControlState::Crashed;
			}
		);
		messageHandler.set(NAME(aim_dcc_docked), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->requestedState = DropCapsuleControlState::Docked;
			}
		);
		messageHandler.set(NAME(aim_dcc_allowLaunch), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->launchAllowed = true;
			}
		);
		messageHandler.set(NAME(aim_dcc_disallowLaunch), [keepExecutionData](Framework::AI::Message const& _message)
			{
				keepExecutionData->launchAllowed = false;
			}
		);
		messageHandler.set(NAME(aim_dcc_setTimeToHit), [keepExecutionData](Framework::AI::Message const& _message)
			{
				if (auto* timeToHit = _message.get_param(NAME(timeToHit)))
				{
					keepExecutionData->timeToHit = timeToHit->get_as<float>();
					if (keepExecutionData->timeToHit.get() <= 0.0f)
					{
						keepExecutionData->timeToHit.clear();
					}
				}
			}
		);
	}

	// wait a bit for interactive controls to appear
	LATENT_WAIT(0.1f);

	if (imo)
	{
		if (auto* cmd = imo->get_custom<Framework::CustomModules::Display>())
		{
			self->display = cmd->get_display();
		}

		self->executionData->interactiveControls.clear();
		{
			if (auto* r = imo->get_presence()->get_in_room())
			{
				if (auto* id = imo->get_variables().get_existing<int>(NAME(dropCapsuleControlId)))
				{
					int dropCapsuleControlId = *id;

					auto* world = imo->get_in_world();
					an_assert(world);

					self->executionData->dropCapsuleControlStartUpSequences = 1;

					world->for_every_object_with_id(NAME(dropCapsuleControlId), dropCapsuleControlId,
						[self, imo](Framework::Object* _object)
						{
							if (_object != imo)
							{
								int interactiveDeviceId = _object->get_variables().get_value<int>(NAME(interactiveDeviceId), NONE);
								if (interactiveDeviceId != NONE)
								{
									if (_object->get_tags().get_tag(NAME(dropCapsuleLaunchControl)))
									{
										InteractiveControl ic;
										ic.buttonHandler.initialise(_object, NAME(interactiveDeviceId));
										ic.dialHandler.initialise(_object, NAME(interactiveDeviceId));
										ic.switchHandler.initialise(_object, NAME(interactiveDeviceId));
										self->executionData->launchControl = ic;
									}
									else
									{
										InteractiveControl ic;
										ic.dropCapsuleControlStartUpSequenceIdx = _object->get_variables().get_value<int>(NAME(dropCapsuleControlStartUpSequenceIdx), 0);
										ic.interactiveDeviceId = interactiveDeviceId;
										ic.buttonHandler.initialise(_object, NAME(interactiveDeviceId));
										ic.dialHandler.initialise(_object, NAME(interactiveDeviceId));
										ic.switchHandler.initialise(_object, NAME(interactiveDeviceId));
										self->executionData->interactiveControls.push_back(ic);
										self->executionData->dropCapsuleControlStartUpSequences = max(self->executionData->dropCapsuleControlStartUpSequences, ic.dropCapsuleControlStartUpSequenceIdx + 1);
									}
								}
							}
							return false; // keep going on - we want to get all
						});
				}
			}
		}
	}

	while (true)
	{
		LATENT_YIELD(); // to allow advance every frame
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(DropCapsuleControlData);

bool DropCapsuleControlData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	dropCapsuleLineModel.load_from_xml(_node, TXT("dropCapsuleLineModel"), _lc);
	obstacleLineModel.load_from_xml(_node, TXT("obstacleLineModel"), _lc);

	return result;
}

bool DropCapsuleControlData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= dropCapsuleLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);
	result &= obstacleLineModel.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

//

DropCapsuleControl::InteractiveControl::~InteractiveControl()
{
}

DropCapsuleControl::InteractiveControl::InteractiveControl()
{
}
