#include "me_mine.h"

#include "..\..\custom\mc_switchSidesHandler.h"

#include "..\..\..\teaForGodTest.h"

#include "..\..\..\game\damage.h"
#include "..\..\..\game\game.h"

#include "..\..\..\ai\aiRayCasts.h"

#include "..\moduleEnergyQuantum.h"
#include "..\modulePilgrim.h"

#include "..\..\custom\mc_emissiveControl.h"
#include "..\..\custom\mc_pickup.h"
#include "..\..\custom\health\mc_health.h"

#include "..\..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\..\framework\ai\aiSocial.h"
#include "..\..\..\..\framework\debug\previewGame.h"
#include "..\..\..\..\framework\display\display.h"
#include "..\..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\..\framework\display\displayText.h"
#include "..\..\..\..\framework\display\displayUtils.h"
#include "..\..\..\..\framework\game\gameUtils.h"
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\module\moduleAI.h"
#include "..\..\..\..\framework\module\moduleMovementFastColliding.h"
#include "..\..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_ALLOW_EXTENSIVE_LOGS
//#define INSPECT_ARMED_MINE
#endif

//

using namespace TeaForGodEmperor;
using namespace ModuleEquipments;

//

// params
DEFINE_STATIC_NAME(detonationTime);
DEFINE_STATIC_NAME(detonationTimeFastColliding);
DEFINE_STATIC_NAME(proximityRange);
DEFINE_STATIC_NAME(proximitySpeed);
DEFINE_STATIC_NAME(detectFlags);
DEFINE_STATIC_NAME(confirmRayFlags);

// vars
DEFINE_STATIC_NAME(interactiveDeviceId);

// sounds
DEFINE_STATIC_NAME(triggered);

// temporary objects
DEFINE_STATIC_NAME(explode);

// emissive
DEFINE_STATIC_NAME(armed);
DEFINE_STATIC_NAME_STR(armedActive, TXT("armed active"));
DEFINE_STATIC_NAME_STR(armedDetonate, TXT("armed detonate"));

// collision/detection
DEFINE_STATIC_NAME(mineDetection);

// rare advance for move
DEFINE_STATIC_NAME(raMoveArmedMine);

//

REGISTER_FOR_FAST_CAST(Mine);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new Mine(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new MineData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & Mine::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("mine")), create_module, create_module_data);
}

Mine::Mine(Framework::IModulesOwner* _owner)
: base( _owner )
{
	allowEasyReleaseForAutoInterimEquipment = true;
	requiresDisplayExtra = true;
	rotatedDisplay = true;
}

Mine::~Mine()
{
}

void Mine::display_extra(::Framework::Display* _display, bool _justStarted)
{
	Framework::DisplayUtils::clear_all(_display);
	tchar const * t = TXT("-");
	Colour colour = mineData->colourSafe;
	if (!safe)
	{
		colour = mineData->colourArmed;
		t = TXT("x");
	}

	{
		auto* text = new Framework::DisplayDrawCommands::TextAt();
		text->text(t)
			->use_font(mineData->font.get())
			->at(mineData->textAt)
			->scale(mineData->textScale)
			->use_coordinates(Framework::DisplayCoordinates::Pixel)
			->use_colourise_ink(colour)
			;
		_display->add(text);
	}

	_display->draw_all_commands_immediately();
}

void Mine::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	mineData = fast_cast<MineData>(_moduleData);

	detonationTime = _moduleData->get_parameter<float>(this, NAME(detonationTime), detonationTime);
	detonationTimeFastColliding = _moduleData->get_parameter<float>(this, NAME(detonationTimeFastColliding), detonationTimeFastColliding);
	proximityRange = _moduleData->get_parameter<float>(this, NAME(proximityRange), proximityRange);
	proximitySpeed = _moduleData->get_parameter<float>(this, NAME(proximitySpeed), proximitySpeed);
	detectFlags.apply(_moduleData->get_parameter<String>(this, NAME(detectFlags)));
	if (detectFlags.is_empty())
	{
		error(TXT("no detect flags set! won't be triggered"));
	}
	confirmRayFlags.apply(_moduleData->get_parameter<String>(this, NAME(confirmRayFlags)));
}

