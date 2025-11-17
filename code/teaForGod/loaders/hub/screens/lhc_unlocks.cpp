#include "lhc_unlocks.h"

#include "lhc_messageSetBrowser.h"

#include "..\loaderHub.h"
#include "..\screens\lhc_question.h"
#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_customDraw.h"
#include "..\widgets\lhw_list.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\playerSetup.h"
#include "..\..\..\library\gameDefinition.h"
#include "..\..\..\library\library.h"
#include "..\..\..\utils\reward.h"

#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\..\framework\video\font.h"
#include "..\..\..\..\framework\video\texturePart.h"

#include "..\..\..\..\core\system\video\video3dPrimitives.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsNoUnlocksLong, TXT("hub; unlocks; no unlocks available long"));
DEFINE_STATIC_NAME_STR(lsSelectToUnlockLong, TXT("hub; unlocks; select to unlock long"));
DEFINE_STATIC_NAME_STR(lsSelectToUnlock, TXT("hub; unlocks; select to unlock"));
DEFINE_STATIC_NAME_STR(lsUnlock, TXT("hub; unlocks; unlock"));
DEFINE_STATIC_NAME_STR(lsUnlockBuy, TXT("hub; unlocks; unlock; buy"));
	DEFINE_STATIC_NAME(cost);
DEFINE_STATIC_NAME_STR(lsUnlocked, TXT("hub; unlocks; unlocked"));
DEFINE_STATIC_NAME_STR(lsCurrentXP, TXT("hub; unlocks; current xp"));
	DEFINE_STATIC_NAME(xpToSpend);
DEFINE_STATIC_NAME_STR(lsUnlockedCount, TXT("hub; unlocks; unlocked count"));
DEFINE_STATIC_NAME_STR(lsUnlockedCountBuy, TXT("hub; unlocks; unlocked count; buy"));
	DEFINE_STATIC_NAME(unlocked);
	DEFINE_STATIC_NAME(limit);
DEFINE_STATIC_NAME_STR(lsEXMNotePermanent, TXT("hub; unlocks; exm note; permanent"));
DEFINE_STATIC_NAME_STR(lsEXMNoteActive, TXT("hub; unlocks; exm note; active"));
DEFINE_STATIC_NAME_STR(lsEXMNotePassive, TXT("hub; unlocks; exm note; passive"));
DEFINE_STATIC_NAME_STR(lsRunInProgress, TXT("hub; unlocks; run in progress"));
DEFINE_STATIC_NAME_STR(lsRunInProgressEndRun, TXT("hub; unlocks; abort run"));
DEFINE_STATIC_NAME_STR(lsRunInProgressEndRunQuestion, TXT("hub; unlocks; abort run; question"));

DEFINE_STATIC_NAME_STR(lsCharsExperience, TXT("chars; experience"));
DEFINE_STATIC_NAME_STR(lsCharsMeritPoints, TXT("chars; merit points"));

// ids
DEFINE_STATIC_NAME(idUnlocksHelp);
DEFINE_STATIC_NAME(idAvailable);
DEFINE_STATIC_NAME(idUnlockHistory);
DEFINE_STATIC_NAME(idSelectedEXMView);
DEFINE_STATIC_NAME(idSelectedEXMDesc);
DEFINE_STATIC_NAME(idSelectedEXMUnlockedCount);
DEFINE_STATIC_NAME(idSelectedEXMNote);
DEFINE_STATIC_NAME(idSelectedEXMXPMP);
DEFINE_STATIC_NAME(idSelectedEXMUnlock);
DEFINE_STATIC_NAME(idSelectedEXMUnlockWhole);
DEFINE_STATIC_NAME(idSelectedEXMDescWhole);

// global references
DEFINE_STATIC_NAME_STR(grActiveEXMLineModel, TXT("pi.ov.in; active exm line model"));
DEFINE_STATIC_NAME_STR(grPassiveEXMLineModel, TXT("pi.ov.in; passive exm line model"));
DEFINE_STATIC_NAME_STR(grPermanentEXMLineModel, TXT("pi.ov.in; permanent exm line model"));
DEFINE_STATIC_NAME_STR(grPilgrimageLineModel, TXT("pi.ov.in; pilgrimage line model"));
DEFINE_STATIC_NAME_STR(grHelpEXMsSimpleRules, TXT("hub; help; unlocks; exms; simple rules"));
DEFINE_STATIC_NAME_STR(grHelpEXMsComplexRules, TXT("hub; help; unlocks; exms; complex rules"));

//

using namespace Loader;
using namespace HubScreens;

//

class HubScreens::EXMInfo
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(EXMInfo);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	Name exm;
	bool unlocked = false;
	TeaForGodEmperor::EXMType const* exmType = nullptr;
	TeaForGodEmperor::LineModel const* borderLineModel = nullptr;
	TeaForGodEmperor::LineModel const* lineModel = nullptr;
	int unlockedCount = 0;
	int unlockLimit = 0;

	EXMInfo() {}
	EXMInfo(Unlocks const* _unlocks, Name const& _exm, bool _unlocked)
	: exm(_exm)
	, unlocked(_unlocked)
	, exmType(TeaForGodEmperor::EXMType::find(_exm))
	{
		if (exmType)
		{
			lineModel = exmType->get_line_model_for_display();
			if (exmType->is_permanent())
			{
				borderLineModel = _unlocks->permanentEXMBorderLineModel.get();
			}
			else if (exmType->is_active())
			{
				borderLineModel = _unlocks->activeEXMBorderLineModel.get();
			}
			else
			{
				borderLineModel = _unlocks->passiveEXMBorderLineModel.get();
			}
		}
		update();
	}

	void update()
	{
		if (exmType)
		{
			if (exmType->is_permanent())
			{
				unlockLimit = exmType->get_permanent_limit();
			}
			else
			{
				unlockLimit = 1;
			}
		}
	}
};
REGISTER_FOR_FAST_CAST(EXMInfo);
//

