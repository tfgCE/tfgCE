#include "aiLogic_energyBalancer.h"

#include "..\..\game\playerSetup.h"
#include "..\..\modules\custom\mc_emissiveControl.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\core\vr\iVR.h"

#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\ai\aiMessageHandler.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\gameScript\gameScriptExecution.h"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\framework\module\moduleSound.h"
#include "..\..\..\framework\module\moduleTemporaryObjects.h"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\object\object.h"
#include "..\..\..\framework\world\room.h"
#include "..\..\..\framework\world\world.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// variables
DEFINE_STATIC_NAME(triggerGameScriptExecutionTrapOnDone);
DEFINE_STATIC_NAME(autoOverflowIdx);
DEFINE_STATIC_NAME(timeToOverheat);
DEFINE_STATIC_NAME(timeToCooldown);

// variables
DEFINE_STATIC_NAME(energyBalancerDeviceId);
DEFINE_STATIC_NAME(interactiveDeviceId);
DEFINE_STATIC_NAME(balancerButtonIdx);
DEFINE_STATIC_NAME(balancerSliderIdx);

// emissive layers for buttons
DEFINE_STATIC_NAME(ready);
DEFINE_STATIC_NAME(bad);
DEFINE_STATIC_NAME(busy);

// sounds
DEFINE_STATIC_NAME(overflow);
DEFINE_STATIC_NAME(warning);
DEFINE_STATIC_NAME(done);
DEFINE_STATIC_NAME(restored);
DEFINE_STATIC_NAME_STR(callToRestore, TXT("call to restore"));

// temporary objects
DEFINE_STATIC_NAME_STR(damagedSparkles, TXT("damaged sparkles"));

// ai message
DEFINE_STATIC_NAME_STR(aimOverflow, TXT("energyBalancerOverflow"));
	DEFINE_STATIC_NAME(overflowIdx);
DEFINE_STATIC_NAME_STR(aimBalance, TXT("energyBalancerBalance"));
	DEFINE_STATIC_NAME(silent);

//

REGISTER_FOR_FAST_CAST(EnergyBalancer);

EnergyBalancer::EnergyBalancer(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const * _logicData)
: base(_mind, _logicData, execute_logic)
{
	energyBalancerData = fast_cast<EnergyBalancerData>(_logicData);
}

EnergyBalancer::~EnergyBalancer()
{
}

