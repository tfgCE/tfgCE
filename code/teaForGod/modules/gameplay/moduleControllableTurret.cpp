#include "moduleControllableTurret.h"

#include "..\custom\health\mc_health.h"

#include "..\..\game\game.h"
#include "..\..\game\gameDirector.h"

#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_text.h"

#include "..\..\..\framework\module\modules.h"
#include "..\..\..\framework\object\actor.h"
#include "..\..\..\framework\object\temporaryObject.h"

#include "..\..\..\core\vr\iVR.h"

#ifdef AN_CLANG
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

// module params
DEFINE_STATIC_NAME(rpm);
DEFINE_STATIC_NAME(appearAsDeadStoryFact);
DEFINE_STATIC_NAME(playerInControlVarID);
DEFINE_STATIC_NAME(canShootAttachedTo);
DEFINE_STATIC_NAME(cantShootAttachedTo);

// vars
DEFINE_STATIC_NAME(projectileSpeed);

// muzzle sockets
DEFINE_STATIC_NAME(muzzle_0);
DEFINE_STATIC_NAME(muzzle_1);
DEFINE_STATIC_NAME(muzzle_2);
DEFINE_STATIC_NAME(muzzle_3);
DEFINE_STATIC_NAME(muzzle_4);
DEFINE_STATIC_NAME(muzzle_5);

// input
DEFINE_STATIC_NAME(useEquipment);
DEFINE_STATIC_NAME(useEquipmentLeft);
DEFINE_STATIC_NAME(useEquipmentRight);

// sounds / temporary objects
DEFINE_STATIC_NAME(shoot);
DEFINE_STATIC_NAME_STR(shootAllAtOnce, TXT("shoot all at once"));
DEFINE_STATIC_NAME(muzzleFlash);

// localised strings
DEFINE_STATIC_NAME_STR(lsCharsHealth, TXT("chars; health"));

//

Name const& get_muzzle_socket(int _muzzleIdx)
{
	if (_muzzleIdx == 0) return NAME(muzzle_0);
	if (_muzzleIdx == 1) return NAME(muzzle_1);
	if (_muzzleIdx == 2) return NAME(muzzle_2);
	if (_muzzleIdx == 3) return NAME(muzzle_3);
	if (_muzzleIdx == 4) return NAME(muzzle_4);
	if (_muzzleIdx == 5) return NAME(muzzle_5);
	todo_implement(TXT("add more hardcoded muzzles"));
	return Name::invalid();
}

//

REGISTER_FOR_FAST_CAST(ModuleControllableTurret);

static Framework::ModuleGameplay* create_module(Framework::IModulesOwner* _owner)
{
	return new ModuleControllableTurret(_owner);
}

static Framework::ModuleData* create_module_data(::Framework::LibraryStored* _inLibraryStored)
{
	return new ModuleControllableTurretData(_inLibraryStored);
}

Framework::RegisteredModule<Framework::ModuleGameplay> & ModuleControllableTurret::register_itself()
{
	return Framework::Modules::gameplay.register_module(String(TXT("controllable turret")), create_module, create_module_data);
}

ModuleControllableTurret::ModuleControllableTurret(Framework::IModulesOwner* _owner)
: base( _owner )
{
}

ModuleControllableTurret::~ModuleControllableTurret()
{
}

void ModuleControllableTurret::setup_with(Framework::ModuleData const* _moduleData)
{
	base::setup_with(_moduleData);

	rpm = _moduleData->get_parameter<float>(this, NAME(rpm), rpm);
	rpm = max(1.0f, rpm);

	canShootAttachedTo = _moduleData->get_parameter<bool>(this, NAME(canShootAttachedTo), canShootAttachedTo);
	canShootAttachedTo = ! _moduleData->get_parameter<bool>(this, NAME(cantShootAttachedTo), !canShootAttachedTo);

	appearAsDeadStoryFact =_moduleData->get_parameter<Name>(this, NAME(appearAsDeadStoryFact), appearAsDeadStoryFact);
	playerInControlVar.set_name(_moduleData->get_parameter<Name>(this, NAME(playerInControlVarID), playerInControlVar.get_name()));
}