void Mine::reset()
{
	base::reset();
}

void Mine::initialise()
{
	base::initialise();
}

void Mine::activate()
{
	base::activate();

	safe = true;
	armedBy = nullptr;

	if (auto* imo = get_owner())
	{
		an_assert(imo->get_in_world());

		if (mineData->allowArmingVar.is_valid())
		{
			allowArmingVar = imo->access_variables().find<bool>(mineData->allowArmingVar);
		}
		if (mineData->armedVar.is_valid())
		{
			armedVar = imo->access_variables().find<bool>(mineData->armedVar);
		}
	}

	interactiveButtonHandler.has_to_be_held();
	interactiveButtonHandler.initialise(get_owner(), mineData->buttonIdVar, mineData->onButtonIdVar);
}

void Mine::trigger()
{
#ifdef TEST_NON_EXPLOSIVE_EXPLODABLES
	return;
#endif
	if (!timeToDetonate.is_set())
	{
		if (auto* s = get_owner()->get_sound())
		{
			s->play_sound(NAME(triggered));
		}
		timeToDetonate = detonationTime;
		if (fast_cast<Framework::ModuleMovementFastColliding>(get_owner()->get_movement()))
		{
			timeToDetonate = detonationTimeFastColliding;
		}
	}
}

void Mine::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

#ifdef AN_DEVELOPMENT
	if (Game::get_as<Framework::PreviewGame>())
	{
		return;
	}