void EnergyBalancer::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	redrawTime -= _deltaTime;
	if (redrawTime <= 0.0f)
	{
		redrawNow = true;
		redrawTime = 0.05f;
	}

	{
		bool allSafe = true;
		for_every(c, containers)
		{
			if (!c->overheated && c->contains > c->maxSafe)
			{
				allSafe = false;
				break;
			}
		}

		if (allSafe)
		{
			maxSafePt = 1.0f;
		}
		else
		{
			float prev = maxSafePt;
			maxSafePt = mod(maxSafePt + _deltaTime, 1.0f);
			if (maxSafePt < prev)
			{
				play_sound(nullptr, NAME(warning));
			}
		}
	}

	{
		bool anyOverheated = false;
		for_every(c, containers)
		{
			if (c->overheated)
			{
				anyOverheated = true;
				break;
			}
		}

		if (anyOverheated)
		{
			float prev = overheatedPt;
			overheatedPt = mod(overheatedPt + _deltaTime, 1.0f);
			if (overheatedPt < prev)
			{
				play_sound(nullptr, NAME(callToRestore));
			}
			for_every(c, containers)
			{
				if (c->overheated)
				{
					c->update_emissive(this);
				}
			}
		}
		else
		{
			overheatedPt = 1.0f;
		}
	}
	if (display && ! displaySetup)
	{
		displaySetup = true;
		display->draw_all_commands_immediately();
		display->set_on_update_display(this,
			[this](Framework::Display* _display)
			{
				if (!redrawNow)
				{
					return;
				}

				redrawNow = false;

				_display->drop_all_draw_commands();
				_display->always_draw_commands_immediately();
				_display->add((new Framework::DisplayDrawCommands::CLS(Colour::black)));
				_display->add((new Framework::DisplayDrawCommands::Border(Colour::black)));
				_display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
					[this](Framework::Display* _display, ::System::Video3D* _v3d)
					{
						Vector2 rt = _display->get_right_top_of_screen();
						Vector2 lb = _display->get_left_bottom_of_screen();

						Vector2 size(round((rt.x - lb.x) / 4.0f), round(((rt.x - lb.x) / 2.0f) * 0.9f));
						Range2 rect[4];
						rect[0] = Range2::empty;
						rect[1] = Range2::empty;
						rect[2] = Range2::empty;
						rect[3] = Range2::empty;
						rect[0].include(Vector2(lb.x, rt.y)).include(Vector2(lb.x + (size.x - 1.0f), rt.y - (size.y - 1.0f)));
						rect[1].include(Vector2(rt.x, rt.y)).include(Vector2(rt.x - (size.x - 1.0f), rt.y - (size.y - 1.0f)));
						rect[2].include(Vector2(lb.x, lb.y)).include(Vector2(lb.x + (size.x - 1.0f), lb.y + (size.y - 1.0f)));
						rect[3].include(Vector2(rt.x, lb.y)).include(Vector2(rt.x - (size.x - 1.0f), lb.y + (size.y - 1.0f)));

						Vector2 socket[4];
						socket[0] = Vector2(rect[0].x.max + 1.0f, rect[0].y.get_at(0.2f));
						socket[1] = Vector2(rect[1].x.min - 1.0f, rect[1].y.get_at(0.2f));
						socket[2] = Vector2(rect[2].x.max + 1.0f, rect[2].y.get_at(0.8f));
						socket[3] = Vector2(rect[3].x.min - 1.0f, rect[3].y.get_at(0.8f));

						Colour colourTransfer = _display->get_current_ink();
						Colour colourActive = done? Colour::c64LightBlue : _display->get_current_paper();

						for_every(c, containers)
						{
							int idx = for_everys_index(c);
							Colour colour = _display->get_current_ink();
							Colour colourOverheated = Colour::red;
							Colour colourOverheatedBack = Colour::red.mul_rgb(0.5f);

							bool appearOverheated = c->overheated;
							if (c->contains > c->maxSafe && maxSafePt <= 0.25f)
							{
								appearOverheated = true;
							}
							if (appearOverheated)
							{
								colour = colourOverheated;
							}

							Range2 r = rect[idx];
							Range2 rOrg = r;
							float smallerBy = 2.0f;
							r.expand_by(Vector2::one * (-smallerBy * 2.0f));
							if (c->overheatedDisplay > 0.0f && (! c->overheated || overheatedPt < 0.7f))
							{
								Range2 rO = rOrg;
								rO.y.max = rO.y.get_at(c->overheatedDisplay);
								::System::Video3DPrimitives::fill_rect_2d(colourOverheatedBack, rO, false);
							}
							::System::Video3DPrimitives::rect_2d(colour, rOrg.get_at(Vector2::zero), rOrg.get_at(Vector2::one) - Vector2::half, false);
							float reqPt;
							float safePt;
							{
								float at = c->required.div_to_float(c->capacity);
								reqPt = at;
								::System::Video3DPrimitives::line_2d(colour, Vector2(rOrg.x.min, r.y.get_at(at)), Vector2(rOrg.x.max, r.y.get_at(at)));
							}
							{
								float at = c->maxSafe.div_to_float(c->capacity);
								safePt = at;
								::System::Video3DPrimitives::line_2d(colour, Vector2(rOrg.x.min, r.y.get_at(at)), Vector2(rOrg.x.max, r.y.get_at(at)));
							}

							if (c->contains < c->required)
							{
								Vector2 at = rOrg.centre();
								at = round(at);
								at.y = r.y.get_at((reqPt + safePt) * 0.5f);
								float size = rOrg.x.length() * 0.6f;
								size = round(min(size, r.y.get_at(safePt - reqPt) - 4.0f)); // so it doesn't get out of the boundary
								float ax = round(size * 0.5f) + 0.5f;
								float ay = round(ax * 0.5f) + 0.5f;
								ax = ay * 2.0f;
								if (!c->overheated)
								{
									float ax2 = ax;
									float ay2 = ay;
									Vector2 offsets[] = { Vector2( 0.0f,  1.0f),
														  Vector2( 1.0f,  0.0f),
														  Vector2( 0.0f, -1.0f),
														  Vector2(-1.0f,  0.0f),
														  Vector2( 1.0f, -1.0f),
														  Vector2(-1.0f, -1.0f) };
									for_count(int, offset, 6)
									{
										::System::Video3DPrimitives::triangle_2d(_display->get_current_paper(),
											at + Vector2(0.0f, ay2)  + offsets[offset],
											at + Vector2( ax2, -ay2) + offsets[offset],
											at + Vector2(-ax2, -ay2) + offsets[offset], false);
									}
								}
								::System::Video3DPrimitives::triangle_2d(c->overheated? _display->get_current_paper() : colour,
									at + Vector2(0.0f, ay),
									at + Vector2( ax, -ay),
									at + Vector2(-ax, -ay), false);
							}
							/*
							if (c->overheatedDisplay > 0.0f)
							{
								::System::Video3DPrimitives::line_2d(colourOverheated, rOrg.get_at(Vector2(0.0f, 0.0f)), rOrg.get_at(Vector2(1.0f, 0.0f)));
								::System::Video3DPrimitives::line_2d(colourOverheated, rOrg.get_at(Vector2(0.0f, 0.0f)), rOrg.get_at(Vector2(0.0f, c->overheatedDisplay)));
								::System::Video3DPrimitives::line_2d(colourOverheated, rOrg.get_at(Vector2(1.0f, 0.0f)), rOrg.get_at(Vector2(1.0f, c->overheatedDisplay)));
								if (c->overheatedDisplay >= 1.0f)
								{
									::System::Video3DPrimitives::line_2d(colourOverheated, rOrg.get_at(Vector2(0.0f, 1.0f)), rOrg.get_at(Vector2(1.0f, 1.0f)));
								}
							}
							*/

							if (c->contains.is_positive())
							{
								r.y.max = r.y.get_at(c->contains.div_to_float(c->capacity));
								::System::Video3DPrimitives::fill_rect_2d(colour, r, false);
							}

							{
								Vector2 start = socket[idx];
								Vector2 end = start;
								end.x = (rt.x + lb.x) * 0.5f;

								Segment2 s(start, end);
								Range r = Range::empty;
								r.include(start.x);
								r.include(end.x);

								float f = -c->flowPt;
								float divInto = 4.0f;
								float off = 2.0f;
								float ft = f / divInto;
								float fx = start.x + (end.x - start.x) * ft;;
								float l = round((end.x - start.x) / divInto);
								float l1 = round(l / 4.5f);
								while ((fx - end.x) * (start.x - end.x) >= 0.0f)
								{
									float fxs = fx;
									float fxe = fx + l1;
									if (r.does_contain(fxs) ||
										r.does_contain(fxe))
									{
										fxs = r.clamp(fxs);
										fxe = r.clamp(fxe);
										{
											Vector2 a(min(fxs, fxe), start.y);
											Vector2 b(max(fxs, fxe), start.y);
											a.y -= off;
											b.y += off;
											::System::Video3DPrimitives::fill_rect_2d(colour, a, b, false);
										}
									}
									fx += l;
								}

								Vector2 offY = Vector2(0.0f, off + 2.0f);
								::System::Video3DPrimitives::line_2d(colour, start + offY, end + offY);
								::System::Video3DPrimitives::line_2d(colour, start - offY, end - offY);
							}

						}

						{
							Range2 transfer = Range2::empty;

							Vector2 centre = (lb + rt) * 0.5f;
							Vector2 tSize((rt.x - lb.x) * 0.1f, (rt.y - lb.y) * 1.0f);

							transfer.include(centre - tSize * 0.5f);
							transfer.include(centre + tSize * 0.5f);
							::System::Video3DPrimitives::fill_rect_2d(colourTransfer, transfer, false);
							transfer.expand_by(-Vector2::one);
							::System::Video3DPrimitives::fill_rect_2d(colourActive, transfer, false);
						}

						{
							Vector2 centre = (lb + rt) * 0.5f;
							Vector2 tSize((rt.x - lb.x) * 0.2f, (rt.y - lb.y) * 0.2f);

							centre = round(centre);
							tSize = round(tSize);

							for_count(int, i, 2)
							{
								centre.y = i == 0? (lb.y + tSize.y * 0.5f) : (rt.y - tSize.y * 0.5f);
								Range2 transfer = Range2::empty;
								transfer.include(centre - tSize * 0.5f);
								transfer.include(centre + tSize * 0.5f);
								::System::Video3DPrimitives::fill_rect_2d(colourTransfer, transfer, false);
								transfer.expand_by(-Vector2::one);
								::System::Video3DPrimitives::fill_rect_2d(colourActive, transfer, false);
							}
						}

					}));
			});
	}

	for_every(c, containers)
	{
		if (c->overheated)
		{
			c->overheatedDisplay = 1.0f;
			if (auto* imo = c->button.get())
			{
				if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(imo->get_movement()))
				{
					if (mms->is_active_at(1))
					{
						if (c->overheated)
						{
							c->overheated = false;
							c->update_emissive(this);
							play_sound(nullptr, NAME(restored));
						}
					}
				}
			}
		}
		else if (c->contains > c->maxSafe)
		{
			c->overheatedDisplay = min(1.0f, c->overheatedDisplay + _deltaTime / timeToOverheat);
			if (c->overheatedDisplay >= 1.0f)
			{
				if (!c->overheated)
				{
					c->overheated = true;
					c->update_emissive(this);
					play_sound(nullptr, NAME(overflow));
				}
			}
		}
		else if (c->overheatedDisplay > 0.0f)
		{
			c->overheatedDisplay = max(0.0f, c->overheatedDisplay - _deltaTime / timeToCooldown);
			if (c->overheatedDisplay < 0.0f)
			{
				c->overheated = false;
				c->update_emissive(this);
			}
		}

		float pushToOthers = 0.0f;
		{
			if (c->overheated)
			{
				pushToOthers = 3.0f;
			}
			else
			{
				if (auto* imo = c->slider.get())
				{
					if (auto* mms = fast_cast<Framework::ModuleMovementSwitch>(imo->get_movement()))
					{
						pushToOthers = mms->get_at();
					}
				}
			}
		}

		Energy speed = Energy(5.0f * pushToOthers);
		if (speed.is_positive())
		{
			Energy transfer = max(Energy::smallest(), speed.mul(_deltaTime));
			for_every(tc, containers)
			{
				if (tc == c ||
					tc->overheated)
				{
					continue;
				}
				Energy e = min(transfer, c->contains);
				e = min(e, max(Energy::zero(), tc->capacity - tc->contains));
				if (done)
				{
					// can't exceed safe/required limits
					e = min(e, max(Energy::zero(), c->contains - c->required));
					e = min(e, max(Energy::zero(), tc->maxSafe - tc->contains));
				}
				tc->contains += e;
				c->contains -= e;
			}

			for_every(c, containers)
			{
				c->update_emissive(this);
			}
		}
	}

	for_every(c, containers)
	{
		float const maxSpeed = 5.0f;
		c->flowPtSpeed = clamp((c->contains - c->prev).div_to_float(Energy(0.01f)), -maxSpeed, maxSpeed);
		c->prev = c->contains;
		c->flowPt = mod(c->flowPt + _deltaTime * c->flowPtSpeed, 1.0f);
	}

	if (! done)
	{
		done = true;
		if (GameSettings::get().difficulty.simplerPuzzles)
		{
			for_every(c, containers)
			{
				if (c->overheated)
				{
					done = false;
					break;
				}
			}
			if (done)
			{
				force_balance();
			}
		}
		else
		{
			for_every(c, containers)
			{
				if (c->contains < c->required || c->contains > c->maxSafe || c->overheated)
				{
					done = false;
					break;
				}
			}
		}
		if (done)
		{
			if (triggerGameScriptExecutionTrapOnDone.is_valid())
			{
				Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptExecutionTrapOnDone);
				repeatTriggerGameScriptExecutionTrapOnDone = 0.5f;
			}
			{
				Energy experience = GameplayBalance::energy_balancer__xp();
				if (experience.is_positive())
				{
					experience = experience.adjusted_plus_one(GameSettings::get().experienceModifier);
					PlayerSetup::access_current().stats__experience(experience);
					GameStats::get().add_experience(experience);
					Persistence::access_current().provide_experience(experience);
				}
			}
			play_sound(nullptr, NAME(done));
			for_every(c, containers)
			{
				c->update_emissive(this);
			}
		}
	}
	else
	{
		repeatTriggerGameScriptExecutionTrapOnDone -= _deltaTime;
		if (repeatTriggerGameScriptExecutionTrapOnDone < 0.0f)
		{
			if (triggerGameScriptExecutionTrapOnDone.is_valid())
			{
				Framework::GameScript::ScriptExecution::trigger_execution_trap(triggerGameScriptExecutionTrapOnDone);
				repeatTriggerGameScriptExecutionTrapOnDone = 1.0f;
			}
		}
	}
}

