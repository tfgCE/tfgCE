#include "aiLogic_pilgrimHumanForearmDisplay.h"

#include "..\..\game\gameDirector.h"

#include "..\..\modules\gameplay\modulePilgrim.h"
#include "..\..\modules\custom\health\mc_healthRegen.h"

#include "..\..\..\framework\framework.h"
#include "..\..\..\framework\ai\aiMindInstance.h"
#include "..\..\..\framework\display\display.h"
#include "..\..\..\framework\display\displayDrawCommands.h"
#include "..\..\..\framework\display\displayUtils.h"
#include "..\..\..\framework\game\bullshotSystem.h"
#include "..\..\..\framework\modulesOwner\modulesOwnerImpl.inl"
#include "..\..\..\framework\module\custom\mc_display.h"
#include "..\..\..\framework\module\moduleSound.h"

#include "..\..\..\core\system\core.h"
#include "..\..\..\core\system\video\video3dPrimitives.h"

#ifdef AN_CLANG
#include "..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace Logics;

//

// object variables
DEFINE_STATIC_NAME(hand);
	// values
	DEFINE_STATIC_NAME(left);
	DEFINE_STATIC_NAME(right);

// sockets
DEFINE_STATIC_NAME(guidance);
DEFINE_STATIC_NAME(guidanceOrigin);

// sounds
DEFINE_STATIC_NAME(guidanceActivated);

// bullshot system allowances
DEFINE_STATIC_NAME_STR(bsBullshotGuidance, TXT("bullshot guidance"));

//

REGISTER_FOR_FAST_CAST(PilgrimHumanForearmDisplay);

PilgrimHumanForearmDisplay::PilgrimHumanForearmDisplay(::Framework::AI::MindInstance* _mind, ::Framework::AI::LogicData const* _logicData)
: base(_mind, _logicData, execute_logic)
, pilgrimHumanForearmDisplayData(fast_cast<PilgrimHumanForearmDisplayData>(_logicData))
{
	ammoColour = pilgrimHumanForearmDisplayData->ammoColour;
	healthColour = pilgrimHumanForearmDisplayData->healthColour;
	guidanceColour = pilgrimHumanForearmDisplayData->guidanceColour;

	ammoUpColour = Colour::lerp(0.5f, ammoColour, Colour::white);
	healthUpColour = Colour::lerp(0.5f, healthColour, Colour::white);

	colours.health = healthColour;
	colours.healthBackup = healthColour;
	colours.mainStorage = ammoColour;
	colours.magazine = ammoColour;
	colours.chamber = ammoColour;
	colours.itemHealth = healthColour;
	colours.itemAmmo = ammoColour;
}

PilgrimHumanForearmDisplay::~PilgrimHumanForearmDisplay()
{
	if (display)
	{
		display->set_on_advance_display(nullptr, nullptr);
	}
}

void PilgrimHumanForearmDisplay::advance(float _deltaTime)
{
	base::advance(_deltaTime);

	prepare_proposed_elements(_deltaTime);

	update_visible_elements();
}

