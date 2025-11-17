#include "lhs_handyMessages.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\screens\lhc_message.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_list.h"

#include "..\..\..\..\core\buildInformation.h"
#include "..\..\..\..\core\vr\iVR.h"
#include "..\..\..\..\framework\video\font.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

DEFINE_STATIC_NAME(handyMessage);
DEFINE_STATIC_NAME(back);

//

using namespace Loader;
using namespace HubScenes;

//

REGISTER_FOR_FAST_CAST(HandyMessages);

HandyMessages::HandyMessages()
{
}

void HandyMessages::on_activate(HubScene* _prev)
{
	if (!prevScene.is_set())
	{
		prevScene = _prev;
	}

	base::on_activate(_prev);

	get_hub()->allow_to_deactivate_with_loader_immediately(false);

	get_hub()->deactivate_all_screens();

	//
	idx = NONE;

	// messages
	messages.clear();
	messages.push_back(String(get_build_version_short()));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("EXMs~~(extension modules)")));
	messages.push_back(String(TXT("setting up EXMs")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("fundamental systems~~navigation")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("fundamental systems~~energy pulling")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("manual energy transfer")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("auto energy transfer")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("selecting EXM~~(touch the module)")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("activating EXM~~(press action button~on controller)")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("presets")));
	messages.push_back(String::empty());
	messages.push_back(String(TXT("explore     ~~ experiment ~~  evangelize")));
	messages.push_back(String::empty());
}

void HandyMessages::on_update(float _deltaTime)
{
	base::on_update(_deltaTime);

	int prevIdx = idx;
	if ((VR::IVR::get() && (VR::IVR::get()->get_controls().has_button_been_pressed(VR::Input::Button::WithSource(VR::Input::Button::Trigger, VR::Input::Devices::all, Hand::Right)) ||
							VR::IVR::get()->get_controls().has_button_been_pressed(VR::Input::Button::WithSource(VR::Input::Button::PointerPinch, VR::Input::Devices::all, Hand::Right))))
#ifdef AN_STANDARD_INPUT
		|| ::System::Input::get()->has_key_been_pressed(::System::Key::Space)
#endif
		)
	{
		++idx;
	}
	if ((VR::IVR::get() && (VR::IVR::get()->get_controls().has_button_been_pressed(VR::Input::Button::WithSource(VR::Input::Button::Trigger, VR::Input::Devices::all, Hand::Left)) ||
							VR::IVR::get()->get_controls().has_button_been_pressed(VR::Input::Button::WithSource(VR::Input::Button::PointerPinch, VR::Input::Devices::all, Hand::Left))))
#ifdef AN_STANDARD_INPUT
		|| ::System::Input::get()->has_key_been_pressed(::System::Key::LeftCtrl)
#endif
		)
	{
		-- idx;
	}

	idx = clamp(idx, -1, messages.get_size());

	if (idx != prevIdx)
	{
		get_hub()->keep_only_screen(NAME(back));

		if (messages.is_index_valid(idx) && !messages[idx].is_empty())
		{
			HubScreens::Message::show(get_hub(), messages[idx], nullptr);
		}
	}

	{
		HubScreen* screen = get_hub()->get_screen(NAME(back));
		if (!screen)
		{
			Vector2 ppa = Vector2(24.0f, 24.0f);

			String text(TXT("<<<<"));

			Rotator3 atDir = get_hub()->get_background_dir_or_main_forward();
			atDir.pitch = 85.0f;

			Vector2 sizeInPixels = Vector2(text.get_length() + 0.0f, 1.0f) * HubScreen::s_fontSizeInPixels;

			Vector2 borderSize = Vector2(3.0f, 3.0f);

			sizeInPixels *= 2.0f;
			sizeInPixels += borderSize * 2.0f;

			HubScreen* screen = new HubScreen(get_hub(), NAME(back), sizeInPixels / ppa,
				atDir, get_hub()->get_radius() * 0.7f, false, NP, ppa);
			screen->followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;

			{
				Range2 at = screen->mainResolutionInPixels;
				auto* w = new HubWidgets::Button(NAME(back), at, text);
				w->on_click = [this](HubWidget* _widget, Vector2 const& _at, HubInputFlags::Flags _flags)
				{
					get_hub()->set_scene(prevScene.get());
				};
				screen->add_widget(w);
			}

			screen->activate();
			get_hub()->add_screen(screen);
		}
	}
}
