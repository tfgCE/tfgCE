#include "gse_waitForMainEquipment.h"

#include "..\..\game\game.h"
#include "..\..\overlayInfo\overlayInfoSystem.h"
#include "..\..\overlayInfo\elements\oie_customDraw.h"
#include "..\..\modules\gameplay\moduleEquipment.h"
#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\gameplay\equipment\me_gun.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\system\video\video3dPrimitives.h"
#include "..\..\..\framework\modulesOwner\modulesOwner.h"
#include "..\..\..\framework\object\actor.h"

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// ids
DEFINE_STATIC_NAME(waitForMainEquipment);

// variable
DEFINE_STATIC_NAME(waitForMainEquipmentTime);

//

bool WaitForMainEquipment::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	if (auto* a = _node->get_attribute(TXT("hand")))
	{
		hand = Hand::parse(a->get_as_string());
	}

	deployed.load_from_xml(_node, TXT("deployed"));
	heldActively.load_from_xml(_node, TXT("heldActively"));
	enoughEnergyToShoot.load_from_xml(_node, TXT("enoughEnergyToShoot"));

	return result;
}

float WaitForMainEquipment::get_energy_to_shoot_pt()
{
	float atPt = 0.0f;

	if (auto* game = Game::get_as<Game>())
	{
		if (auto* actor = game->access_player().get_actor())
		{
			if (auto* pilgrim = actor->get_gameplay_as<ModulePilgrim>())
			{
				for_count(int, iHandIdx, Hand::MAX)
				{
					Hand::Type hand = (Hand::Type)iHandIdx;
					if (auto* gunImo = pilgrim->get_main_equipment(hand))
					{
						if (auto* gun = gunImo->get_gameplay_as<ModuleEquipments::Gun>())
						{
							Energy inChamber = gun->get_chamber();
							Energy minShoot = gun->get_chamber_size();
							if (inChamber >= minShoot)
							{
								atPt = 1.0f;
							}
							else if (!minShoot.is_zero())
							{
								atPt = max(atPt, inChamber.as_float() / minShoot.as_float());
							}
						}
					}
				}
			}
		}
	}

	return atPt;
}

Framework::GameScript::ScriptExecutionResult::Type WaitForMainEquipment::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	bool okToContinue = true;
	if (auto* game = Game::get_as<Game>())
	{
		if (auto* actor = game->access_player().get_actor())
		{
			if (auto* pilgrim = actor->get_gameplay_as<ModulePilgrim>())
			{
				if (deployed.is_set())
				{
					if (hand == Hand::MAX)
					{
						okToContinue = false;
						if (deployed.get())
						{
							if ((pilgrim->get_hand_equipment(Hand::Left) == pilgrim->get_main_equipment(Hand::Left)) ||
								(pilgrim->get_hand_equipment(Hand::Right) == pilgrim->get_main_equipment(Hand::Right)))
							{
								okToContinue = true;
							}
						}
						else
						{
							if ((pilgrim->get_hand_equipment(Hand::Left) != pilgrim->get_main_equipment(Hand::Left)) &&
								(pilgrim->get_hand_equipment(Hand::Right) != pilgrim->get_main_equipment(Hand::Right)))
							{
								okToContinue = true;
							}
						}
					}
					else
					{
						todo_important(TXT("implement"));
					}
				}
				if (heldActively.is_set())
				{
					if (hand == Hand::MAX)
					{
						okToContinue = false;
						if (heldActively.get())
						{
							if (pilgrim->is_actively_holding_in_hand(Hand::Left) ||
								pilgrim->is_actively_holding_in_hand(Hand::Right))
							{
								okToContinue = true;
							}
						}
						else
						{
							todo_important(TXT("implement, what should it mean?"));
						}
					}
					else
					{
						todo_important(TXT("implement"));
					}
				}
				if (enoughEnergyToShoot.is_set())
				{
					okToContinue = false;
					if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
					{
						if (auto* ois = OverlayInfo::System::get())
						{
							{
								auto* c = new OverlayInfo::Elements::CustomDraw();

								c->with_id(NAME(waitForMainEquipment));
								c->set_cant_be_deactivated_by_system();

								c->with_draw(
									[](float _active, float _pulse, Colour const& _colour)
								{
									float atPt = get_energy_to_shoot_pt();

									Colour colour = _colour.mul_rgb(_pulse).with_alpha(_active);

									::System::Video3DPrimitives::ring_xz(colour, Vector3::zero, 0.7f, 1.0f, _active < 1.0f, 0.0f, atPt, 0.05f);
								});

								Rotator3 offset = Rotator3(-10.0f, 0.0f, 0.0f);
								if (TutorialSystem::check_active())
								{
									TutorialSystem::configure_oie_element(c, offset);
								}
								else
								{
									c->with_location(OverlayInfo::Element::Relativity::RelativeToAnchor);
									c->with_rotation(OverlayInfo::Element::Relativity::RelativeToAnchor, offset);
									c->with_distance(10.0f);
								}

								ois->add(c);
							}
						}
					}
					if (get_energy_to_shoot_pt() >= 1.0f)
					{
						okToContinue = true;
					}
				}
			}
		}
	}

	if (timeLimit.is_set())
	{
		float& waitVar = _execution.access_variables().access<float>(NAME(waitForMainEquipmentTime));

		if (is_flag_set(_flags, Framework::GameScript::ScriptExecution::Entered))
		{
			waitVar = timeLimit.get();
		}
		else
		{
			float deltaTime = TutorialSystem::check_active() ? ::System::Core::get_raw_delta_time() : ::System::Core::get_delta_time();
			waitVar -= deltaTime;
		}
		if (waitVar <= 0.0f)
		{
			okToContinue = true;
		}
	}

	if (okToContinue)
	{
		return Framework::GameScript::ScriptExecutionResult::Continue;
	}
	else
	{
		return Framework::GameScript::ScriptExecutionResult::Repeat;
	}
}

void WaitForMainEquipment::interrupted(Framework::GameScript::ScriptExecution& _execution) const
{
	base::interrupted(_execution);
	if (auto* ois = OverlayInfo::System::get())
	{
		ois->deactivate(NAME(waitForMainEquipment), true);
	}
}