void PilgrimHumanForearmDisplay::prepare_proposed_elements(float _deltaTime)
{
	proposedElements.clear();

	if (display && pilgrimOwner.get())
	{
		Range2 screen = Range2::empty;
		if (!proposedElemenstBackground.is_empty())
		{
			screen = proposedElemenstBackground;
		}
		else
		{
			Range2 screenPt = pilgrimHumanForearmDisplayData->screenAsRightPt;

			Vector2 lb = display->get_left_bottom_of_screen();
			Vector2 rt = display->get_right_top_of_screen();

			Vector2 ulb = lb;
			Vector2 urt = rt;
			ulb.x = lerp(screenPt.x.min, lb.x, rt.x);
			ulb.y = lerp(screenPt.y.min, lb.y, rt.y);
			urt.x = lerp(screenPt.x.max, lb.x, rt.x);
			urt.y = lerp(screenPt.y.max, lb.y, rt.y);

			if (hand == Hand::Left)
			{
				// on the other side
				swap(ulb.x, urt.x);
				ulb.x = lb.x + rt.x - ulb.x;
				urt.x = lb.x + rt.x - urt.x;
			}

			screen.include(ulb);
			screen.include(urt);

			proposedElemenstBackground = screen;
		}
		auto* gd = GameDirector::get();
		if (gd)
		{
			mainActive = gd->get_ammo_status();
			healthActive = gd->get_health_status();
			equipmentActive = gd->get_ammo_status();
		}

		bool simplifiedDisplay = GameSettings::get().difficulty.regenerateEnergy;
		bool noTopBar = false;

		if (auto* h = pilgrimOwner->get_custom<CustomModules::Health>())
		{
			if (h->is_super_health_storage())
			{
				noTopBar = true;
			}
		}


		float spacing = (float)TypeConversions::Normal::f_i_closest((screen.x.length() + 1.0f) / 11.0f);
		float spacingPlus = spacing + 1.0f;
		float barCount = simplifiedDisplay? 2.0f : 3.0f;
		float barWidth = (float)TypeConversions::Normal::f_i_cells((screen.x.length() + 1.0f - spacing * (barCount - 1.0f)) / barCount);

		Range2 available = screen;

		Range2 guidanceBar = available;
		guidanceBar.y.min = guidanceBar.y.max - round(guidanceBar.x.length() * 0.25f) + 1.0f;
		
		bool showGuidance = ! simplifiedDisplay;

		if (auto* gd = GameDirector::get_active())
		{
			if (gd->is_forearm_navigation_blocked())
			{
				showGuidance = false;
			}
		}

		float guidanceTime = ::System::Core::get_timer_mod(2.0f);

#ifdef AN_ALLOW_BULLSHOTS
		if (Framework::BullshotSystem::is_setting_active(NAME(bsBullshotGuidance)))
		{
			showGuidance = true;
			guidanceTime = 0.0f;
			guidance.guidanceDirOS = Vector3::zAxis;
		}
#endif

		//if (showGuidance)
		if (!simplifiedDisplay)
		{
			available.y.max = guidanceBar.y.min - spacingPlus;
		}

		Range2 topBar = available;
		topBar.y.min = topBar.y.max - topBar.x.length();

		Range2 leftBar = available;
		Range2 centreBar = available;
		Range2 rightBar = available;

		leftBar.x.max = leftBar.x.min + (barWidth - 1.0f);
		rightBar.x.min = rightBar.x.max - (barWidth - 1.0f);

		centreBar.x.min = leftBar.x.max + spacingPlus;
		centreBar.x.max = rightBar.x.min - spacingPlus;

		if (noTopBar)
		{
		}
		else if (!simplifiedDisplay)
		{
			leftBar.y.max = topBar.y.min - spacingPlus;
			rightBar.y.max = topBar.y.min - spacingPlus;

			centreBar.y.max = topBar.y.min - 1.0f;
		}

		Range2 mainStorageBar = hand == Hand::Right ? leftBar : rightBar;
		Range2 itemStorageBar = hand == Hand::Right ? rightBar : leftBar;

		Range2 simplifiedHealthBar = hand == Hand::Right ? leftBar : rightBar;
		Range2 simplifiedEnergyBar = hand == Hand::Right ? rightBar : leftBar;

		if (noTopBar && !simplifiedDisplay)
		{
			simplifiedHealthBar = centreBar;
		}

		if (simplifiedDisplay)
		{
			itemStorageBar = simplifiedEnergyBar;
		}

		auto* mp = pilgrimOwner->get_gameplay_as<ModulePilgrim>();

		if (!mp)
		{
			while (pilgrimOwner->get_presence()->is_attached())
			{
				pilgrimOwner = pilgrimOwner->get_presence()->get_attached_to();
				mp = pilgrimOwner->get_gameplay_as<ModulePilgrim>();
				if (mp)
				{
					break;
				}
			}
		}

		if (mp)
		{
			// guidance
			if (showGuidance && (!gd || !gd->is_guidance_status_blocked()))
			{
				guidance.update(pilgrimOwner.get(), mp->get_hand(hand), _deltaTime, true, NAME(guidance), NAME(guidanceOrigin), 1.0f);

				{
					bool newGuidanceActive = !guidance.guidanceDirOS.is_zero();
					if (newGuidanceActive && !guidanceActive)
					{
						if (auto* s = pilgrimOwner->get_sound())
						{
							//s->play_sound(NAME(guidanceActivated)); // replaced with pilgrim overlay info spoken lines
						}
					}
					guidanceActive = newGuidanceActive;
				}

				if (guidance.guidanceType == Guidance::Strength)
				{
					if (guidance.guidanceDirOS.is_zero())
					{
						float at = guidanceTime < 1.0f ? 0.25f : 0.75f;
						Range2 gb = guidanceBar;
						gb.x.min = guidanceBar.x.get_at(at - 0.05f);
						gb.x.max = guidanceBar.x.get_at(at + 0.05f);
						proposedElements.push_back(VisibleElement(guidanceColour, gb));
					}
					else
					{
						if (abs(guidance.guidanceDirOS.z) > 0.7f)
						{
							// up or down
							float at = mod(guidanceTime, 1.0f);
							if (guidance.guidanceDirOS.z < 0.0f)
							{
								at = 1.0f - at;
							}

							Range2 gb = guidanceBar;
							gb.y.min = guidanceBar.y.get_at(0.0f + 0.8f * at);
							gb.y.max = guidanceBar.y.get_at(0.2f + 0.8f * at);
							proposedElements.push_back(VisibleElement(guidanceColour, gb));
						}
						else
						{
							float angle = Rotator3::get_yaw(guidance.guidanceDirOS); // -180 - 180
							float absAngle = abs(angle);
							float atTarget = clamp(1.0f - absAngle / 180.0f, 0.0f, 1.0f);
							//atTarget = sqr(atTarget);
							
							float xHalf = 0.5f * (1.0f - atTarget);
							Range2 gb = guidanceBar;
							gb.x.min = guidanceBar.x.get_at(xHalf);
							gb.x.max = guidanceBar.x.get_at(1.0f - xHalf);
							proposedElements.push_back(VisibleElement(guidanceColour, gb));
						}
					}
				}
				else
				{
					an_assert(guidance.guidanceType == Guidance::NoGuidance, TXT("implement!"));
				}
			}

			float newMainStoragePt = mp->get_hand_energy_storage((hand)).div_to_float(mp->get_hand_energy_max_storage((hand)));

			if (auto* eq = mp->get_hand_equipment(hand))
			{
				if (auto* g = eq->get_gameplay_as<ModuleEquipments::Gun>())
				{
					Range2 chamberIndicator = itemStorageBar;
					Range2 magazineBar = itemStorageBar;
					float chamberAt = g->get_chamber().div_to_float(g->get_chamber_size());
					if (g->is_using_magazine())
					{
						float pt = max(chamberIndicator.x.length() / chamberIndicator.y.length(), g->get_chamber_size().div_to_float(g->get_magazine_size() + g->get_chamber_size()));
						chamberIndicator.y.min = (float)TypeConversions::Normal::f_i_closest(chamberIndicator.y.get_at(1.0f - pt));
						magazineBar.y.max = chamberIndicator.y.min - spacingPlus;

						if (equipmentActive > 0.0f)
						{
							if (g->get_magazine().is_positive())
							{
								float usePt = g->get_magazine().div_to_float(g->get_magazine_size()) * equipmentActive;
								if (usePt > prevValues.magazinePt.get(0.0f)) colours.magazine = ammoUpColour;
								proposedElements.push_back(VisibleElement(colours.magazine, magazineBar.mul_pt(0.0f, 1.0f, 1.0f - usePt, 1.0f)));
								prevValues.magazinePt = usePt;
							}
							else
							{
								prevValues.magazinePt = 0.0f;
								colours.magazine = ammoColour;
							}
							if (g->get_chamber().is_positive())
							{
								float usePt = chamberAt * equipmentActive;
								if (usePt > prevValues.chamberPt.get(0.0f) && usePt == 1.0f) colours.chamber = ammoUpColour; else if (!g->is_chamber_ready()) colours.chamber = ammoColour.mul_rgb(0.5f);
								proposedElements.push_back(VisibleElement(colours.chamber, chamberIndicator.mul_pt(0.0f, 1.0f, 0.0f, chamberAt)));
								prevValues.chamberPt= usePt;
							}
							else
							{
								prevValues.chamberPt = 0.0f;
								colours.chamber = ammoColour;
							}
						}
						else
						{
							prevValues.magazinePt.clear();
							prevValues.chamberPt.clear();
							colours.magazine = ammoColour;
							colours.chamber = ammoColour;
						}
					}
					else
					{
						if (equipmentActive > 0.0f)
						{
							if (g->get_chamber().is_positive())
							{
								float usePt = chamberAt * equipmentActive;
								if (usePt > prevValues.chamberPt.get(0.0f) && usePt == 1.0f) colours.chamber = ammoUpColour; else if (!g->is_chamber_ready()) colours.chamber = ammoColour.mul_rgb(0.5f);
								proposedElements.push_back(VisibleElement(colours.chamber, chamberIndicator.mul_pt(0.0f, 1.0f, 0.0f, usePt)));
								prevValues.chamberPt = usePt;
							}
							else
							{
								prevValues.magazinePt.clear();
								colours.magazine = ammoColour;
								prevValues.chamberPt = 0.0f;
								colours.chamber = ammoColour;
							}
						}
						else
						{
							prevValues.magazinePt.clear();
							prevValues.chamberPt.clear();
							colours.magazine = ammoColour;
							colours.chamber = ammoColour;
						}
					}

					newMainStoragePt = clamp((mp->get_hand_energy_storage((hand)) - g->get_chamber() - g->get_magazine()).div_to_float(mp->get_hand_energy_max_storage((hand))), 0.0f, 1.0f);
					
					prevValues.itemHealthPt.clear();
					prevValues.itemAmmoPt.clear();
					colours.itemHealth = healthColour;
					colours.itemAmmo = ammoColour;
				}
				else
				{
					if (auto* h = IEnergyTransfer::get_energy_transfer_module_from(eq, EnergyType::Ammo))
					{
						float pt = equipmentActive * h->calculate_total_energy_available(EnergyType::Ammo).div_to_float(h->calculate_total_max_energy_available(EnergyType::Ammo));
						if (pt > prevValues.itemAmmoPt.get(0.0f)) colours.itemAmmo = ammoUpColour;
						if (pt > 0.0f) proposedElements.push_back(VisibleElement(colours.itemAmmo, itemStorageBar.mul_pt(0.0f, 1.0f, 1.0f - pt, 1.0f)));
						prevValues.itemAmmoPt = pt;
						prevValues.itemHealthPt.clear();
						colours.itemHealth = healthColour;
					}
					else if (auto* h = IEnergyTransfer::get_energy_transfer_module_from(eq, EnergyType::Health))
					{
						float pt = equipmentActive * h->calculate_total_energy_available(EnergyType::Health).div_to_float(h->calculate_total_max_energy_available(EnergyType::Health));
						if (pt > prevValues.itemHealthPt.get(0.0f)) colours.itemHealth = healthUpColour;
						if (pt > 0.0f) proposedElements.push_back(VisibleElement(colours.itemHealth, itemStorageBar.mul_pt(0.0f, 1.0f, 1.0f - pt, 1.0f)));
						prevValues.itemHealthPt = pt;
						prevValues.itemAmmoPt.clear();
						colours.itemAmmo = ammoColour;
					}
					prevValues.magazinePt.clear();
					prevValues.chamberPt.clear();
					colours.magazine = ammoColour;
					colours.chamber = ammoColour;
				}
			}

			if (!simplifiedDisplay)
			{
				mainStoragePt = blend_to_using_speed_based_on_time(mainStoragePt, newMainStoragePt, 0.8f, _deltaTime);
				if (mainActive * mainStoragePt > 0.0f)
				{
					float pt = mainActive * mainStoragePt;
					if (pt > prevValues.mainStoragePt.get(0.0f)) colours.mainStorage = ammoUpColour;
					proposedElements.push_back(VisibleElement(colours.mainStorage, mainStorageBar.mul_pt(0.0f, 1.0f, 1.0f - pt, 1.0f)));
					prevValues.mainStoragePt = pt;
				}
				else
				{
					prevValues.mainStoragePt.clear();
					colours.mainStorage = ammoColour;
				}
			}
		}

		if (auto* hr = pilgrimOwner->get_custom<CustomModules::HealthRegen>())
		{
			if (healthActive > 0.0f)
			{
				if (!simplifiedDisplay && !noTopBar)
				{
					if (hr->get_health_backup().is_positive())
					{
						float pt = healthActive * hr->get_health_backup().div_to_float(hr->get_max_health_backup());
						if (pt > prevValues.healthBackupPt.get(0.0f)) colours.healthBackup = healthUpColour;
						proposedElements.push_back(VisibleElement(colours.healthBackup, centreBar.mul_pt(0.0f, 1.0f, 1.0f - pt, 1.0f)));
						prevValues.healthBackupPt = pt;
					}
					else
					{
						prevValues.healthBackupPt.clear();
						colours.healthBackup = healthColour;
					}
				}
				if (hr->get_health().is_positive())
				{
					float pt = healthActive * hr->get_health().div_to_float(hr->get_max_health());
					if (pt > prevValues.healthPt.get(0.0f)) colours.health = healthUpColour;
					proposedElements.push_back(VisibleElement(colours.health, (simplifiedDisplay || noTopBar ? simplifiedHealthBar : topBar).mul_pt(0.0f, 1.0f, 0.0f, pt)));
					prevValues.healthPt = pt;
				}
				else
				{
					prevValues.healthPt.clear();
					colours.health = healthColour;
				}
			}
			else
			{
				prevValues.healthPt.clear();
				prevValues.healthBackupPt.clear();
				colours.health = healthColour;
				colours.healthBackup = healthColour;
			}
		}

		float const blendTime = 0.5f;
		colours.health = blend_to_using_time(colours.health, healthColour, blendTime, _deltaTime);
		colours.healthBackup = blend_to_using_time(colours.healthBackup, healthColour, blendTime, _deltaTime);
		colours.mainStorage = blend_to_using_time(colours.mainStorage, ammoColour, blendTime, _deltaTime);
		colours.magazine = blend_to_using_time(colours.magazine, ammoColour, blendTime, _deltaTime);
		colours.chamber = blend_to_using_time(colours.chamber, ammoColour, blendTime, _deltaTime);
		colours.itemHealth = blend_to_using_time(colours.itemHealth, healthColour, blendTime, _deltaTime);
		colours.itemAmmo = blend_to_using_time(colours.itemAmmo, ammoColour, blendTime, _deltaTime);
	}
}