#endif

	interactiveButtonHandler.advance(_deltaTime);

	bool heldNow = false;

	if (get_user())
	{
		heldNow = true;
	}
	else if (auto * pickup = get_owner()->get_custom<CustomModules::Pickup>())
	{
		if (pickup->is_held() || pickup->is_in_pocket() || pickup->is_in_holder())
		{
			heldNow = true;
		}
	}

	bool armedActive = false;

	bool switchSafe = false;
	auto* switchSidesHandler = get_owner()->get_custom<CustomModules::SwitchSidesHandler>();

	if (switchSidesHandler)
	{
		bool newSwitchedSides = switchSidesHandler->get_switch_sides_to() != nullptr;
		switchSafe = !switchedSides.is_set() || switchedSides.get() != newSwitchedSides;
		switchedSides = newSwitchedSides;
	}

	if (heldNow || switchSafe)
	{
		timeToDetonate.clear();
		timeToProximityCheck.clear();

		if (! switchSidesHandler)
		{
			switchSafe = interactiveButtonHandler.is_triggered();
		}

		if (switchSafe)
		{
			bool shouldBeSafe = !safe;
			if (switchSidesHandler)
			{
				shouldBeSafe = !switchedSides.get();
			}
			if (!shouldBeSafe)
			{
#ifdef INSPECT_ARMED_MINE
				output(TXT("[MINE] armed o%p"), get_owner());
#endif
				if (mineData->armSound.is_valid())
				{
					if (auto * s = get_owner()->get_sound())
					{
						s->play_sound(mineData->armSound);
					}
				}
				safe = false;
				if (switchSidesHandler)
				{
					armedBy = switchSidesHandler->get_switch_sides_to();
				}
				else
				{
					if (auto* pilgrim = get_user())
					{
						armedBy = pilgrim->get_owner();
					}
					else
					{
						armedBy = nullptr;
					}
				}
				get_owner()->allow_rare_move_advance(false, NAME(raMoveArmedMine)); // every frame
			}
			else
			{
#ifdef INSPECT_ARMED_MINE
				output(TXT("[MINE] disarmed o%p"), get_owner());
#endif
				if (mineData->disarmSound.is_valid())
				{
					if (auto * s = get_owner()->get_sound())
					{
						s->play_sound(mineData->disarmSound);
					}
				}
				safe = true;
				armedBy = nullptr;
				get_owner()->allow_rare_move_advance(true, NAME(raMoveArmedMine)); // every frame
			}
		}
	}
	else
	{
		if (!safe && !detonated && (!allowArmingVar.is_valid() || allowArmingVar.get<bool>()))
		{
			armedActive = true;
			bool detonate = false;
			if (!timeToProximityCheck.is_set())
			{
				timeToProximityCheck = allowArmingVar.is_valid()? 0.1f : 0.2f;
			}
			else
			{
				timeToProximityCheck = max(0.0f, timeToProximityCheck.get() - _deltaTime);
			}
#ifdef INSPECT_ARMED_MINE
			output(TXT("[MINE] act armed o%p (tp:%.3f)"), get_owner(), timeToProximityCheck.get());
#endif
			if (timeToProximityCheck.get() <= 0.0f &&
				!timeToDetonate.is_set())
			{
#ifdef INSPECT_ARMED_MINE
				{
					LogInfoContext l;
					l.log(TXT("[MINE] o%p's presence links:"), get_owner());
					{
						LOG_INDENT(l);
						get_owner()->get_presence()->log_presence_links(l);
					}
					l.output_to_output();
				}
#endif
					if (auto* c = get_owner()->get_collision())
				{
					if (!c->get_detected().is_empty())
					{
#ifdef INSPECT_ARMED_MINE
						output(TXT("[MINE] o%p collision detected"), get_owner());
#endif
						bool actualObject = false;
						Framework::AI::Social const * armedBySocial = nullptr;
						if (armedBy.get())
						{
							if (auto* armedByAI = armedBy.get()->get_ai())
							{
								armedBySocial = &armedByAI->get_mind()->get_social();
							}
						}
						for_every(dc, c->get_detected())
						{
							if (auto* ob = fast_cast<Framework::Object>(dc->ico))
							{
#ifdef INSPECT_ARMED_MINE
								output(TXT("[MINE] o%p collision detected with o%p \"%S\""), get_owner(), ob, ob->ai_get_name().to_char());
#endif
								// only if not the one who armed us (unless works on speed)
								if (ob == armedBy.get() ||
									(ob->get_top_instigator() == armedBy.get() && proximitySpeed == 0.0f))
								{
#ifdef INSPECT_ARMED_MINE
									output(TXT("[MINE] o%p collision detected with o%p - ignore"), get_owner(), ob);
#endif
									continue;
								}
								if (armedBySocial)
								{
									if (auto* ai = ob->get_ai())
									{
										if (!armedBySocial->is_enemy(ob))
										{
#ifdef INSPECT_ARMED_MINE
											output(TXT("[MINE] o%p collision detected with o%p - ignore - not an enemy"), get_owner(), ob);
											if (auto* ai = get_owner()->get_ai())
											{
												if (auto* mind = ai->get_mind())
												{
													output(TXT("[MINE] o%p's faction = "), get_owner(), mind->get_social().get_faction().to_char());
												}
											}
											if (auto* ai = ob->get_ai())
											{
												if (auto* mind = ai->get_mind())
												{
													output(TXT("[MINE] o%p's faction = "), ob, mind->get_social().get_faction().to_char());
												}
											}
#endif
											continue;
										}
									}
									else
									{
#ifdef INSPECT_ARMED_MINE
										output(TXT("[MINE] o%p collision detected with o%p - ignore - no ai"), get_owner(), ob);
#endif
										continue; // no ai? it can't be an enemy
									}
								}
								bool speedCheck = false; // this is not part of the player, explode on any proximity
								if (Framework::GameUtils::is_controlled_by_player(ob))
								{
									speedCheck = true; // this is a part of the player, do speed check
								}
#ifdef INSPECT_ARMED_MINE
								output(TXT("[MINE] o%p collision detected with o%p - %S - v:%.3f >= %.3f"), get_owner(), ob, speedCheck? TXT("speed check") : TXT("no speed check"), ob->get_presence()->get_velocity_linear().length(), proximitySpeed);
#endif
								if (!speedCheck || ob->get_presence()->get_velocity_linear().length() >= proximitySpeed)
								{
#ifdef INSPECT_ARMED_MINE
									output(TXT("[MINE] o%p collision detected with o%p - good candidate to detonate"), get_owner(), ob);
#endif
									bool ok = true;
									if (dc->distance > 0.1f) // close enough to ignore objects in the way
									{
										if (Framework::IModulesOwner* enemy = cast_to_nonconst(ob))
										{
											Framework::RelativeToPresencePlacement rpp;
											rpp.be_temporary_snapshot();
											if (rpp.find_path(get_owner(), enemy))
											{
												AI::CastInfo castInfo;
												castInfo.at_within_target();
												if (! confirmRayFlags.is_empty())
												{
													castInfo.with_collision_flags(confirmRayFlags);
												}
												ok = AI::check_clear_ray_cast(castInfo, get_owner()->get_presence()->get_centre_of_presence_WS(), get_owner(),
													cast_to_nonconst(ob),
													rpp.get_in_final_room(), rpp.get_placement_in_owner_room());
#ifdef INSPECT_ARMED_MINE
												debug_subject(get_owner());
												debug_context(get_owner()->get_presence()->get_in_room());
												debug_draw_arrow(true, ok ? Colour::red : Colour::blue, get_owner()->get_presence()->get_centre_of_presence_WS(), rpp.get_placement_in_owner_room().location_to_world(ob->get_presence()->get_centre_of_presence_os().get_translation()));
												debug_no_context();
												debug_context(enemy->get_presence()->get_in_room());
												debug_draw_sphere(true, false, Colour::cyan, 0.2f, Sphere(enemy->get_presence()->get_placement().location_to_world(enemy->get_presence()->get_centre_of_presence_os().get_translation()), 0.02f));
												debug_no_context();
												debug_no_subject();
												//AN_PAUSE;
#endif
											}
										}
									}
									if (ok)
									{
#ifdef INSPECT_ARMED_MINE
										output(TXT("[MINE] o%p collision detected with o%p - good candidate to detonate - confirmed"), get_owner(), ob);
#endif
										actualObject = true;
										break;
									}
#ifdef INSPECT_ARMED_MINE
									else
									{
										output(TXT("[MINE] o%p collision detected with o%p - failed additional checks"), get_owner(), ob);
									}
#endif
								}
							}
						}
						if (actualObject)
						{
							trigger();
#ifdef INSPECT_ARMED_MINE
							output(TXT("[MINE] o%p detonate in %.3f"), get_owner(), timeToDetonate.get());
#endif
						}
					}
				}
			}
			if (timeToDetonate.is_set())
			{
				timeToDetonate = timeToDetonate.get() - _deltaTime;
#ifdef INSPECT_ARMED_MINE
				output(TXT("[MINE] o%p time to detonate %.3f"), get_owner(), timeToDetonate.get());
#endif
				if (timeToDetonate.get() <= 0.0f)
				{
					detonate = true;
				}
			}

			if (detonate)
			{
#ifdef INSPECT_ARMED_MINE
				output(TXT("[MINE] o%p DETONATE NOW!"), get_owner());
#endif
				explode();
			}
		}
		else
		{
			timeToProximityCheck.clear();
		}
	}

	if (auto* c = get_owner()->get_collision())
	{
		if (armedActive)
		{
			if (!collisionForArmed)
			{
				collisionForArmed = true;
				c->set_radius_for_detection_override(proximityRange);
				c->push_detects_flags(NAME(mineDetection), detectFlags);
			}
		}
		else
		{
			if (collisionForArmed)
			{
				collisionForArmed = false;
				c->set_radius_for_detection_override();
				c->pop_detects_flags(NAME(mineDetection));
			}
		}
	}

	if (auto* e = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		if (timeToDetonate.is_set())
		{
			if (e->has_emissive(NAME(armedDetonate)))
			{
				e->emissive_deactivate(NAME(armed));
				e->emissive_deactivate(NAME(armedActive));
				e->emissive_activate(NAME(armedDetonate));
			}
			else
			{
				e->emissive_deactivate(NAME(armed));
				e->emissive_activate(NAME(armedActive));
			}
		}
		else if (heldNow || !armedActive)
		{
			e->emissive_deactivate(NAME(armedActive));
			e->emissive_deactivate(NAME(armedDetonate));
			if (!safe)
			{
				e->emissive_activate(NAME(armed));
			}
			else
			{
				e->emissive_deactivate(NAME(armed));
			}
		}
		else if (armedActive)
		{
			e->emissive_deactivate(NAME(armed));
			e->emissive_deactivate(NAME(armedDetonate));
			if (!safe)
			{
				e->emissive_activate(NAME(armedActive));
			}
			else
			{
				e->emissive_deactivate(NAME(armedActive));
			}
		}
		else
		{
			e->emissive_deactivate(NAME(armed));
			e->emissive_deactivate(NAME(armedActive));
			e->emissive_deactivate(NAME(armedDetonate));
		}
	}

	if (armedVar.is_valid())
	{
		armedVar.access<bool>() = armedActive;
	}
}