void EnergyBalancer::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	int autoOverflowIdx = NONE;

	triggerGameScriptExecutionTrapOnDone = _parameters.get_value<Name>(NAME(triggerGameScriptExecutionTrapOnDone), triggerGameScriptExecutionTrapOnDone);
	autoOverflowIdx = _parameters.get_value<int>(NAME(autoOverflowIdx), autoOverflowIdx);
	timeToOverheat = _parameters.get_value<float>(NAME(timeToOverheat), timeToOverheat);
	timeToCooldown = _parameters.get_value<float>(NAME(timeToCooldown), timeToCooldown);

	{
		Random::Generator rg = get_mind()->get_owner_as_modules_owner()->get_individual_random_generator();
		rg.advance_seed(208580, 25974);

		while (true)
		{
			containers.clear();
			containers.set_size(4);
			int minIdx = rg.get_int(containers.get_size());
			for_every(c, containers)
			{
				int cap = rg.get_int_from_range(100, 250);
				if (for_everys_index(c) == minIdx)
				{
					cap = rg.get_int_from_range(40, 70);
				}
				cap = round_to(cap, 10);
				c->capacity = Energy(cap);
				c->maxSafe = Energy(round_down_to(cap * 9 / 10, 10));
				c->required = min(Energy(round_down_to(cap * rg.get_int_from_range(2, 7) / 10, 10)), c->maxSafe - Energy(10));
			}
			Energy totalCapacity = Energy::zero();
			Energy totalMaxSafe = Energy::zero();
			Energy totalMaxSafeNotOverheated = Energy::zero();
			Energy totalRequired = Energy::zero();
			if (autoOverflowIdx != NONE && containers.is_index_valid(autoOverflowIdx))
			{
				containers[autoOverflowIdx].overheated = true;
			}
			for_every(c, containers)
			{
				c->contains = Energy::zero();
				totalCapacity += c->capacity;
				totalMaxSafe += c->maxSafe;
				totalRequired += c->required;
				if (!c->overheated)
				{
					totalMaxSafeNotOverheated += c->maxSafe;
				}
			}

			Energy available = totalRequired * 5 / 4;

			if (available >= totalMaxSafeNotOverheated)
			{
				// reroll values
				continue;
			}

			// redistribute
			while (available.is_positive())
			{
				Energy e = min(available, Energy(rg.get_int_from_range(1, 3)));
				int idx = rg.get_int(containers.get_size());
				auto& c = containers[idx];
				if (!c.overheated)
				{
					Energy canHave = c.maxSafe - c.contains;
					e = min(e, canHave);
					available -= e;
					c.contains += e;
				}
			}

			break;
		}
	}
}