class HubScreens::CustomUpgradeInfo
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(CustomUpgradeInfo);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	bool unlocked = false;
	bool canBeUnlocked = true;
	TeaForGodEmperor::CustomUpgradeType const* customUpgradeType = nullptr;
	TeaForGodEmperor::LineModel const* borderLineModel = nullptr;
	TeaForGodEmperor::LineModel const* lineModel = nullptr;
	int unlockedCount = 0;
	int unlockLimit = 0;

	CustomUpgradeInfo() {}
	CustomUpgradeInfo(Unlocks const* _unlocks, TeaForGodEmperor::CustomUpgradeType const* _customUpgradeType)
	: customUpgradeType(_customUpgradeType)
	{
		update();
	}

	void update()
	{
		if (customUpgradeType)
		{
			lineModel = customUpgradeType->get_line_model_for_display();
			borderLineModel = customUpgradeType->get_outer_line_model();
			unlockedCount = customUpgradeType->get_unlocked_count();
			unlockLimit = customUpgradeType->get_unlock_limit();
			canBeUnlocked = customUpgradeType->is_available_to_unlock();
		}
		unlocked &= unlockedCount < unlockLimit;
	}
};
REGISTER_FOR_FAST_CAST(CustomUpgradeInfo);
//

class HubScreens::PilgrimageInfo
: public Loader::IHubDraggableData
{
	FAST_CAST_DECLARE(PilgrimageInfo);
	FAST_CAST_BASE(Loader::IHubDraggableData);
	FAST_CAST_END();
public:
	bool unlocked = false;
	TeaForGodEmperor::Pilgrimage const* pilgrimage = nullptr;
	TeaForGodEmperor::LineModel const* borderLineModel = nullptr;
	String showText;

	PilgrimageInfo() {}
	PilgrimageInfo(Unlocks const* _unlocks, TeaForGodEmperor::Pilgrimage const * _pilgrimage, bool _unlocked)
	: unlocked(_unlocked)
	, pilgrimage(_pilgrimage)
	{
		if (pilgrimage)
		{
			borderLineModel = _unlocks->pilgrimageBorderLineModel.get();
		}
		update();
	}

	void update()
	{
		if (pilgrimage)
		{
			showText = pilgrimage->get_pilgrimage_name_pre_short_ls().get();
		}
	}
};
REGISTER_FOR_FAST_CAST(PilgrimageInfo);

//

int compare_pilgrimage(void const* _a, void const* _b)
{
	TeaForGodEmperor::Pilgrimage const* a = *(plain_cast<TeaForGodEmperor::Pilgrimage*>(_a));
	TeaForGodEmperor::Pilgrimage const* b = *(plain_cast<TeaForGodEmperor::Pilgrimage*>(_b));

	return String::compare_tchar_icase_sort(a->get_name().get_group().to_char(), b->get_name().get_group().to_char());
}

void sort_pilgrimages(REF_ Array<TeaForGodEmperor::Pilgrimage*>& _array)
{
	sort(_array, compare_pilgrimage);
}

bool add_exm(Unlocks* unlocks, HubWidgets::List* list, Name const& _exm, bool _unlocked = false, int _unlockedCount = 0)
{
	if (!_exm.is_valid())
	{
		return false;
	}

	auto* ei = new EXMInfo(unlocks, _exm, _unlocked);
	ei->unlockedCount = _unlockedCount;
	if (ei->exmType == nullptr)
	{
		delete ei;
		return false;
	}
	list->add(ei);
	return true;
}

bool add_custom_upgrade(Unlocks* unlocks, HubWidgets::List* list, TeaForGodEmperor::CustomUpgradeType const * _customUpgrade)
{
	if (!_customUpgrade)
	{
		return false;
	}

	auto* cui = new CustomUpgradeInfo(unlocks, _customUpgrade);
	if (cui->customUpgradeType == nullptr)
	{
		delete cui;
		return false;
	}
	list->add(cui);
	return true;
}

bool add_pilgrimage(Unlocks* unlocks, HubWidgets::List* list, TeaForGodEmperor::Pilgrimage * _p, bool _unlocked = false)
{
	bool add = false;
	if (_p)
	{
		if (_p->can_be_unlocked_for_experience())
		{
			add = true;
		}
		else if (_p->should_appear_as_unlocked_for_experience() && _unlocked)
		{
			add = true;
		}
	}
	if (add)
	{
		auto* pi = new PilgrimageInfo(unlocks, _p, _unlocked);
		list->add(pi);
		return true;
	}
	else
	{
		return false;
	}
}