void ModuleControllableTurret::initialise()
{
	base::initialise();

	muzzles.clear();

	if (auto * ap = get_owner()->get_appearance())
	{
		while (true)
		{
			Framework::SocketID socket;
			socket.set_name(get_muzzle_socket(muzzles.get_size()));
			socket.look_up(ap->get_mesh(), AllowToFail);
			if (socket.is_valid())
			{
				Muzzle m;
				m.muzzleSocket = socket;
				muzzles.push_back(m);
			}
			else
			{
				// no more muzzles
				break;
			}
		}
	}
}

void ModuleControllableTurret::pre_process_controls()
{
	shootRequests = 0;
	shootRequestDir = 1;
}

void ModuleControllableTurret::process_controls(Hand::Type _hand, Framework::GameInput const& _controls, float _deltaTime)
{
	bool useVR = VR::IVR::get() != nullptr;
	
	Name useEquipment = useVR ? NAME(useEquipment) : (_hand == Hand::Left ? NAME(useEquipmentLeft) : NAME(useEquipmentRight));

	if (_controls.is_button_pressed(useEquipment))
	{
		++ shootRequests;
		shootRequestDir = _hand == Hand::Left ? -1 : 1;
	}
}

void ModuleControllableTurret::advance_post_move(float _deltaTime)
{
	base::advance_post_move(_deltaTime);

	if (muzzles.is_empty())
	{
		return;
	}

	{
		bool controlledByPlayer = false;
		bool fullyControlledByPlayer = false;
		if (auto* game = Game::get_as<Game>())
		{
			for_count(int, idxControl, 2)
			{
				auto& ptc = idxControl == 0? game->access_player_taken_control() : game->access_player();
				if (ptc.get_actor() == get_owner())
				{
					controlledByPlayer = true;
				}
				
				if (game->get_taking_controls_at() == 0.0f)
				{
					if (ptc.get_actor() == get_owner() &&
						! ptc.may_control_both())
					{
						fullyControlledByPlayer = true;
					}
				}
			}
		}

		if (!playerInControlVar.is_valid() &&
			playerInControlVar.get_name().is_valid())
		{
			playerInControlVar.look_up<float>(get_owner()->access_variables());
		}

		if (playerInControlVar.is_valid())
		{
			playerInControlVar.access<float>() = controlledByPlayer ? 1.0f : 0.0f;
		}

		{
			Optional<int> value;
			if (fullyControlledByPlayer)
			{
				if (!overlayStatus.is_shown())
				{
					overlayStatus.show(true);
					overlayStatus.set_prefix(NAME(lsCharsHealth));
					overlayStatus.set_highlight_colour(Colour::red);
					overlayStatus.set_highlight_at_value_or_below(0);
				}

				Optional<int> newValue;
				{
					auto* imo = get_owner();
					while (imo)
					{
						if (auto* h = imo->get_custom<CustomModules::Health>())
						{
							newValue = clamp(TypeConversions::Normal::f_i_closest(100.0f * h->get_health().div_to_float(h->get_max_health())), 0, 100);
							if (h->get_health().is_positive()) newValue = max(1, newValue.get());
							break;
						}
						imo = imo->get_presence()->get_attached_to();
					}
				}
				if (appearAsDeadStoryFact.is_valid())
				{
					if (auto* gd = GameDirector::get())
					{
						Concurrency::ScopedMRSWLockRead lockRead(gd->access_story_execution().access_facts_lock());
						if (gd->access_story_execution().get_facts().get_tag(appearAsDeadStoryFact))
						{
							newValue = 0;
						}
					}
				}
				value = newValue;
			}
			else
			{
				overlayStatus.show(false);
			}
			overlayStatus.update(value);
		}
	}

	timeSinceLastShot += _deltaTime;
	for_every(m, muzzles)
	{
		m->toToBeReady = max(0.0f, m->toToBeReady - _deltaTime);
		m->shootNow = false;
	}
	bool shootSomethingNow = false;

	float muzzleTimeToBeReady = 60.0f / rpm;
	if (shootRequests == 1)
	{
		float continuousShootInterval = muzzleTimeToBeReady / (float)muzzles.get_size();
		if (timeSinceLastShot >= continuousShootInterval)
		{
			for_count(int, i, 1)//muzzles.get_size())
			{
				int idx = mod(lastShotMuzzleIdx + (i + 1) * shootRequestDir, muzzles.get_size());
				auto& m = muzzles[idx];
				if (m.toToBeReady == 0.0f)
				{
					m.shootNow = true;
					shootSomethingNow = true;
					lastShotMuzzleIdx = idx;
					break;
				}
			}
		}
	}
	else if (shootRequests == 2)
	{
		bool allReady = true;
		for_every(m, muzzles)
		{
			if (m->toToBeReady > 0.0f)
			{
				allReady = false;
				break;
			}
		}
		if (allReady)
		{
			for_every(m, muzzles)
			{
				m->shootNow = true;
				shootSomethingNow = true;
			}
		}
	}

	if (shootSomethingNow)
	{
		if (auto* imo = get_owner())
		{
			Random::Generator rg;
			int shotCount = 0;
			Optional<Name> atMuzzleSocket;
			for_every(m, muzzles)
			{
				if (m->shootNow)
				{
					bool shot = false;
					if (auto* tos = imo->get_temporary_objects())
					{
						auto* projectile = tos->spawn(NAME(shoot), Framework::ModuleTemporaryObjects::SpawnParams().at_socket(m->muzzleSocket.get_name()));
						if (projectile)
						{
							// just in any case if we would be shooting from inside of a capsule
							if (auto* collision = projectile->get_collision())
							{
								collision->dont_collide_with_up_to_top_instigator(imo, 0.05f);
								if (!canShootAttachedTo)
								{
									collision->dont_collide_with_up_to_top_attached_to(imo, 0.2f);
								}
							}
							{
								float projectileSpeed = projectile->get_variables().get_value<float>(NAME(projectileSpeed), 10.0f);
								projectileSpeed = imo->get_variables().get_value<float>(NAME(projectileSpeed), projectileSpeed);
								Vector3 velocity = Vector3(0.0f, projectileSpeed, 0.0f);
								projectile->on_activate_set_relative_velocity(velocity);
								{
									Vector3 addAbsoluteOffsetLocation = Vector3::zero;
									Vector3 addAbsoluteVelocity = Vector3::zero;
									Transform placementWS = imo->get_presence()->get_placement();
									Transform prevPlacementWS = placementWS.to_world(imo->get_presence()->get_prev_placement_offset());
									float prevDeltaTime = imo->get_presence()->get_prev_placement_offset_delta_time();
									if (prevDeltaTime != 0.0f)
									{
										addAbsoluteOffsetLocation = placementWS.get_translation() - prevPlacementWS.get_translation();
										addAbsoluteVelocity = addAbsoluteOffsetLocation / prevDeltaTime;
									}
									projectile->on_activate_add_absolute_location_offset(-addAbsoluteOffsetLocation); // something we will miss with the movement this frame, we have to make it up
									projectile->on_activate_add_velocity(addAbsoluteVelocity);
								}
							}
							shot = true;
						}
						if (shot)
						{
							atMuzzleSocket = m->muzzleSocket.get_name();
							// depends on muzzle being symmetrical/round
							Transform relativePlacement(Vector3::zero, Rotator3(0.0f, 0.0f, rg.get_float(-180.0f, 180.0f)).to_quat());
							tos->spawn(NAME(muzzleFlash), Framework::ModuleTemporaryObjects::SpawnParams().at_socket(m->muzzleSocket.get_name()).at_relative_placement(relativePlacement));
						}
					}
					if (shot)
					{
						++shotCount;
						timeSinceLastShot = 0.0f;
						m->toToBeReady = muzzleTimeToBeReady;
					}
				}
			}
			if (shotCount)
			{
				if (auto* s = imo->get_sound())
				{
					if (shotCount > 1)
					{
						s->play_sound(NAME(shootAllAtOnce));
					}
					else
					{
						s->play_sound(NAME(shoot), atMuzzleSocket);
					}
				}
			}
		}
	}

	shootRequests = 0; // clear shooting request
}

//

REGISTER_FOR_FAST_CAST(ModuleControllableTurretData);

ModuleControllableTurretData::ModuleControllableTurretData(Framework::LibraryStored* _inLibraryStored)
: base(_inLibraryStored)
{
}

ModuleControllableTurretData::~ModuleControllableTurretData()
{
}

