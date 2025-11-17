#include "me_grenade.h"

#include "..\..\custom\mc_switchSidesHandler.h"

#include "..\..\..\teaForGodTest.h"

#include "..\..\..\game\damage.h"
#include "..\..\..\game\game.h"

#include "..\moduleEnergyQuantum.h"
#include "..\modulePilgrim.h"

#include "..\..\custom\mc_emissiveControl.h"
#include "..\..\custom\mc_pickup.h"
#include "..\..\custom\health\mc_health.h"

#include "..\..\..\..\framework\debug\previewGame.h"
#include "..\..\..\..\framework\display\display.h"
#include "..\..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\..\framework\display\displayText.h"
#include "..\..\..\..\framework\display\displayUtils.h"
#include "..\..\..\..\framework\module\modules.h"
#include "..\..\..\..\framework\module\moduleMovementSwitch.h"
#include "..\..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

using namespace TeaForGodEmperor;
using namespace ModuleEquipments;

//

// movement
DEFINE_STATIC_NAME(immovable);

// temporary objects
DEFINE_STATIC_NAME(explode);

// emissive
DEFINE_STATIC_NAME(armed);
DEFINE_STATIC_NAME_STR(armedActive, TXT("armed active"));

//

REGISTER_FOR_FAST_CAST(Grenade);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new Grenade(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new GrenadeData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & Grenade::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("grenade")), create_module, create_module_data);
}

Grenade::Grenade(Framework::IModulesOwner* _owner)
: base( _owner )
{
	allowEasyReleaseForAutoInterimEquipment = true;
	requiresDisplayExtra = true;
	rotatedDisplay = true;
}

Grenade::~Grenade()
{
}

void Grenade::display_extra(::Framework::Display* _display, bool _justStarted)
{
	Framework::DisplayUtils::clear_all(_display);
	tchar const * t = TXT("-");
	Colour colour = grenadeData->colourSafe;
	if (!safe)
	{
		colour = grenadeData->colourArmed;
		if (timer.is_set())
		{
			tchar const * times[] = { TXT("0"), TXT("1"), TXT("2"), TXT("3"), TXT("4"), TXT("5"), TXT("6"), TXT("7"), TXT("8"), TXT("9") };
			t = times[timer.get()];
		}
		else if (detonateOnCollision)
		{
			t = TXT("x");
		}
	}

	{
		auto* text = new Framework::DisplayDrawCommands::TextAt();
		text->text(t)
			->use_font(grenadeData->font.get())
			->at(grenadeData->textAt)
			->scale(grenadeData->textScale)
			->use_coordinates(Framework::DisplayCoordinates::Pixel)
			->use_colourise_ink(colour)
			;
		_display->add(text);
	}

	_display->draw_all_commands_immediately();
}

void Grenade::setup_with(Framework::ModuleData const * _moduleData)
{
	base::setup_with(_moduleData);

	grenadeData = fast_cast<GrenadeData>(_moduleData);
}

void Grenade::reset()
{
	base::reset();
}

void Grenade::initialise()
{
	base::initialise();
}

void Grenade::activate()
{
	base::activate();

	safe = true;
	armedBy = nullptr;

	timer.clear();
	detonateOnCollision = false;

	buttonHeldTime = 0.0f;

	interactiveButtonHandler.has_to_be_held();
	interactiveButtonHandler.initialise(get_owner(), grenadeData->buttonIdVar, grenadeData->onButtonIdVar);
}

