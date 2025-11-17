#include "lhc_gameModifiers.h"

#include "..\lhc_messageSetBrowser.h"

#include "..\..\loaderHub.h"
#include "..\..\widgets\lhw_button.h"

#include "..\..\..\..\game\playerSetup.h"
#include "..\..\..\..\library\library.h"

#include "..\..\..\..\..\framework\video\font.h"
#include "..\..\..\..\..\framework\video\texturePart.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsOk, TXT("hub; common; ok"));
DEFINE_STATIC_NAME_STR(lsContinue, TXT("hub; common; continue"));
DEFINE_STATIC_NAME_STR(lsApply, TXT("hub; common; apply"));
DEFINE_STATIC_NAME_STR(lsCancel, TXT("hub; common; cancel"));

DEFINE_STATIC_NAME_STR(lsTitle, TXT("hub; game modifiers"));

// global references
DEFINE_STATIC_NAME_STR(grHelp, TXT("hub; help; game modifiers"));

// ids
DEFINE_STATIC_NAME(idHelp);
DEFINE_STATIC_NAME(idExperienceModifier);

//

using namespace Loader;
using namespace HubScreens;

//

REGISTER_FOR_FAST_CAST(GameModifiers);

HubScreen* GameModifiers::show(Hub* _hub, bool _beStandAlone, Name const& _id, TeaForGodEmperor::RunSetup& _runSetup, std::function<void()> _initialSetup, std::function<void()> _onApply)
{
	float ppa1 = 30.0f;

	{
		VectorInt2 size = VectorInt2::one;
		auto allgms = TeaForGodEmperor::GameModifier::get_all_game_modifiers();
		for_every(gmId, allgms)
		{
			if (auto* gm = TeaForGodEmperor::GameModifier::get(*gmId))
			{
				size.x = max(size.x, gm->get_at().x + 1);
				size.y = max(size.y, gm->get_at().y + 1);
			}
		}

		ppa1 *= (float)size.y / 5.0f;
	}

	Vector2 ppa = Vector2(ppa1, ppa1);
	Vector2 size(50.0f, 50.0f);

	GameModifiers* s = new GameModifiers(_hub, _beStandAlone, _id, _runSetup, _initialSetup, NP, _onApply, NP, size, _hub->get_radius() * 0.6f, ppa);

	s->activate();
	_hub->add_screen(s);

	return s;
}

