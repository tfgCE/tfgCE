#include "lhc_handGestures.h"

#include "..\loaderHub.h"
#include "..\loaderHubWidget.h"

#include "..\scenes\lhs_options.h"

#include "..\widgets\lhw_button.h"
#include "..\widgets\lhw_image.h"
#include "..\widgets\lhw_text.h"

#include "..\..\..\..\core\system\core.h"
#include "..\..\..\..\framework\library\library.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsGeneralInfo, TXT("hand gestures; general info"));
DEFINE_STATIC_NAME_STR(lsOptions, TXT("hand gestures; options"));
DEFINE_STATIC_NAME_STR(lsOk, TXT("hub; common; ok"));

DEFINE_STATIC_NAME_STR(lsSelect, TXT("hand gestures; select"));
DEFINE_STATIC_NAME_STR(lsOpenMenu, TXT("hand gestures; open menu"));
DEFINE_STATIC_NAME_STR(lsDeployMainEquipment, TXT("hand gestures; deploy main equipment"));
DEFINE_STATIC_NAME_STR(lsShoot, TXT("hand gestures; shoot"));
DEFINE_STATIC_NAME_STR(lsShootThumb, TXT("hand gestures; shoot thumb"));
DEFINE_STATIC_NAME_STR(lsGrab, TXT("hand gestures; grab"));
DEFINE_STATIC_NAME_STR(lsRelease, TXT("hand gestures; release"));
DEFINE_STATIC_NAME_STR(lsShowOverlayInfo, TXT("hand gestures; show overlay info"));
DEFINE_STATIC_NAME_STR(lsUseEXM, TXT("hand gestures; use EXM"));
DEFINE_STATIC_NAME_STR(lsRightHand, TXT("hand gestures; right hand"));
DEFINE_STATIC_NAME_STR(lsLeftHand, TXT("hand gestures; left hand"));

// global references
DEFINE_STATIC_NAME_STR(grSelect, TXT("hand gestures; select"));
DEFINE_STATIC_NAME_STR(grOpenMenu, TXT("hand gestures; open menu"));
DEFINE_STATIC_NAME_STR(grDeployMainEquipment, TXT("hand gestures; deploy main equipment"));
DEFINE_STATIC_NAME_STR(grShoot, TXT("hand gestures; shoot"));
DEFINE_STATIC_NAME_STR(grShootThumb, TXT("hand gestures; shoot thumb"));
DEFINE_STATIC_NAME_STR(grGrab, TXT("hand gestures; grab"));
DEFINE_STATIC_NAME_STR(grRelease, TXT("hand gestures; release"));
DEFINE_STATIC_NAME_STR(grShowOverlayInfo, TXT("hand gestures; show overlay info"));
DEFINE_STATIC_NAME_STR(grUseEXM, TXT("hand gestures; use EXM"));

// game inputs
DEFINE_STATIC_NAME(select);
DEFINE_STATIC_NAME(requestInGameMenu);
DEFINE_STATIC_NAME(deployMainEquipment);
DEFINE_STATIC_NAME(useEquipment);
DEFINE_STATIC_NAME(grabEquipment);
DEFINE_STATIC_NAME(releaseEquipment);
DEFINE_STATIC_NAME(showOverlayInfo);
DEFINE_STATIC_NAME(useEXM);
DEFINE_STATIC_NAME(autoShootThumb);

// game input definitions
DEFINE_STATIC_NAME(game);
DEFINE_STATIC_NAME(loaderHub);

//

using namespace Loader;
using namespace HubScreens;

REGISTER_FOR_FAST_CAST(HandGestures);

#define USE_MARGIN (1.0f * max(fs.x, fs.y))

HandGestures* HandGestures::create(Hub* _hub, std::function<void()> _ok, Optional<bool> const& _allowOptionsButton)
{
	Vector2 ppa = Vector2(32.0f, 32.0f);
	Vector2 size(70.0f, 80.0f);
	HandGestures* newHandGestures = new HandGestures(_hub, _ok, _allowOptionsButton, size, _hub->get_last_view_rotation() * Rotator3(0.0f, 1.0f, 0.0f), _hub->get_radius() * 0.5f, ppa);
	return newHandGestures;
}

HandGestures* HandGestures::show(Hub * _hub, std::function<void()> _ok, Optional<bool> const& _allowOptionsButton)
{
	HandGestures* newHandGestures = create(_hub, _ok, _allowOptionsButton);
	newHandGestures->activate();
	_hub->add_screen(newHandGestures);
	return newHandGestures;
}