void EnergyBalancer::play_sound(Framework::IModulesOwner* imo, Name const& _sound)
{
	if (!imo)
	{
		imo = get_mind()->get_owner_as_modules_owner();
	}
	if (auto* s = imo->get_sound())
	{
		s->play_sound(_sound);
	}
}

void EnergyBalancer::force_balance()
{
	Energy available = Energy::zero();
	for_every(c, containers)
	{
		if (c->contains > c->required)
		{
			available += max(Energy::zero(), c->contains - c->required);
			c->contains = c->required;
		}
	}
	for_every(c, containers)
	{
		if (c->contains < c->required)
		{
			Energy give = c->required - c->contains;
			give = min(give, available);
			c->contains += give;
			available -= give;
		}
	}

	Random::Generator rg = get_mind()->get_owner_as_modules_owner()->get_individual_random_generator();
	rg.advance_seed(208580, 25974);

	while (available.is_positive())
	{
		for_every(c, containers)
		{
			Energy maxToGive = c->maxSafe - c->contains;
			Energy give = Energy(rg.get_int_from_range(1, 3));
			give = min(give, min(maxToGive, available));
			c->contains += give;
			available -= give;
		}
	}

	for_every(c, containers)
	{
		c->overheated = false;
		c->overheatedDisplay = 0.0f;
		c->update_emissive(this);
	}
}

