#include "lhs_vrModeSelection.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_text.h"
#include "..\screens\lhc_message.h"

#include "..\..\..\game\game.h"
#include "..\..\..\game\playerSetup.h"

#include "..\..\..\..\framework\library\library.h"
#include "..\..\..\..\framework\text\localisedString.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// screen id
DEFINE_STATIC_NAME(screen_vrModeSelection);
DEFINE_STATIC_NAME(screen_vrModeSelectionOpenLanguageSelection);

// widget ids
DEFINE_STATIC_NAME(button_roomScale);
DEFINE_STATIC_NAME(button_standing);

// localised strings
DEFINE_STATIC_NAME_STR(lsGeneralLanguageAuto, TXT("hub; opt; general; language; auto")); 
DEFINE_STATIC_NAME_STR(lsPleaseWait, TXT("hub; common; please wait"));
DEFINE_STATIC_NAME_STR(lsVRModeSelectionTop, TXT("hub; vr mode sel; top"));
DEFINE_STATIC_NAME_STR(lsVRModeSelectionBottom, TXT("hub; vr mode sel; bottom"));
DEFINE_STATIC_NAME_STR(lsVRModeRoomScale, TXT("hub; vr mode sel; room-scale"));
DEFINE_STATIC_NAME_STR(lsVRModeRoomScaleRecommended, TXT("hub; vr mode sel; room-scale; recommended"));
DEFINE_STATIC_NAME_STR(lsVRModeRoomScaleNotEnoughSpace, TXT("hub; vr mode sel; room-scale; not enough space"));
DEFINE_STATIC_NAME_STR(lsVRModeStanding, TXT("hub; vr mode sel; standing"));

// global references
DEFINE_STATIC_NAME_STR(grLanguageSelection, TXT("language selection"));
//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(VRModeSelection);

VRModeSelection::VRModeSelection(float _delay)
: delay(_delay)
{
}

void VRModeSelection::on_activate(HubScene* _prev)
{
	base::on_activate(_prev);

	okToEnd = false;
	timeToShow = delay;

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	if (timeToShow <= 0.0f)
	{
		show();
	}
}