HandGestures::HandGestures(Hub* _hub, std::function<void()> _ok, Optional<bool> const& _allowOptionsButton, Vector2 const& _size, Rotator3 const& _at, float _radius, Vector2 const& _ppa)
: HubScreen(_hub, Name::invalid(), _size, _at, _radius, NP, NP, _ppa)
, onOk(_ok)
{
	SET_EXTRA_DEBUG_INFO(elements, TXT("HandGestures.elements"));

	be_modal();

	tryHand = Hand::Right;
	if (auto* vr = VR::IVR::get())
	{
		tryHand = vr->get_dominant_hand();
	}

	gameInput.use(NAME(game));
	hubInput.use(NAME(loaderHub));
	leftGameInput.use(NAME(game));
	leftHubInput.use(NAME(loaderHub));
	rightGameInput.use(NAME(game));
	rightHubInput.use(NAME(loaderHub));

	Vector2 fs = HubScreen::s_fontSizeInPixels;
	float fsxy = max(fs.x, fs.y);

	Vector2 border(fsxy, fsxy);
	float spacing = fsxy;

	Range2 spaceAvailable = mainResolutionInPixels.expanded_by(-border);

	{
		Range2 r = spaceAvailable;
		float off = 0.3f;
		r.x.min = spaceAvailable.x.get_at(0.5f - off);
		r.x.max = spaceAvailable.x.get_at(0.5f + off);
		auto* t = new HubWidgets::Text(Name::invalid(), r, NAME(lsGeneralInfo));
		t->alignPt.y = 1.0f;
		add_widget(t);
		float tSize = t->calculate_vertical_size();
		spaceAvailable.y.max -= tSize + spacing;
		t->at.y.min = t->at.y.max - tSize;
	}

	shootTP = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grShoot));
	shootThumbTP = Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grShootThumb));

	int dh = 1; // dominant hand
	if (auto* vr = VR::IVR::get())
	{
		dh = vr->get_dominant_hand() == Hand::Right ? 1 : -1;
	}

	elements.clear();
	elements.set_size(2);
	elements[0].push_back(Element( NP,	NAME(lsSelect), NAME(select), false, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grSelect))));
	elements[1].push_back(Element(-dh,	NAME(lsOpenMenu), NAME(requestInGameMenu), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grOpenMenu))));
	elements[0].push_back(Element( NP,	NAME(lsDeployMainEquipment), NAME(deployMainEquipment), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grDeployMainEquipment))));
	elements[1].push_back(Element( NP,	NAME(lsShoot), NAME(useEquipment), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grShoot))));
	elements[0].push_back(Element( NP,	NAME(lsGrab), NAME(grabEquipment), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grGrab))));
	elements[1].push_back(Element( NP,	NAME(lsRelease), NAME(releaseEquipment), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grRelease))));
	elements[0].push_back(Element( NP,	NAME(lsShowOverlayInfo), NAME(showOverlayInfo), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grShowOverlayInfo))));
	elements[1].push_back(Element( NP,	NAME(lsUseEXM), NAME(useEXM), true, Framework::Library::get_current()->get_global_references().get<Framework::TexturePart>(NAME(grUseEXM))));

	Vector2 imageScale = Vector2(2.0f);

	for_every(row, elements)
	{
		if (row->is_empty())
		{
			continue;
		}

		Range2 rowSpace = spaceAvailable;

		float rowLow = rowSpace.y.max;
		for_every(e, *row)
		{
			if (e->tp)
			{
				rowLow = min(rowLow, rowSpace.y.max - e->tp->get_size().y * imageScale.y + 1.0f);
			}
		}

		float rs = (float)row->get_size();
		float elementWidth = floor((rowSpace.x.length() - spacing * (rs - 1.0f)) / rs);
		float startsAt = round(rowSpace.x.centre() - (elementWidth * rs + spacing * (rs - 1.0f)) * 0.5f);

		float elementBy = elementWidth + spacing;
		for_every(e, *row)
		{
			if (e->tp)
			{
				float ei = (float)for_everys_index(e);
				Range2 r(Range(startsAt + elementBy * ei, startsAt + elementBy * ei + elementWidth - 1.0f), Range(rowLow, rowSpace.y.max));
				auto* t = new HubWidgets::Image(e->locStr, r, e->tp, imageScale);
				add_widget(t);
				e->imgWidget = t;
			}
		}

		rowLow -= spacing;
		rowSpace.y.max = rowLow;

		for_every(e, *row)
		{
			if (e->tp)
			{
				float ei = (float)for_everys_index(e);
				Range2 r(Range(startsAt + elementBy * ei, startsAt + elementBy * ei + elementWidth - 1.0f), rowSpace.y);
				auto* t = new HubWidgets::Text(e->locStr, r, e->locStr);
				t->alignPt.y = 1.0f; // align to top
				add_widget(t);
				float tSize = t->calculate_vertical_size();
				rowLow = min(rowLow, rowSpace.y.max - tSize);
				t->at.y.min = t->at.y.max - tSize; // to fix compression
				e->textWidget = t;
			}
		}

		spaceAvailable.y.max = rowLow - spacing;
	}

	{
		String ok = LOC_STR(NAME(lsOk));
		
		float scale = 2.0f;

		RefCountObjectPtr<HubWidget> okButton;
		Array<HubScreenButtonInfo> buttons;
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsLeftHand), [this]() { tryHand = (tryHand == Hand::Left ? Hand::Right : Hand::Left); }).store_widget_in(&handWidget));
		buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOk), [this]() { if (onOk) onOk();  deactivate(); }).store_widget_in(&okButton));
		if (_allowOptionsButton.get(true))
		{
			buttons.push_back(HubScreenButtonInfo(Name::invalid(), NAME(lsOptions), [this]() { hub->set_scene(new HubScenes::Options(HubScenes::Options::InGame)); }));
		}

		Range2 at = spaceAvailable;
		at.y.max = at.y.min + fs.y * 2.0f * scale;
		float off = 0.15f;
		at.x.max = round(spaceAvailable.x.get_at(0.5f + off));
		at.x.min = round(spaceAvailable.x.get_at(0.5f - off));
		add_button_widgets(buttons, at, spacing);

		if (auto* b = fast_cast<HubWidgets::Button>(okButton.get()))
		{
			b->scale = Vector2(scale);
		}
	}

	compress_vertically(spacing);

	followYawDeadZoneByBorder = HubScreen::DEFAULT_YAW_DEAD_ZONE_BY_BORDER;
}