void PilgrimHumanForearmDisplay::update_visible_elements()
{
	bool updateDisplay = false;

	{
		::Concurrency::ScopedSpinLock lock(visibleElementsLock);
		if (visibleElements != proposedElements)
		{
			visibleElements = proposedElements;
			updateDisplay = true;
		}
	}

	if (updateDisplay && display)
	{
		display->drop_all_draw_commands();
		if (proposedElemenstBackground.is_empty())
		{
			display->add(new ::Framework::DisplayDrawCommands::CLS());
		}

		display->add((new Framework::DisplayDrawCommands::Custom())->use_simple(
			[this](Framework::Display* _display, ::System::Video3D* _v3d)
			{
				if (!proposedElemenstBackground.is_empty())
				{
					::System::Video3DPrimitives::fill_rect_2d(_display->get_current_paper(),
						TypeConversions::Normal::f_i_closest(proposedElemenstBackground.bottom_left() - Vector2::half).to_vector2() + Vector2::half,
						TypeConversions::Normal::f_i_closest(proposedElemenstBackground.top_right() - Vector2::half).to_vector2() + Vector2::half);
				}
				::Concurrency::ScopedSpinLock lock(visibleElementsLock);
				for_every(ve, visibleElements)
				{
					if (ve->rect.is_set())
					{
						::System::Video3DPrimitives::fill_rect_2d(ve->colour.get(_display->get_current_ink()),
							TypeConversions::Normal::f_i_closest(ve->rect.get().bottom_left() - Vector2::half).to_vector2() + Vector2::half,
							TypeConversions::Normal::f_i_closest(ve->rect.get().top_right() - Vector2::half).to_vector2() + Vector2::half);
					}
				}
			}));
	}
}