GameModifiers::GameModifiers(Hub* _hub, bool _beStandAlone, Name const& _id, TeaForGodEmperor::RunSetup& _runSetup, std::function<void()> _initialSetup, Optional<Name> const& _onApplyButton, std::function<void()> _onApply, Optional<Rotator3> const& _atDir, Vector2 const& _size, float _radius, Vector2 const& _ppa, bool _forHeightCalculation)
: HubScreen(_hub, _id, _size, _atDir.get(_hub->get_current_forward()), _radius, true, NP, _ppa)
, runSetupSource(_runSetup)
, runSetup(_runSetup)
{
	if (_beStandAlone)
	{
		be_modal();
	}
	
	colourTitleOn.parse_from_string(String(TXT("gameModifiers_title_on")));
	colourTitleOff.parse_from_string(String(TXT("gameModifiers_title_off")));
	colourDescOn.parse_from_string(String(TXT("gameModifiers_desc_on")));
	colourDescOff.parse_from_string(String(TXT("gameModifiers_desc_off")));

	Vector2 fs = HubScreen::s_fontSizeInPixels;

	float spacing = max(fs.x, fs.y);

	VectorInt2 size = VectorInt2::one;
	float verticalSize = 0.0f;

	if (_forHeightCalculation)
	{
		VectorInt2 maxSize = TeaForGodEmperor::GameModifier::get_game_modifiers_grid_size();
		auto allgms = TeaForGodEmperor::GameModifier::get_all_game_modifiers();
		for_every(gmId, allgms)
		{
			if (auto* gm = TeaForGodEmperor::GameModifier::get(*gmId))
			{
				if (widgets.is_empty())
				{
					for_count(int, y, maxSize.y)
					{
						for_count(int, x, maxSize.x)
						{
							VectorInt2 at(x, y);
							GameModifierWidget w;
							w.gameModifier = gm;
							w.at = at;
							widgets.push_back(w);
							size.x = max(size.x, at.x + 1);
							size.y = max(size.y, at.y + 1);

						}
					}
				}
				verticalSize = max(verticalSize, gm->get_texture_part_on()->get_size().y);
			}
		}
	}
	else
	{
		auto allgms = TeaForGodEmperor::GameModifier::get_all_game_modifiers();
		for_every(gmId, allgms)
		{
			if (auto* gm = TeaForGodEmperor::GameModifier::get(*gmId))
			{
				GameModifierWidget w;
				w.gameModifier = gm;
				widgets.push_back(w);
				size.x = max(size.x, gm->get_at().x + 1);
				size.y = max(size.y, gm->get_at().y + 1);

				verticalSize = max(verticalSize, gm->get_texture_part_on()->get_size().y);
			}
		}
	}

	verticalSize *= 2.0f;

	verticalSize += 10.0f;

	//

	Range2 screen = mainResolutionInPixels;
	screen.expand_by(Vector2::one * (-spacing));

	{
		auto* t = new HubWidgets::Text(NAME(idExperienceModifier), screen, String::empty());
		t->scale = Vector2::one * 2.0f;
		t->at.x.min = t->at.x.max - fs.x * t->scale.x * 5.0f;
		t->at.y.min = t->at.y.max - fs.y * t->scale.y;
		t->with_align_pt(Vector2(1.0f, 1.0f));
		add_widget(t);
		experienceModifierWidget = t;
	}

	{
		auto* t = new HubWidgets::Text();
		t->at = screen;
		t->scale = Vector2::one * 2.0f;
		t->at.y.min = t->at.y.max - fs.y * t->scale.y;
		t->with_align_pt(Vector2(0.5f, 1.0f));
		t->set(LOC_STR(NAME(lsTitle)));
		add_widget(t);
		screen.y.max -= fs.y * t->scale.y + spacing;
	}

	float gmSize = (screen.x.length() + 1.0f - spacing * (float)size.x) / (float)size.x;

	for_every(w, widgets)
	{
		VectorInt2 atCoord = w->at.get(w->gameModifier->get_at());

		Range2 at = screen;

		at.x.min = screen.x.min + (gmSize + spacing) * (float)atCoord.x;
		at.x.max = atCoord.x == size.x - 1? screen.x.max : at.x.min + gmSize - 1.0f;
		at.y.max = screen.y.max - (verticalSize + spacing) * (float)atCoord.y;
		at.y.min = at.y.max - verticalSize + 1.0f;

		Range2 buttonAt = at;
		buttonAt.x.max = buttonAt.x.min + (buttonAt.y.max - buttonAt.y.min); // force square
		{
			auto* b = new HubWidgets::Button();
			b->at = buttonAt;
			b->scale = Vector2::one * 2.0f;
			Name id = w->gameModifier->get_id();
			b->on_click = [this, id](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags) { change_modifier(id); };
			w->buttonWidget = b;
			add_widget(b);
		}
		Range2 titleAt = at;
		titleAt.x.min = buttonAt.x.max + spacing;
		titleAt.y.min = titleAt.y.max - _hub->get_font()->get_height() * 1.2f;
		{
			auto* t = new HubWidgets::Text();
			t->at = titleAt;
			t->with_align_pt(Vector2(0.0f, 1.0f));
			t->set(w->gameModifier ? w->gameModifier->get_title().get() : String::something());
			w->titleWidget = t;
			add_widget(t);
		}
		Range2 descAt = titleAt;
		descAt.x.min = buttonAt.x.max + spacing;
		descAt.y.max = titleAt.y.min;
		descAt.y.min = at.y.min;
		{
			auto* d = new HubWidgets::Text();
			d->at = descAt;
			d->with_align_pt(Vector2(0.0f, 1.0f));
			d->set(w->gameModifier ? w->gameModifier->get_desc().get() : String::something());
			w->descriptionWidget = d;
			add_widget(d);
		}
	}

	if (_beStandAlone || _onApply)
	{
		Array<HubScreenButtonInfo> buttons;
		Range2 at = mainResolutionInPixels;
		at.y.min = mainResolutionInPixels.y.min + fs.y * 1.0f;
		at.y.max = mainResolutionInPixels.y.min + fs.y * 2.5f * (_beStandAlone? 1.0f : 1.5f);
		if (! _beStandAlone)
		{
			at.x.min = mainResolutionInPixels.x.get_at(0.35f);
			at.x.max = mainResolutionInPixels.x.get_at(0.65f);
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), _onApplyButton.get(NAME(lsContinue)), [this, _initialSetup, _onApply]() { runSetupSource = runSetup; if (_onApply) _onApply(); }).activate_on_trigger_hold().with_scale(2.0f));
			add_button_widgets(buttons, at, spacing);
		}
		else if (_initialSetup)
		{
			at.x.min = mainResolutionInPixels.x.get_at(0.35f);
			at.x.max = mainResolutionInPixels.x.get_at(0.65f);
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsContinue), [this, _initialSetup]() { runSetupSource = runSetup; deactivate(); _initialSetup(); }).activate_on_trigger_hold());
			add_button_widgets(buttons, at, spacing);
		}
		else
		{
			at.x = mainResolutionInPixels.x.expanded_by(-fs.x * 6.0f);
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), _onApplyButton.get(NAME(lsApply)), [this, _onApply]() { runSetupSource = runSetup; deactivate(); if (_onApply) _onApply(); }).activate_on_trigger_hold());
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsCancel), [this]() { deactivate(); }));
			add_button_widgets(buttons, at, spacing);
		}
	}

	compress_vertically();

	{
		auto* b = add_help_button();
		b->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
		{
			show_help();
		};
	}

	change_modifier(Name::invalid());
	// update_widgets is called from within
}