void HandGestures::advance(float _deltaTime, bool _beyondModal)
{
	base::advance(_deltaTime, _beyondModal);
	
	gameInput.use(tryHand == Hand::Left ? Framework::GameInputIncludeVR::Left : Framework::GameInputIncludeVR::Right);
	hubInput.use(tryHand == Hand::Left ? Framework::GameInputIncludeVR::Left : Framework::GameInputIncludeVR::Right);
	leftGameInput.use(Framework::GameInputIncludeVR::Left);
	leftHubInput.use(Framework::GameInputIncludeVR::Left);
	rightGameInput.use(Framework::GameInputIncludeVR::Right);
	rightHubInput.use(Framework::GameInputIncludeVR::Right);
	if (auto* w = fast_cast<HubWidgets::Button>(handWidget.get()))
	{
		w->set(tryHand == Hand::Left ? NAME(lsLeftHand) : NAME(lsRightHand));
	}

	for_every(row, elements)
	{
		for_every(e, *row)
		{
			if (auto* w = fast_cast<HubWidgets::Image>(e->imgWidget.get()))
			{
				w->scale.x = abs(w->scale.x);
				if ((e->hand.is_set() && e->hand.get() < 0) ||
					(!e->hand.is_set() && tryHand == Hand::Left))
				{
					w->scale.x = -w->scale.x;
				}
				auto* t = fast_cast<HubWidgets::Text>(e->textWidget.get());
				bool colourise = false;
				//float period = 0.5f;
				//if (::System::Core::get_timer_mod(period) < period * 0.667f)
				{
					if (e->inputButton.is_valid())
					{
						auto& useInput = e->gameInput ? (e->hand.is_set()? (e->hand.get() > 0? rightGameInput : leftGameInput) : gameInput)
													  : (e->hand.is_set() ? (e->hand.get() > 0 ? rightHubInput : leftHubInput) : hubInput);
						colourise = useInput.is_button_pressed(e->inputButton);

						if (e->inputButton == NAME(useEquipment))
						{
							if (useInput.is_button_pressed(NAME(autoShootThumb)))
							{
								w->texturePart = shootThumbTP;
								if (t) t->set(NAME(lsShootThumb));
							}
							else
							{
								w->texturePart = shootTP;
								if (t) t->set(NAME(lsShoot));
							}
						}
					}
				}
				w->colour = colourise ? Colour::yellow : Colour::white;
				w->mark_requires_rendering();
			}
		}
	}
}