void VRModeSelection::show()
{
	get_hub()->deactivate_all_screens();

	get_hub()->force_reset_and_update_forward();

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

	HubScreen* vrModeSelectionScreen = nullptr;
	if (!atDir.is_set())
	{
		atDir = get_hub()->get_current_forward();
	}
	float screenSpacing = 1.0f;
	{
		Vector2 size(40.0f, 40.0f);
		Vector2 ppa(24.0f, 24.0f);
		Rotator3 verticalOffset(0.0f, 0.0f, 0.0f);

		auto* screen = new HubScreen(get_hub(), NAME(screen_vrModeSelection), size, atDir.get(), get_hub()->get_radius() * 0.5f, true, NP, ppa);
		screen->activate();
		get_hub()->add_screen(screen);

		Vector2 spacing = Vector2::one * fsxy;
		Range2 at = screen->mainResolutionInPixels.expanded_by(-spacing);

		// show screen with a message
		{
			Range2 eat = at;
			auto* e = new HubWidgets::Text(Name::invalid(), eat, LOC_STR(NAME(lsVRModeSelectionTop)));
			e->alignPt.x = 0.0f;
			e->alignPt.y = 1.0f;
			screen->add_widget(e);
			e->at.y.min = e->at.y.max - e->calculate_vertical_size();

			at.y.max = e->at.y.min - spacing.y;
		}

		// buttons to select room-scale
		{
			Array<HubScreenButtonInfo> buttons;
			{
				bool roomScaleRecommended = false;
				bool roomScaleNotEnoughSpace = false;
				if (VR::IVR::can_be_used())
				{
					if (auto* vr = VR::IVR::get())
					{
						// reload just in any case
						vr->load_play_area_rect(true);

						if (vr->has_loaded_play_area_rect())
						{
							// double as we're reading halves
							float x = vr->get_raw_whole_play_area_rect_half_right().length() * 2.0f;
							float y = vr->get_raw_whole_play_area_rect_half_forward().length() * 2.0f;

							if (x >= 1.85f && y >= 1.85f)
							{
								roomScaleRecommended = true;
							}
							if (x <= 1.5f || y <= 1.0f)
							{
								roomScaleNotEnoughSpace = true;
							}
						}
					}
				}

				if (roomScaleRecommended)
				{

					buttons.push_back(HubScreenButtonInfo(NAME(button_roomScale), NAME(lsVRModeRoomScaleRecommended), [this]()
						{
							auto& mc = MainConfig::access_global();
							mc.set_immobile_vr(Option::False);
							okToEnd = true;
							get_hub()->deactivate_all_screens();
						}));
				}
				else if (roomScaleNotEnoughSpace)
				{
					buttons.push_back(HubScreenButtonInfo(NAME(button_roomScale), NAME(lsVRModeRoomScaleNotEnoughSpace), [this]()
						{
							auto& mc = MainConfig::access_global();
							mc.set_immobile_vr(Option::False);
							okToEnd = true;
							get_hub()->deactivate_all_screens();
						}));
				}
				else
				{
					buttons.push_back(HubScreenButtonInfo(NAME(button_roomScale), NAME(lsVRModeRoomScale), [this]()
						{
							auto& mc = MainConfig::access_global();
							mc.set_immobile_vr(Option::False);
							okToEnd = true;
							get_hub()->deactivate_all_screens();
						}));
				}

				buttons.push_back(HubScreenButtonInfo(NAME(button_roomScale), NAME(lsVRModeStanding), [this]()
					{
						auto& mc = MainConfig::access_global();
						mc.set_immobile_vr(Option::True);
						okToEnd = true;
						get_hub()->deactivate_all_screens();
					}));

				float linesRequired = 3.0f;
				{
					int maxWidthAvailable = TypeConversions::Normal::f_i_closest((at.x.length() - spacing.x) / 2.0f);
					for_every(but, buttons)
					{
						int lc;
						int width;
						HubWidgets::Text::measure(get_hub()->get_font(), NP, NP, LOC_STR(but->locStrId), OUT_ lc, OUT_ width, maxWidthAvailable);

						linesRequired = max(linesRequired, (float)lc + 1.0f);
					}
				}

				Range2 bat = at;
				bat.y.min = bat.y.max - (fs.y * linesRequired);
				screen->add_button_widgets_grid(buttons, VectorInt2(buttons.get_size(), 1), bat, spacing);

				at.y.max = bat.y.min - spacing.y;
			}
		}

		{
			Range2 eat = at;
			auto* e = new HubWidgets::Text(Name::invalid(), eat, LOC_STR(NAME(lsVRModeSelectionBottom)));
			e->alignPt.x = 1.0f;
			e->alignPt.y = 1.0f;
			screen->add_widget(e);
			e->at.y.min = e->at.y.max - e->calculate_vertical_size();

			at.y.max = e->at.y.min - spacing.y;
		}

		screen->compress_vertically();

		vrModeSelectionScreen = screen;
	}

	if (auto* tp = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grLanguageSelection)))
	{
		Vector2 size(8.0f, 5.0f);
		Vector2 ppa(24.0f, 24.0f);

		auto* screen = new HubScreen(get_hub(), NAME(screen_vrModeSelectionOpenLanguageSelection), size, atDir.get(), get_hub()->get_radius() * 0.5f, true, Rotator3(vrModeSelectionScreen->size.y * 0.5f + size.y * 0.5f + screenSpacing, 0.0f, 0.0f), ppa);

		Vector2 spacing = Vector2::one * 6.0f;
		Range2 at = screen->mainResolutionInPixels.expanded_by(-spacing);

		{
			auto* b = new HubWidgets::Button(Name::invalid(), at, tp, Colour::white.mul_rgb(0.9f));
			b->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
			{
				get_hub()->deactivate_all_screens();
				open_language_selection();
			};
			
			screen->add_widget(b);
		}


		screen->activate();
		get_hub()->add_screen(screen);
	}
}