void GameModifiers::show_help()
{
	if (auto* lib = TeaForGodEmperor::Library::get_current_as<TeaForGodEmperor::Library>())
	{
		TeaForGodEmperor::MessageSet* ms = lib->get_global_references().get<TeaForGodEmperor::MessageSet>(NAME(grHelp));
		if (ms)
		{
			float spacing = 3.0f;

			Vector2 ppa(20.0f, 20.0f);

			Vector2 hSize = get_size();
			hSize.x = hSize.y;
			hSize.y = round(hSize.y * 0.8f);
			Rotator3 hAt = at;
			hAt.yaw += get_size().x * 0.5f + hSize.x * 0.5f + spacing;
			HubScreens::MessageSetBrowser* msb = HubScreens::MessageSetBrowser::browse(ms, HubScreens::MessageSetBrowser::BrowseParams(), hub, NAME(idHelp), true, hSize, hAt, radius, beVertical, verticalOffset, ppa, 1.0f, Vector2(0.0f, 1.0f));
			force_appear_foreground();
			msb->do_on_deactivate = [this]() { force_appear_foreground(false); };
			msb->dont_follow();
		}
	}
}


void GameModifiers::change_modifier(Name const& _modifier, Optional<bool> const& _set)
{
	if (_modifier.is_valid())
	{
		bool on = _set.get(!runSetup.activeGameModifiers.get_tag_as_int(_modifier));
		if (on)
		{
			runSetup.activeGameModifiers.set_tag(_modifier);
		}
		else
		{
			runSetup.activeGameModifiers.remove_tag(_modifier);
		}

		// disallow others
		for_every(w, widgets)
		{
			if (w->gameModifier->get_id() != _modifier)
			{
				if (w->gameModifier->get_disabled_when().does_contain(_modifier))
				{
					runSetup.activeGameModifiers.remove_tag(w->gameModifier->get_id());
				}
			}
		}
	}

	update_widgets();

	if (!standAlone)
	{
		// copy wherever we can so we're not lost
		runSetupSource = runSetup;
		runSetupSource.update_using_this();

		auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
		if (auto* gs = ps.access_active_game_slot())
		{
			gs->runSetup = runSetup;
		}

		if (auto* t = fast_cast<HubWidgets::Text>(experienceModifierWidget.get()))
		{
			t->set(String::printf(TXT("x%S"), (TeaForGodEmperor::GameSettings::get().experienceModifier + TeaForGodEmperor::EnergyCoef::one()).as_string_percentage_auto_decimals().to_char()));
		}
	}
}

void GameModifiers::update_widgets()
{
	for_every(w, widgets)
	{
		Name id = w->gameModifier->get_id();
		bool isOn = runSetup.activeGameModifiers.get_tag_as_int(id);
		// disallow if other active
		{
			bool shouldBeInactive = false;
			for_every(dw, w->gameModifier->get_disabled_when())
			{
				if (runSetup.activeGameModifiers.get_tag_as_int(*dw))
				{
					shouldBeInactive = true;
					break;
				}
			}
			if (shouldBeInactive)
			{
				runSetup.activeGameModifiers.remove_tag(id);
			}
			w->blocked = shouldBeInactive;
		}

		if (auto* b = fast_cast<HubWidgets::Button>(w->buttonWidget.get()))
		{
			b->enable(!w->blocked);
			b->texturePart = isOn ? w->gameModifier->get_texture_part_on() : w->gameModifier->get_texture_part_off();
			b->noBorderWhenDisabled = true;
			b->alignPt = Vector2::one * 0.48f;
			if (auto* tp = b->texturePart)
			{
				// it just looks good, maybe there are some other reasons it has to be aligned this way, but it works
				float px = (1.0f / (float)tp->get_size().x);
				float bs = b->scale.x + 1.0f;
				float pxbs = px / bs;
				float cpt = (tp->get_uv_anchor().x - tp->get_uv_0().x) / (tp->get_uv_1().x - tp->get_uv_0().x);
				float aptx = cpt + pxbs;
				float apty = cpt + (px-pxbs);
				b->alignPt = Vector2(aptx, apty);
			}
			b->set_highlighted(isOn);
		}
		if (auto* t = fast_cast<HubWidgets::Text>(w->titleWidget.get()))
		{
			t->set_colour(isOn ? colourTitleOn : colourTitleOff);
		}
		if (auto* d = fast_cast<HubWidgets::Text>(w->descriptionWidget.get()))
		{
			d->set_colour(isOn ? colourDescOn : colourDescOff);
		}
	}

}