void Mine::explode(Framework::IModulesOwner* _instigator)
{
#ifdef TEST_NON_EXPLOSIVE_EXPLODABLES
	return;
#endif
	detonated = true;

	if (auto * tos = get_owner()->get_temporary_objects())
	{
		if (!_instigator)
		{
			_instigator = get_owner()->get_valid_top_instigator();
		}
		// explosion will do the damage
		Array<SafePtr<Framework::IModulesOwner>> spawned;
		tos->spawn_all(NAME(explode), NP, &spawned);
		for_every_ref(s, spawned)
		{
			s->set_creator(get_owner());
			s->set_instigator(_instigator);
		}
	}

	get_owner()->set_timer(0.005f, [](Framework::IModulesOwner* _imo) { _imo->cease_to_exist(true); }); // destroy this one in a while
}

bool Mine::on_death(Damage const& _damage, DamageInfo const & _damageInfo)
{
	if (!detonated && (mineData->alwaysExplodeOnDeath || !safe) && !_damageInfo.peacefulDamage &&
		_damage.damageType != DamageType::Corrosion)
	{
		explode(_damageInfo.instigator.get());
		return false;
	}
	return base::on_death(_damage, _damageInfo);
}

void Mine::advance_user_controls()
{
	base::advance_user_controls();

	if (!user)
	{
		return;
	}
}