void draw_element(HubWidget* widget, Framework::Display* _display, Range2 const& _at, HubDraggable const* _selectedElement, HubDraggable const* _element, bool _forceOriginalColours = false)
{
	Colour const ink = _display->get_current_ink();

	Vector2 bl = _at.bottom_left() + Vector2::half;
	Vector2 tr = _at.top_right() - Vector2::half;

	Vector2 c = TypeConversions::Normal::f_i_cells((bl + tr) * 0.5f).to_vector2() + Vector2::half;

	Vector2 size(tr.x - c.x, tr.y - c.y);
	size.x = min(size.x, size.y);
	size.y = size.x;

	size = TypeConversions::Normal::f_i_closest(size * 2.0f).to_vector2();

	struct Draw
	{
		static void line(Colour const& _c, Vector2 const& _a, Vector2 const& _b)
		{
			for (float ox = 0.0f; ox <= 1.0f; ++ox)
			{
				for (float oy = 0.0f; oy <= 1.0f; ++oy)
				{
					Vector2 o(ox, oy);
					::System::Video3DPrimitives::line_2d(_c, _a + o, _b + o);
				}
			}
		}
	};

	if (auto* ei = fast_cast<EXMInfo>(_element->data.get()))
	{
		bool isSelected = _selectedElement == _element;
		if (!_forceOriginalColours && isSelected)
		{
			Colour inside = HubColours::highlight();

			::System::Video3DPrimitives::fill_rect_2d(inside, _at, false);
		}

		if (auto* blm = ei->borderLineModel)
		{
			for_every(l, blm->get_lines())
			{
				Colour uc = !_forceOriginalColours && isSelected ? HubColours::selected_highlighted() : l->colour.get(ink);
				Draw::line(uc,
					TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
					TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
			}
			size *= 0.8f;
		}
		if (auto* lm = ei->lineModel)
		{
			for_every(l, lm->get_lines())
			{
				Colour uc = !_forceOriginalColours && isSelected ? HubColours::selected_highlighted() : l->colour.get(ink);
				Draw::line(uc,
					TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
					TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
			}
		}
	}
	if (auto* ei = fast_cast<CustomUpgradeInfo>(_element->data.get()))
	{
		bool isSelected = _selectedElement == _element;
		if (!_forceOriginalColours && isSelected)
		{
			Colour inside = HubColours::highlight();

			::System::Video3DPrimitives::fill_rect_2d(inside, _at, false);
		}

		if (auto* blm = ei->borderLineModel)
		{
			for_every(l, blm->get_lines())
			{
				Colour uc = !_forceOriginalColours && isSelected ? HubColours::selected_highlighted() : l->colour.get(ink);
				Draw::line(uc,
					TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
					TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
			}
			size *= 0.8f;
		}
		if (auto* lm = ei->lineModel)
		{
			for_every(l, lm->get_lines())
			{
				Colour uc = !_forceOriginalColours && isSelected ? HubColours::selected_highlighted() : l->colour.get(ink);
				Draw::line(uc,
					TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
					TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
			}
		}
	}
	if (auto* ei = fast_cast<PilgrimageInfo>(_element->data.get()))
	{
		bool isSelected = _selectedElement == _element;
		if (!_forceOriginalColours && isSelected)
		{
			Colour inside = HubColours::highlight();

			::System::Video3DPrimitives::fill_rect_2d(inside, _at, false);
		}

		if (auto* blm = ei->borderLineModel)
		{
			for_every(l, blm->get_lines())
			{
				Colour uc = !_forceOriginalColours && isSelected ? HubColours::selected_highlighted() : l->colour.get(ink);
				Draw::line(uc,
					TypeConversions::Normal::f_i_cells(c + size * l->a.to_vector2()).to_vector2() + Vector2::half,
					TypeConversions::Normal::f_i_cells(c + size * l->b.to_vector2()).to_vector2() + Vector2::half);
			}
			size *= 0.8f;
		}

		if (auto* font = _display->get_font())
		{
			Colour textColour = HubColours::text();

			float scale = 1.0f;
			if (auto* f = font->get_font())
			{
				Vector2 size = Vector2::zero;
				float tryScale = 1.0f;
				while (size.x < _at.x.length() * 0.8f &&
					   size.y < _at.y.length() * 0.7f)
				{
					scale = tryScale; // confirmed

					// try bigger one
					tryScale += 1.0f;
					size = f->calculate_text_size(TXT("AB"), Vector2::one * tryScale); // hardcoded to get the same scale
				}
			}
			font->draw_text_at(::System::Video3D::get(), ei->showText, textColour, _at.centre(), Vector2::one * scale, Vector2(0.5f, 0.5f));
		}
	}
}

//

Unlocks::ShowUnlocks Unlocks::lastUnlocksShown = Unlocks::SU_EXMs;
Optional<int> Unlocks::lastUnlocksEXMIdx;

REGISTER_FOR_FAST_CAST(Unlocks);

Unlocks::Unlocks(Hub* _hub, Name const& _id, Optional<Rotator3> const& _atDir, Vector2 const& _size, float _radius, Vector2 const& _ppa)
: HubScreen(_hub, _id, _size, _atDir.get(_hub->get_current_forward()), _radius, true, NP, _ppa)
{
	if (auto* lib = TeaForGodEmperor::Library::get_current_as< TeaForGodEmperor::Library>())
	{
		activeEXMBorderLineModel = lib->get_global_references().get<TeaForGodEmperor::LineModel>(NAME(grActiveEXMLineModel));
		passiveEXMBorderLineModel = lib->get_global_references().get<TeaForGodEmperor::LineModel>(NAME(grPassiveEXMLineModel));
		permanentEXMBorderLineModel = lib->get_global_references().get<TeaForGodEmperor::LineModel>(NAME(grPermanentEXMLineModel));
		pilgrimageBorderLineModel = lib->get_global_references().get<TeaForGodEmperor::LineModel>(NAME(grPilgrimageLineModel));
	}

	simpleRules = true;
	if (auto* gd = TeaForGodEmperor::GameDefinition::get_chosen())
	{
		if (gd->get_type() == TeaForGodEmperor::GameDefinition::Type::ComplexRules)
		{
			simpleRules = false;

			// doesn't make sense to check run in progress, you could end, unlock, continue
			// better allow to buy at any time
		}
	}

	unlockablePilgrimages = false;
	runInProgressDisallowsPilgrimageUnlocks = false;
	if (auto* gsd = TeaForGodEmperor::GameDefinition::get_chosen_sub(0))
	{
		if (gsd->get_difficulty().restartMode.is_set() &&
			gsd->get_difficulty().restartMode.get() == TeaForGodEmperor::GameSettings::RestartMode::UnlockedChapter)
		{
			unlockablePilgrimages = true;

			if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
			{
				if (gs->get_game_state_to_continue())
				{
					runInProgressDisallowsPilgrimageUnlocks = true;
				}
			}
		}
	}

	lastUnlocksEXMIdx.clear();

	show_unlocks(NP);
}

void Unlocks::update_unlockables()
{
	bool doingMissions = false;
	if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
	{
		if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
		{
			doingMissions = true;

			// we need mission state to get the available missions
			if (!TeaForGodEmperor::MissionState::get_current())
			{
				gs->create_new_mission_state();
			}
		}
	}
	unlockableEXMs.update(simpleRules, ! simpleRules || doingMissions);
	unlockableCustomUpgrades.update();

	if (auto* persistence = TeaForGodEmperor::Persistence::access_current_if_exists())
	{
		Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);

		xpToSpend = persistence->get_experience_to_spend();
		meritPointsToSpend = persistence->get_merit_points_to_spend();

		if (unlockablePilgrimages)
		{
			unlockedPilgrimages.clear();
			availablePilgrimagesToUnlock.clear();
			if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
			{
				if (auto* gs = ps->access_active_game_slot())
				{
					for_every(up, gs->unlockedPilgrimages)
					{
						if (auto* p = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>()->get_pilgrimages().find(*up))
						{
							unlockedPilgrimages.push_back(p);
						}						
					}
					gs->store_played_recently_pilgrimages_as_unlockables();
					for_every(up, gs->unlockablePlayedRecentlyPilgrimages)
					{
						if (auto* p = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>()->get_pilgrimages().find(*up))
						{
							if (!unlockedPilgrimages.does_contain(p))
							{
								availablePilgrimagesToUnlock.push_back(p);
							}
						}						
					}
					sort_pilgrimages(REF_ unlockedPilgrimages);
					sort_pilgrimages(REF_ availablePilgrimagesToUnlock);
				}
			}
		}
	}
}

void Unlocks::update_info()
{
	if (!hub)
	{
		return;
	}

	{
		// update unlocked count
		if (auto* draggable = selectedInfoElement.get())
		{
			if (auto* ei = fast_cast<EXMInfo>(draggable->data.get()))
			{
				ei->update();
			}
			else if (auto* pi = fast_cast<PilgrimageInfo>(draggable->data.get()))
			{
				pi->update();
			}
			else if (auto* cui = fast_cast<CustomUpgradeInfo>(draggable->data.get()))
			{
				cui->update();
			}
		}
	}

	EXMInfo* ei = nullptr;
	CustomUpgradeInfo* cui = nullptr;
	PilgrimageInfo* pi = nullptr;
	if (auto* draggable = selectedInfoElement.get())
	{
		ei = fast_cast<EXMInfo>(draggable->data.get());
		cui = fast_cast<CustomUpgradeInfo>(draggable->data.get());
		pi = fast_cast<PilgrimageInfo>(draggable->data.get());
	}

	if (auto* t = fast_cast<HubWidgets::Text>(get_widget(NAME(idSelectedEXMXPMP))))
	{
		TeaForGodEmperor::Reward toSpend;
		bool hasXPCost = calculate_unlock_xp_cost().is_positive();
		bool hasMPCost = calculate_unlock_merit_points_cost() > 0;
		if (hasXPCost)
		{
			toSpend.xp = xpToSpend;
		}
		if (hasMPCost)
		{
			if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
			{
				if (gs->get_game_slot_mode() == TeaForGodEmperor::GameSlotMode::Missions)
				{
					toSpend.meritPoints = meritPointsToSpend;
				}
			}
		}

		t->set(toSpend.as_string(hasXPCost, hasMPCost));
	}

	if (auto* b = fast_cast<HubWidgets::Button>(get_widget(NAME(idSelectedEXMUnlock))))
	{
		bool unlocked = (ei && ei->exmType && ei->unlocked) ||
						(cui && cui->customUpgradeType && cui->unlocked) ||
						(pi && pi->pilgrimage && pi->unlocked);
		bool canBeUnlocked = !unlocked;
		if (cui && cui->customUpgradeType)
		{
			canBeUnlocked = cui->canBeUnlocked;
		}
		if (runInProgressDisallowsPilgrimageUnlocks &&
			pi && pi->pilgrimage)
		{
			b->set(LOC_STR(NAME(lsRunInProgressEndRun)));
			b->enable();
		}
		else if ((ei && ei->exmType) ||
				 (cui && cui->customUpgradeType) ||
				 (pi && pi->pilgrimage))
		{
			if (unlocked)
			{
				b->set(LOC_STR(NAME(lsUnlocked)));
				b->disable();
			}
			else if (!canBeUnlocked)
			{
				b->set(String::empty()); // can't be unlocked, just disable
				b->disable();
			}
			else
			{
				TeaForGodEmperor::Energy costXP = calculate_unlock_xp_cost();
				int costMP = calculate_unlock_merit_points_cost();
				TeaForGodEmperor::Reward cost(costXP, costMP);
				Name unlockLS = NAME(lsUnlock);
				if (cui && cui->customUpgradeType && cui->customUpgradeType->is_for_buying())
				{
					unlockLS = NAME(lsUnlockBuy);
				}
				b->set(Framework::StringFormatter::format_loc_str(unlockLS, Framework::StringFormatterParams()
					.add(NAME(cost), cost.as_string())));
				if (xpToSpend >= costXP &&
					meritPointsToSpend >= costMP)
				{
					b->enable();
				}
				else
				{
					b->disable();
				}
			}
		}
		else
		{
			b->set(LOC_STR(NAME(lsSelectToUnlock)));
			b->disable();
		}
	}

	if (auto* t = fast_cast<HubWidgets::Text>(get_widget(NAME(idSelectedEXMDesc))))
	{
		if (runInProgressDisallowsPilgrimageUnlocks &&
			pi && pi->pilgrimage)
		{
			t->set(LOC_STR(NAME(lsRunInProgress)));
		}
		else if (ei && ei->exmType)
		{
			t->set(ei->exmType->get_ui_desc());
		}
		else if (cui && cui->customUpgradeType)
		{
			t->set(cui->customUpgradeType->get_ui_desc());
		}
		else if (pi && pi->pilgrimage)
		{
			t->set(pi->pilgrimage->get_pilgrimage_name_ls().get());
		}
		else
		{
			if (auto* l = fast_cast<HubWidgets::List>(get_widget(NAME(idAvailable))))
			{
				if (l->elements.is_empty())
				{
					t->set(LOC_STR(NAME(lsNoUnlocksLong)));
				}
				else
				{
					t->set(LOC_STR(NAME(lsSelectToUnlockLong)));
				}
			}
		}
	}

	if (cui && cui->customUpgradeType)
	{
		auto* t = fast_cast<HubWidgets::Text>(get_widget(NAME(idSelectedEXMUnlockedCount)));
		if (!t)
		{
			t = fast_cast<HubWidgets::Text>(get_widget(NAME(idSelectedEXMNote)));
		}

		if (t)
		{
			if (cui->unlockLimit > 0)
			{
				Name unlockLS = NAME(lsUnlockedCount);
				if (cui->customUpgradeType->is_for_buying())
				{
					unlockLS = NAME(lsUnlockedCountBuy);
				}
				t->set(Framework::StringFormatter::format_loc_str(unlockLS, Framework::StringFormatterParams()
					.add(NAME(unlocked), cui->unlockedCount)
					.add(NAME(limit), cui->unlockLimit)
				));
			}
			else
			{
				t->set(String::empty());
			}
		}
	}
	else
	{
		if (auto* t = fast_cast<HubWidgets::Text>(get_widget(NAME(idSelectedEXMUnlockedCount))))
		{
			if (runInProgressDisallowsPilgrimageUnlocks &&
				pi && pi->pilgrimage)
			{
				t->set(String::empty());
			}
			else if (ei && ei->exmType)
			{
				if (ei->exmType->is_permanent())
				{
					t->set(Framework::StringFormatter::format_loc_str(NAME(lsUnlockedCount), Framework::StringFormatterParams()
						.add(NAME(unlocked), ei->unlockedCount)
						.add(NAME(limit), ei->unlockLimit)
						));
				}
				else
				{
					t->set(String::empty());
				}
			}
			else
			{
				t->set(String::empty());
			}
		}

		if (auto* t = fast_cast<HubWidgets::Text>(get_widget(NAME(idSelectedEXMNote))))
		{
			if (runInProgressDisallowsPilgrimageUnlocks &&
				pi && pi->pilgrimage)
			{
				t->set(String::empty());
			}
			else if (ei && ei->exmType)
			{
				if (ei->exmType->is_permanent())
				{
					t->set(LOC_STR(NAME(lsEXMNotePermanent)));
				}
				else if (ei->exmType->is_active())
				{
					t->set(LOC_STR(NAME(lsEXMNoteActive)));
				}
				else
				{
					t->set(LOC_STR(NAME(lsEXMNotePassive)));
				}
			}
			else if (cui && cui->customUpgradeType)
			{
				t->set(String::empty());
			}
			else
			{
				t->set(String::empty());
			}
		}
	}
}

TeaForGodEmperor::Energy Unlocks::calculate_unlock_xp_cost()
{
	if (auto* draggable = selectedInfoElement.get())
	{
		if (auto* ei = fast_cast<EXMInfo>(draggable->data.get()))
		{
			if (!ei->unlocked && ei->exmType)
			{
				if (auto* persistence = TeaForGodEmperor::Persistence::access_current_if_exists())
				{
					Concurrency::ScopedSpinLock lock(persistence->access_lock(), true);
					if (ei->exmType->is_permanent())
					{
						return TeaForGodEmperor::GameplayBalance::persistent_permanent_exm_cost(persistence->get_permanent_exms().get_size());
					}
					else
					{
						return TeaForGodEmperor::GameplayBalance::persistent_normal_exm_cost();
					}
				}
			}
		}
		if (auto* pi = fast_cast<PilgrimageInfo>(draggable->data.get()))
		{
			if (!pi->unlocked && pi->pilgrimage)
			{
				int unlockedSoFar = 0;
				for_every_ptr(p, unlockedPilgrimages)
				{
					if (p->can_be_unlocked_for_experience())
					{
						++unlockedSoFar;
					}
				}
				return TeaForGodEmperor::GameplayBalance::pilgrimage_unlock_cost(unlockedSoFar);
			}
		}
	}
	if (auto* draggable = selectedInfoElement.get())
	{
		if (auto* cui = fast_cast<CustomUpgradeInfo>(draggable->data.get()))
		{
			if (auto* cu = cui->customUpgradeType)
			{
				return cu->calculate_unlock_xp_cost();
			}
		}
	}
	return TeaForGodEmperor::Energy::zero();
}

int Unlocks::calculate_unlock_merit_points_cost()
{
	if (auto* draggable = selectedInfoElement.get())
	{
		if (auto* cui = fast_cast<CustomUpgradeInfo>(draggable->data.get()))
		{
			if (auto* cu = cui->customUpgradeType)
			{
				return cu->calculate_unlock_merit_points_cost();
			}
		}
	}
	return 0;
}

void Unlocks::unlock_selected()
{
	if (runInProgressDisallowsPilgrimageUnlocks)
	{
		if (auto* draggable = selectedInfoElement.get())
		{
			if (auto* pi = fast_cast<PilgrimageInfo>(draggable->data.get()))
			{
				if (!pi->unlocked && pi->pilgrimage)
				{
					HubScreens::Question::ask(hub, NAME(lsRunInProgressEndRunQuestion),
						[this]()
						{
							clear();
							if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
							{
								if (auto* gs = TeaForGodEmperor::PlayerSetup::access_current().access_active_game_slot())
								{
									runInProgressDisallowsPilgrimageUnlocks = false;
									gs->startUsingGameState.clear();
									gs->lastMoment.clear();
									gs->checkpoint.clear();
									gs->atLeastHalfHealth.clear();
									gs->chapterStart.clear();
									if (auto* ms = TeaForGodEmperor::MissionState::get_current())
									{
										ms->mark_died();
									}
									gs->store_played_recently_pilgrimages_as_unlockables();
									gs->clear_played_recently_pilgrimages();
								}
								g->add_async_save_player_profile();
							}
							show_unlocks();
						},
						[]()
						{
						});
					return;
				}
			}
		}
	}
	if (auto* draggable = selectedInfoElement.get())
	{
		if (auto* persistence = TeaForGodEmperor::Persistence::access_current_if_exists())
		{
			Concurrency::ScopedSpinLock lock(persistence->access_lock());
			TeaForGodEmperor::Energy costXP = calculate_unlock_xp_cost();
			int costMP = calculate_unlock_merit_points_cost();
			if (persistence->get_experience_to_spend() >= costXP &&
				persistence->get_merit_points_to_spend() >= costMP)
			{
				bool unlocked = false;
				if (auto* ei = fast_cast<EXMInfo>(draggable->data.get()))
				{
					if (!ei->unlocked)
					{
						if (ei->exmType)
						{
							Name exm = ei->exmType->get_id();
							if (ei->exmType->is_permanent())
							{
								persistence->access_all_exms().push_back(exm);
								TeaForGodEmperor::PlayerSetup::access_current().stats__unlock_all_exms();
								persistence->cache_exms();
								unlocked = true;
							}
							else
							{
								if (!persistence->get_unlocked_exms().does_contain(exm))
								{
									persistence->access_all_exms().push_back(exm);
									TeaForGodEmperor::PlayerSetup::access_current().stats__unlock_all_exms();
									persistence->cache_exms();
									unlocked = true;
								}
							}
						}
					}
				}
				if (auto* cui = fast_cast<CustomUpgradeInfo>(draggable->data.get()))
				{
					if (!cui->unlocked)
					{
						if (auto* cu = cui->customUpgradeType)
						{
							unlocked = cu->process_unlock();
						}
					}
				}
				if (unlocked)
				{
					persistence->spend_experience(costXP); // we should still have it, we locked persistence
					persistence->spend_merit_points(costMP); // we should still have it, we locked persistence
					if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
					{
						// do not do it via store game state as that should be only called from within pilgrimage
#ifdef AN_ALLOW_EXTENSIVE_LOGS
						output(TXT("store player profile - persistent unlock"));
#endif
						g->add_async_save_player_profile();
					}
					show_unlocks();
					return;
				}
			}
		}
		if (auto* pi = fast_cast<PilgrimageInfo>(draggable->data.get()))
		{
			if (!pi->unlocked && pi->pilgrimage)
			{
				if (auto* persistence = TeaForGodEmperor::Persistence::access_current_if_exists())
				{
					Concurrency::ScopedSpinLock lock(persistence->access_lock());
					TeaForGodEmperor::Energy cost = calculate_unlock_xp_cost();
					if (persistence->get_experience_to_spend() >= cost)
					{
						bool unlocked = false;
						if (auto* ps = TeaForGodEmperor::PlayerSetup::access_current_if_exists())
						{
							if (auto* gs = ps->access_active_game_slot())
							{
								unlocked = gs->ignore_sync_unlock_pilgrimage(pi->pilgrimage->get_name());
							}
						}
						if (unlocked)
						{
							persistence->spend_experience(cost); // we should still have it, we locked persistence
							if (auto* g = TeaForGodEmperor::Game::get_as<TeaForGodEmperor::Game>())
							{
								// do not do it via store game state as that should be only called from within pilgrimage
#ifdef AN_ALLOW_EXTENSIVE_LOGS
								output(TXT("store player profile - persistent unlock"));
#endif
								g->add_async_save_player_profile();
							}
							show_unlocks();
							return;
						}
					}
				}
			}
		}
	}
}

void Unlocks::show_unlocks_help()
{
	if (auto* lib = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>())
	{
		TeaForGodEmperor::MessageSet* ms = nullptr;
		if (lastUnlocksShown == SU_EXMs)
		{
			ms = lib->get_global_references().get<TeaForGodEmperor::MessageSet>(simpleRules ? NAME(grHelpEXMsSimpleRules) : NAME(grHelpEXMsComplexRules));
		}
		if (ms)
		{
			float spacing = 3.0f;

			Vector2 ppa(20.0f, 20.0f);

			Vector2 hSize = get_size();
			hSize.x = hSize.y;
			hSize.y = round(hSize.y * 0.8f);
			Rotator3 hAt = at;
			hAt.yaw += get_size().x * 0.5f + hSize.x * 0.5f + spacing;
			HubScreens::MessageSetBrowser* msb = HubScreens::MessageSetBrowser::browse(ms, HubScreens::MessageSetBrowser::BrowseParams(), hub, NAME(idUnlocksHelp), true, hSize, hAt, radius, beVertical, verticalOffset, ppa, 1.0f, Vector2(0.0f, 1.0f));
			force_appear_foreground();
			msb->do_on_deactivate = [this]() { force_appear_foreground(false); };
			msb->dont_follow();
		}
	}
}

void Unlocks::show_unlocks(Optional<ShowUnlocks> _showUnlocks)
{
	_showUnlocks = _showUnlocks.get(lastUnlocksShown);
	lastUnlocksShown = _showUnlocks.get();

	clear();

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float spacing = max(fs.x, fs.y);

	Range2 availableSpace = mainResolutionInPixels;
	availableSpace.expand_by(Vector2::one * (-spacing));

	todo_note(TXT("add button to cycle through unlocks"));

	if (lastUnlocksShown == SU_EXMs)
	{
		Range2 helpButtonAt = Range2::empty;
		{
			auto* b = add_help_button();
			b->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
			{
				show_unlocks_help();
			};
			helpButtonAt = b->at;
		}

		update_unlockables();

		Vector2 paddingSize(0.0f, 0.0f);
		Vector2 elementSize(128.0f, 128.0f);
		Vector2 elementSizeSmall(128.0f, 128.0f);
		Vector2 elementSizeBig(256.0f, 256.0f);
		float elementSpacing = 8.0f;
		float scrollSize = 32.0f;

		{
			int availableToUnlock = 0;
			/*
			for_count(int, hIdx, Hand::MAX)
			{
				if (availableSetup.activeEXMs[hIdx].is_valid())
				{
					++availableToUnlock;
				}
				for_every(exm, availableSetup.passiveEXMs[hIdx])
				{
					if (exm->is_valid())
					{
						++availableToUnlock;
					}
				}
			}

			for_every(exm, availableSetup.permanentEXMs)
			{
				if (exm->is_valid())
				{
					++availableToUnlock;
				}
			}
			*/
			availableToUnlock += unlockableEXMs.availableEXMs.get_size();
			availableToUnlock += unlockableCustomUpgrades.availableCustomUpgrades.get_size();

			if (unlockablePilgrimages)
			{
				for_every_ptr(p, availablePilgrimagesToUnlock)
				{
					if (!unlockedPilgrimages.does_contain(p))
					{
						++availableToUnlock;
					}
				}
			}

			if (availableToUnlock <= 6)
			{
				elementSize = elementSizeBig;
			}
			else if (availableToUnlock <= 12)
			{
				elementSize = elementSize * 0.5f + 0.5f * elementSizeBig;
			}
		}

		elementSize += paddingSize;

		{
			Range2 at = availableSpace;
			at.y.max = at.y.min + elementSizeSmall.y + elementSpacing + scrollSize;

			if (!helpButtonAt.is_empty())
			{
				at.x.max = helpButtonAt.x.min - spacing;
			}

			auto* list = new HubWidgets::List(NAME(idUnlockHistory), at);
			list->be_grid();
			list->allowScrollWithStick = true;
			list->scroll.scrollType = Loader::HubWidgets::InnerScroll::HorizontalScroll;
			list->elementSize = elementSizeSmall;
			list->elementSpacing = Vector2(elementSpacing, elementSpacing);
			list->drawElementBorder = false;
			list->on_select = [this](HubDraggable const* _element, bool _clicked) { selectedInfoElement = _element;  update_info(); };
			list->draw_element = [this, list](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
			{
				draw_element(list, _display, _at, selectedInfoElement.get(), _element);
			};
			add_widget(list);

			for_every(exm, unlockableEXMs.unlockedEXMs)
			{
				// for simple rules only show permanent
				if (simpleRules)
				{
					if (auto* exmT = TeaForGodEmperor::EXMType::find(*exm))
					{
						if (!exmT->is_permanent())
						{
							continue;
						}
					}
				}
				add_exm(this, list, *exm, true, unlockableEXMs.unlockedEXMs.count(*exm));
			}

			if (unlockablePilgrimages)
			{
				for_every_ptr(p, unlockedPilgrimages)
				{
					add_pilgrimage(this, list, p, true);
				}
			}

			availableSpace.y.min = at.y.max + spacing;
		}

		// left - available to unlock
		{
			Range2 at = availableSpace;
			at.x.max = at.x.min + elementSpacing + scrollSize + elementSize.x;
			{
				float c = (float)TypeConversions::Normal::f_i_cells(availableSpace.x.centre() - spacing * 0.5f);
				float stride = elementSize.x + elementSpacing;
				while (at.x.max + stride <= c)
				{
					at.x.max += stride;
				}
			}

			auto* list = new HubWidgets::List(NAME(idAvailable), at);
			list->be_grid();
			list->allowScrollWithStick = true;
			list->scroll.scrollType = Loader::HubWidgets::InnerScroll::VerticalScroll;
			list->scroll.autoHideScroll = true;
			list->elementSize = elementSize;
			list->elementSpacing = Vector2(elementSpacing, elementSpacing);
			list->drawElementBorder = false;
			list->on_select = [list, this](HubDraggable const* _element, bool _clicked)
			{
				selectedInfoElement = _element;
				for_every_ref(e, list->elements)
				{
					if (e == _element)
					{
						lastUnlocksEXMIdx = for_everys_index(e);
					}
				}
				update_info();
			};
			list->draw_element = [this, list](Framework::Display* _display, Range2 const& _at, HubDraggable const* _element)
			{
				draw_element(list, _display, _at, selectedInfoElement.get(), _element);
			};
			add_widget(list);

			bool anythingAvailable = false;
			/*
			anythingAvailable |= add_exm(this, list, availableSetup.activeEXMs[Hand::Left], false, unlockedEXMs.count(availableSetup.activeEXMs[Hand::Left]));
			anythingAvailable |= add_exm(this, list, availableSetup.activeEXMs[Hand::Right], false, unlockedEXMs.count(availableSetup.activeEXMs[Hand::Right]));

			for_count(int, hIdx, Hand::MAX)
			{
				for_every(exm, availableSetup.passiveEXMs[hIdx])
				{
					anythingAvailable |= add_exm(this, list, *exm, false, unlockedEXMs.count(*exm));
				}
			}

			for_every(exm, availableSetup.permanentEXMs)
			{
				anythingAvailable |= add_exm(this, list, *exm, false, unlockedEXMs.count(*exm));
			}
			*/
			for_every(acu, unlockableCustomUpgrades.availableCustomUpgrades)
			{
				add_custom_upgrade(this, list, acu->upgrade);
			}

			for_every(exm, unlockableEXMs.availableEXMs)
			{
				anythingAvailable |= add_exm(this, list, *exm, false, unlockableEXMs.unlockedEXMs.count(*exm));
			}

			if (unlockablePilgrimages)
			{
				for_every_ptr(p, availablePilgrimagesToUnlock)
				{
					if (!unlockedPilgrimages.does_contain(p))
					{
						anythingAvailable |= add_pilgrimage(this, list, p, false);
					}
				}
			}

			availableSpace.x.min = at.x.max + spacing;

			if (anythingAvailable)
			{
				hub->select(list, list->elements[clamp(lastUnlocksEXMIdx.get(0), 0, list->elements.get_size() - 1)].get());
			}
			else
			{
				hub->deselect();
			}
		}

		// right - info
		{
			Range2 at = availableSpace;

			Range2 descWhole;
			Range2 unlockWhole;

			{
				Range2 eat = at;
				eat.y.min = eat.y.max - (elementSizeBig.y - 1.0f);
				eat.x.min = (float)TypeConversions::Normal::f_i_cells(at.x.centre() - elementSizeBig.x * 0.5f);
				eat.x.max = eat.x.min + (elementSizeBig.x - 1.0f);

				auto* cd = new HubWidgets::CustomDraw(NAME(idSelectedEXMView), eat, nullptr);
				cd->custom_draw = [this, cd](Framework::Display* _display, Range2 const& _at)
				{
					if (cd->hub)
					{
						if (auto* draggable = selectedInfoElement.get())
						{
							bool drawNow = true;
							if (!runInProgressDisallowsPilgrimageUnlocks)
							{
								if (auto* pi = fast_cast<PilgrimageInfo>(draggable->data.get()))
								{
									if (!pi->unlocked && pi->pilgrimage)
									{
										drawNow = false;
									}
								}
							}
							if (drawNow)
							{
								draw_element(cd, _display, _at, draggable, draggable, true);
							}
						}
					}
				};
				add_widget(cd);

				at.y.max = eat.y.min - spacing;

				descWhole = eat;
			}

			{
				Range2 eat = at;
				eat.y.max = eat.y.min + fs.y;

				String xpToSpendText;
				xpToSpendText = TXT("--");

				auto* t = new HubWidgets::Text(NAME(idSelectedEXMXPMP), eat, Framework::StringFormatter::format_loc_str(NAME(lsCurrentXP), Framework::StringFormatterParams()
					.add(NAME(xpToSpend), xpToSpendText)));
				add_widget(t);

				at.y.min = eat.y.max + spacing;
				unlockWhole = eat;
			}

			{
				Range2 eat = at;
				eat.y.max = eat.y.min + spacing * 3.0f;

				auto* b = new HubWidgets::Button(NAME(idSelectedEXMUnlock), eat, NAME(lsSelectToUnlock));
				b->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { unlock_selected(); };
				b->disable();
				//b->activateOnHold = true;
				add_widget(b);

				at.y.min = eat.y.max + spacing;
				unlockWhole.include(eat);
			}

			{
				Range2 eat = at;

				auto* t = new HubWidgets::Text(NAME(idSelectedEXMDesc), eat, String::empty());
				t->alignPt = Vector2(0.5f, 1.0f);
				add_widget(t);

				descWhole.include(eat);
			}

			if (simpleRules)
			{
				Range2 eat = at;

				auto* t = new HubWidgets::Text(NAME(idSelectedEXMUnlockedCount), eat, String::empty());
				t->alignPt = Vector2(0.5f, 0.0f);
				add_widget(t);

				descWhole.include(eat);				
			}
			else
			{
				Range2 eat = at;

				auto* t = new HubWidgets::Text(NAME(idSelectedEXMNote), eat, String::empty());
				t->alignPt = Vector2(0.5f, 0.0f);
				add_widget(t);

				descWhole.include(eat);
			}

			// wholes
			{
				Range2 eat = descWhole;

				auto* t = new HubWidgets::Text(NAME(idSelectedEXMDescWhole), eat, String::empty());
				t->ignoreInput = true;
				add_widget(t);				
			}

			{
				Range2 eat = unlockWhole;

				auto* t = new HubWidgets::Text(NAME(idSelectedEXMUnlockWhole), eat, String::empty());
				t->ignoreInput = true;
				add_widget(t);				
			}
		}

		update_info();
	}
}