void VRModeSelection::open_language_selection()
{
	allowToShow = false;
	struct Option
	{
		Name id;
		String showAs;
		bool selected = false;

		Option() {}
		Option(Name const & _id, String const& _showAs, bool _selected = false): id(_id), showAs(_showAs), selected(_selected) {}
	};
	Array<Option> options;

	options.make_space_for(Framework::LocalisedStrings::get_languages().get_size() + 1);
	{
		Name const& systemLanguage = Framework::LocalisedStrings::get_system_language();
		if (systemLanguage.is_valid())
		{
			options.push_back(Option(systemLanguage, LOC_STR(NAME(lsGeneralLanguageAuto))));
		}
	}
	auto sortedLanguages = Framework::LocalisedStrings::get_sorted_languages();
	for_every(sl, sortedLanguages)
	{
		options.push_back(Option(sl->id, sl->name, Framework::LocalisedStrings::get_language() == sl->id));
	}
	
	int maxWidth = 1;
	for_every(option, options)
	{
		int lc, w;
		HubWidgets::Text::measure(get_hub()->get_font(), NP, NP, option->showAs, OUT_ lc, OUT_ w);
		maxWidth = max(maxWidth, w);
	}

	auto fs = HubScreen::s_fontSizeInPixels;
	VectorInt2 grid(1, options.get_size());
	while (grid.y >= 10)
	{
		grid.x++;
		grid.y = 1;
		while (grid.x * grid.y < options.get_size())
		{
			grid.y++;
		}
	}
	Vector2 buttonSize = round(Vector2((float)maxWidth + fs.x * 3.0f, fs.y * 1.8f));
	Vector2 border = Vector2::one * 4.0f;
	Vector2 spacing = Vector2::one * 4.0f;
	Vector2 pixelsPerAngle(18.0f, 18.0f);
	Vector2 listInPixels = grid.to_vector2() * buttonSize + (grid - VectorInt2::one).to_vector2() * spacing + border * 2.0f;
	HubScreen* list = new HubScreen(get_hub(), Name::invalid(),
		listInPixels / pixelsPerAngle, atDir.get(), get_hub()->get_radius() * 0.5f, NP, NP, pixelsPerAngle);
	list->be_modal();
	Array<HubScreenButtonInfo> buttons;
	buttons.make_space_for(options.get_size());
	for_every(opt, options)
	{
		Name id = opt->id;
		String text = opt->showAs;
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), text,
			[this, id, text, list](HubInputFlags::Flags _flags)
			{
				HubScreens::Message::show(get_hub(), LOC_STR(NAME(lsPleaseWait)), nullptr);

				output(TXT("[VRModeSel] requested language change to \"%S\""), id.to_char());
				auto* g = Framework::Game::get_as<Framework::Game>();
				an_assert(g);
				// world jobs won't be working while loading a library, do it as a background job
				Framework::Game::async_background_job(g, [this, id]()
					{
						if (id.is_valid())
						{
							output(TXT("[VRModeSel] switching language \"%S\""), id.to_char());
							auto& ps = TeaForGodEmperor::PlayerSetup::access_current();
							auto& pp = ps.access_preferences();
							pp.language = id;
							Framework::LocalisedStrings::set_language(id);
							output(TXT("[VRModeSel] switched language \"%S\""), id.to_char());
						}
						get_hub()->deactivate_all_screens();
						allowToShow = true;
						timeToShow = 0.1f;
					});
				list->deactivate();
			},
			Framework::LocalisedStrings::get_language() == id).be_highlighted(opt->selected));
	}
	list->add_button_widgets_grid(buttons, grid, list->mainResolutionInPixels.expanded_by(-border), spacing);

	list->activate();
	get_hub()->add_screen(list);
}

void VRModeSelection::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);
	if (get_hub() && get_hub()->get_all_screens().is_empty() && allowToShow)
	{
		if (timeToShow > 0.0f && !okToEnd)
		{
			timeToShow -= _deltaTime;
			if (timeToShow <= 0.0f)
			{
				show();
			}
		}
	}
}

void VRModeSelection::on_deactivate(HubScene* _next)
{
	if (okToEnd)
	{
		base::on_deactivate(_next);
	}
}

void VRModeSelection::on_loader_deactivate()
{
	base::on_loader_deactivate();
}