Optional<Energy> Mine::get_primary_state_value() const
{
	if (auto* h = get_owner()->get_custom<CustomModules::Health>())
	{
		return h->get_health();
	}
	else
	{
		return NP;
	}
}

//

REGISTER_FOR_FAST_CAST(MineData);

MineData::MineData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
	onButtonIdVar = NAME(interactiveDeviceId);
}

MineData::~MineData()
{
}

bool MineData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool MineData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("alwaysExplodeOnDeath"))
	{
		alwaysExplodeOnDeath = _attr->get_as_bool();
		return true;
	}	
	if (_attr->get_name() == TXT("allowArmingVar"))
	{
		allowArmingVar = _attr->get_as_name();
		return true;
	}	
	if (_attr->get_name() == TXT("armedVar"))
	{
		armedVar = _attr->get_as_name();
		return true;
	}	
	if (_attr->get_name() == TXT("buttonIdVar"))
	{
		buttonIdVar = _attr->get_as_name();
		return true;
	}	
	if (_attr->get_name() == TXT("onButtonIdVar"))
	{
		onButtonIdVar = _attr->get_as_name();
		return true;
	}	
	if (_attr->get_name() == TXT("font"))
	{
		return font.load_from_xml(_attr, _lc);
	}
	if (_attr->get_name() == TXT("textAt"))
	{
		textAt.load_from_string(_attr->get_as_string());
		return true;
	}
	if (_attr->get_name() == TXT("textScale"))
	{
		textScale.load_from_string(_attr->get_as_string());
		return true;
	}
	if (_attr->get_name() == TXT("colourSafe"))
	{
		colourSafe.load_from_string(_attr->get_as_string());
		return true;
	}
	if (_attr->get_name() == TXT("colourArmed"))
	{
		colourArmed.load_from_string(_attr->get_as_string());
		return true;
	}
	if (_attr->get_name() == TXT("armSound"))
	{
		armSound = _attr->get_as_name();
		return true;
	}
	if (_attr->get_name() == TXT("disarmSound"))
	{
		disarmSound = _attr->get_as_name();
		return true;
	}
	return base::read_parameter_from(_attr, _lc);
}

bool MineData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= font.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void MineData::prepare_to_unload()
{
	font.clear();

	base::prepare_to_unload();
}