void Grenade::advance_post_move(float _deltaTime)
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

	bool switchArmed = false;
	auto* switchSidesHandler = get_owner()->get_custom<CustomModules::SwitchSidesHandler>();

	if (switchSidesHandler)
	{
		bool newSwitchedSides = switchSidesHandler->get_switch_sides_to() != nullptr;
		switchArmed = !switchedSides.is_set() || switchedSides.get() != newSwitchedSides;
		switchedSides = newSwitchedSides;
	}

	if (heldNow || switchArmed)
	{
		timeToDetonate.clear();

		if (! switchSidesHandler)
		{
			switchArmed = interactiveButtonHandler.is_button_pressed();
		}

		if (switchArmed)
		{
			if (switchSidesHandler)
			{
				if (switchedSides.get())
				{
					safe = false;
					armedBy = switchSidesHandler->get_switch_sides_to();
					timer.clear();
					detonateOnCollision = true;
				}
				else
				{
					safe = true;
					armedBy = nullptr;
				}
			}
			else
			{
				float thresholdReset = 1.0f;
				float thresholdSet = 0.05f;
				float prevButtonHeldTime = buttonHeldTime;
				buttonHeldTime += _deltaTime;
				if (prevButtonHeldTime < thresholdSet && buttonHeldTime >= thresholdSet)
				{
					if (grenadeData->armSound.is_valid())
					{
						if (auto* s = get_owner()->get_sound())
						{
							s->play_sound(grenadeData->armSound);
						}
					}
					if (safe)
					{
						safe = false;
						if (auto* pilgrim = get_user())
						{
							armedBy = pilgrim->get_owner();
						}
						else
						{
							armedBy = nullptr;
						}
						timer = 3;
						detonateOnCollision = false;
					}
					else
					{
						if (!timer.is_set())
						{
							timer = 1;
							detonateOnCollision = false;
						}
						else
						{
							if (timer.get() < 5)
							{
								timer = timer.get() + 1;
								detonateOnCollision = false;
							}
							else
							{
								timer.clear();
								detonateOnCollision = true;
								detonateOnCollisionBlocked.clear();
							}
						}
					}
				}
				if (buttonHeldTime >= thresholdReset)
				{
					if (!safe)
					{
						if (grenadeData->disarmSound.is_valid())
						{
							if (auto* s = get_owner()->get_sound())
							{
								s->play_sound(grenadeData->disarmSound);
							}
						}
					}
					safe = true;
					armedBy = nullptr;
				}
			}
		}
		else
		{
			buttonHeldTime = 0.0f;
		}
	}
	else
	{
		if (!safe && !detonated)
		{
			bool detonate = false;
			if (!timeToDetonate.is_set() && timer.is_set())
			{
				timeToDetonate = (float)timer.get();
			}
			if (timeToDetonate.is_set())
			{
				timeToDetonate = timeToDetonate.get() - _deltaTime;
				if (timeToDetonate.get() <= 0.0f)
				{
					detonate = true;
				}
			}
			if (detonateOnCollision)
			{
				if (!detonateOnCollisionBlocked.is_set())
				{
					detonateOnCollisionBlocked = 0.1f;
				}
				detonateOnCollisionBlocked = detonateOnCollisionBlocked.get() - _deltaTime;
				if (detonateOnCollisionBlocked.get() <= 0.0f)
				{
					if (auto * c = get_owner()->get_collision())
					{
						detonate |= ! c->get_collided_with().is_empty();
					}
				}
			}

			if (detonate)
			{
				explode();
			}
		}
	}

	if (auto* e = get_owner()->get_custom<CustomModules::EmissiveControl>())
	{
		if (heldNow)
		{
			e->emissive_deactivate(NAME(armedActive));
			if (!safe)
			{
				e->emissive_activate(NAME(armed));
			}
			else
			{
				e->emissive_deactivate(NAME(armed));
			}
		}
		else
		{
			e->emissive_deactivate(NAME(armed));
			if (!safe)
			{
				e->emissive_activate(NAME(armedActive));
			}
			else
			{
				e->emissive_deactivate(NAME(armedActive));
			}
		}
	}

}

void Grenade::explode(Framework::IModulesOwner* _instigator)
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

bool Grenade::on_death(Damage const& _damage, DamageInfo const & _damageInfo)
{
	if (!detonated && (grenadeData->alwaysExplodeOnDeath || !safe) && !_damageInfo.peacefulDamage &&
		_damage.damageType != DamageType::Corrosion)
	{
		explode(_damageInfo.instigator.get());
		return false;
	}
	return base::on_death(_damage, _damageInfo);
}

void Grenade::advance_user_controls()
{
	base::advance_user_controls();

	if (!user)
	{
		return;
	}
}

Optional<Energy> Grenade::get_primary_state_value() const
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

REGISTER_FOR_FAST_CAST(GrenadeData);

GrenadeData::GrenadeData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

GrenadeData::~GrenadeData()
{
}

bool GrenadeData::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	return result;
}

bool GrenadeData::read_parameter_from(IO::XML::Attribute const * _attr, Framework::LibraryLoadingContext & _lc)
{
	if (_attr->get_name() == TXT("alwaysExplodeOnDeath"))
	{
		alwaysExplodeOnDeath = _attr->get_as_bool();
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

bool GrenadeData::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	result &= font.prepare_for_game_find(_library, _pfgContext, Framework::LibraryPrepareLevel::Resolve);

	return result;
}

void GrenadeData::prepare_to_unload()
{
	font.clear();

	base::prepare_to_unload();
}