LATENT_FUNCTION(EnergyBalancer::execute_logic)
{
	PERFORMANCE_GUARD_LIMIT(0.005f, TXT("[ai energy balancer] execute logic"));

	LATENT_SCOPE();
	LATENT_SCOPE_INFO(TXT("execute logic"));

	LATENT_PARAM(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_VAR(::Framework::AI::MessageHandler, messageHandler);
	LATENT_VAR(bool, foundIMOs);

#ifdef AUTO_TEST
	LATENT_VAR(::System::TimeStamp, autoTestTS);
#endif

	auto * imo = mind->get_owner_as_modules_owner();
	auto * self = fast_cast<EnergyBalancer>(logic);

	LATENT_BEGIN_CODE();

	ai_log(self, TXT("energy balancer, hello!"));

	messageHandler.use_with(mind);
	{
		messageHandler.set(NAME(aimOverflow), [self](Framework::AI::Message const& _message)
		{
			if (auto * idxParam = _message.get_param(NAME(overflowIdx)))
			{
				int idx = idxParam->get_as<int>();
				if (self->containers.is_index_valid(idx))
				{
					self->containers[idx].overheated = true;
				}
			}
		}
		);
		messageHandler.set(NAME(aimBalance), [self](Framework::AI::Message const& _message)
		{
			bool silent = false;
			if (auto * silentParam = _message.get_param(NAME(silent)))
			{
				silent = silentParam->get_as<bool>();
			}

			// forcing silent now
			self->force_balance();
		}
		);
	}

	foundIMOs = false;
	while (!foundIMOs)
	{
		if (auto* r = imo->get_presence()->get_in_room())
		{
			if (auto* id = imo->get_variables().get_existing<int>(NAME(energyBalancerDeviceId)))
			{
				auto* world = imo->get_in_world();
				an_assert(world);

				// find interactive device ids first (or buttons straight away)
				world->for_every_object_with_id(NAME(energyBalancerDeviceId), *id,
					[self, imo](Framework::Object* _object)
					{
						if (_object != imo)
						{
							if (auto* idx = _object->get_variables().get_existing<int>(NAME(balancerButtonIdx)))
							{
								if (self->containers.get_size() > *idx)
								{
									self->containers[*idx].button = _object;
									self->containers[*idx].update_emissive(self);
								}
								else
								{
									error(TXT("too many buttons for \"%S\""), _object->ai_get_name().to_char());
								}
							}
							else if (auto* idx = _object->get_variables().get_existing<int>(NAME(balancerSliderIdx)))
							{
								if (self->containers.get_size() > *idx)
								{
									self->containers[*idx].sliderInteractiveDeviceID = _object->get_variables().get_value<int>(NAME(interactiveDeviceId), NONE);
								}
								else
								{
									error(TXT("too many sliders for \"%S\""), _object->ai_get_name().to_char());
								}
							}
						}
						return false; // keep going on - we want to get all
					});
				for_every(c, self->containers)
				{
					if (c->sliderInteractiveDeviceID != NONE)
					{
						world->for_every_object_with_id(NAME(interactiveDeviceId), c->sliderInteractiveDeviceID,
							[c](Framework::Object* _object)
							{
								if (fast_cast<Framework::ModuleMovementSwitch>(_object->get_movement()))
								{
									c->slider = _object;
									return true; // just the first one
								}
								return false; // keep looking
							});
					}
				}
			}
			else
			{
				error(TXT("no \"energyBalancerDeviceId\" for \"%S\""), imo->ai_get_name().to_char());
			}
		}
		{
			foundIMOs = ! self->containers.is_empty();
			for_every(c, self->containers)
			{
				if (!c->button.is_set() ||
					!c->slider.is_set())
				{
					foundIMOs = false;
				}
			}
		}
		LATENT_WAIT(self->rg.get_float(0.1f, 0.2f));
	}

	while (!self->display)
	{
		if (auto* cmd = imo->get_custom<Framework::CustomModules::Display>())
		{
			self->display = cmd->get_display();
		}
		LATENT_WAIT(self->rg.get_float(0.1f, 0.2f));
	}
	self->displaySetup = false;

	while (true)
	{		
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();
	//

	LATENT_ON_END();
	//

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

void EnergyBalancer::Container::update_emissive(EnergyBalancer const* _eb)
{
	if (auto* i = button.get())
	{
		if (auto* ec = i->get_custom<CustomModules::EmissiveControl>())
		{
			bool meetsReq = contains >= required;
			ec->emissive_set_active(NAME(ready), ! overheated && ! meetsReq);
			ec->emissive_set_active(NAME(bad), overheated && _eb->overheatedPt < 0.7f);
			ec->emissive_set_active(NAME(busy), ! overheated && meetsReq);
		}
	}
}

//

REGISTER_FOR_FAST_CAST(EnergyBalancerData);

bool EnergyBalancerData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool EnergyBalancerData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}