void PilgrimHumanForearmDisplay::learn_from(SimpleVariableStorage & _parameters)
{
	base::learn_from(_parameters);

	an_assert(!display);
	display = nullptr;
	hand = Hand::MAX;
	if (auto* mind = get_mind())
	{
		if (auto* imo = mind->get_owner_as_modules_owner())
		{
			pilgrimOwner = imo;
			while (pilgrimOwner->get_presence()->is_attached())
			{
				pilgrimOwner = pilgrimOwner->get_presence()->get_attached_to();
			}
			if (auto* displayModule = imo->get_custom<::Framework::CustomModules::Display>())
			{
				display = displayModule->get_display();
				display->always_draw_commands_immediately();
				display->add(new ::Framework::DisplayDrawCommands::CLS());
			}
			{
				Name handName = imo->get_variables().get_value(NAME(hand), Name::invalid());
				if (handName == NAME(left))
				{
					hand = Hand::Left;
				}
				if (handName == NAME(right))
				{
					hand = Hand::Right;
				}
			}
		}
	}
	if (hand >= Hand::MAX ||
		hand < 0)
	{
		error(TXT("no hand provided (right or left"));
	}
}

LATENT_FUNCTION(PilgrimHumanForearmDisplay::execute_logic)
{
	LATENT_SCOPE();

	LATENT_PARAM_NOT_USED(::Framework::AI::MindInstance*, mind); // auto
	LATENT_PARAM_NOT_USED(::Framework::AI::Logic*, logic); // auto
	LATENT_END_PARAMS();

	LATENT_BEGIN_CODE();

	LATENT_WAIT(Random::get_float(0.1f, 0.6f));

#ifdef AN_DEVELOPMENT
	while (Framework::is_preview_game())
	{
		LATENT_YIELD();
	}
#endif

	while (true)
	{
		LATENT_YIELD();
	}

	LATENT_ON_BREAK();

	LATENT_END_CODE();
	LATENT_RETURN();
}

//

REGISTER_FOR_FAST_CAST(PilgrimHumanForearmDisplayData);

bool PilgrimHumanForearmDisplayData::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	ammoColour.load_from_xml_child_or_attr(_node, TXT("ammoColour"));
	healthColour.load_from_xml_child_or_attr(_node, TXT("healthColour"));
	guidanceColour.load_from_xml_child_or_attr(_node, TXT("guidanceColour"));

	for_every(n, _node->children_named(TXT("screenAsRightPt")))
	{
		screenAsRightPt.load_from_xml(n);
	}

	return result;
}

bool PilgrimHumanForearmDisplayData::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

