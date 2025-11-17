#include "lhc_satelliteMap.h"

#include "..\..\loaderHub.h"
#include "..\..\loaderHubWidget.h"

#include "..\..\widgets\lhw_customDraw.h"

#include "..\..\..\..\tutorials\tutorialSystem.h"

#include "..\..\..\..\..\core\system\video\video3dPrimitives.h"

//

using namespace Loader;
using namespace HubScreens;

//

DEFINE_STATIC_NAME(prayStation);
DEFINE_STATIC_NAME(satelliteMap);

//

REGISTER_FOR_FAST_CAST(SatelliteMap);

Name const& SatelliteMap::name()
{
	return NAME(prayStation);
}

SatelliteMap* SatelliteMap::show(Hub* _hub, Optional<Name> const& _name, Optional<Vector2> const& _size, Optional<Rotator3> const& _at, Optional<float> const & _radius, Optional<bool> const& _beVertical, Optional<Rotator3> const& _verticalOffset, Optional<Vector2> const& _pixelsPerAngle)
{
	if (auto* s = _hub->get_screen(_name.get(name())))
	{
		// already there
		return fast_cast<SatelliteMap>(s);
	}

	Vector2 screenSizeInPixels = Vector2(600.0f, 350.0f);
	Vector2 ppa = _pixelsPerAngle.get(Vector2(10.0f, 10.0f));

	SatelliteMap* newSM = new SatelliteMap(_hub, _name.get(name()), _size.get(screenSizeInPixels / ppa), _at.get(_hub->get_background_dir_or_main_forward()), _radius.get(_hub->get_radius() * 0.7f), _beVertical.get(false), _verticalOffset.get(Rotator3::zero), ppa);

	newSM->activate();
	newSM->notAvailableToCursor = true;
	newSM->dontClearOnRender = true;
	newSM->neverGoesToBackground = true;
	_hub->add_screen(newSM);

	return newSM;
}

SatelliteMap::SatelliteMap(Hub* _hub, Name const& _name, Vector2 const& _size, Rotator3 const& _at, float _radius, Optional<bool> const& _beVertical, Optional<Rotator3> const& _verticalOffset, Optional<Vector2> const& _pixelsPerAngle)
: HubScreen(_hub, _name, _size, _at, _radius, _beVertical, _verticalOffset, _pixelsPerAngle)
{
	contextKeeper = new Context();
	RefCountObjectPtr<Context> context = contextKeeper;
	Hub* hub = this->hub;
	auto* w = new HubWidgets::CustomDraw(NAME(satelliteMap), wholeResolutionInPixels, nullptr);
	w->custom_draw = [context, hub, w](Framework::Display* _display, Range2 const& _at)
	{
		if (context->clear || context->next)
		{
			if (context->clear)
			{
				::System::Video3DPrimitives::fill_rect_2d(Colour::none, Vector2::zero, _display->get_setup().wholeDisplayResolution.to_vector2(), false);
				::System::Video3DPrimitives::fill_rect_2d(TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(HubColours::screen_border(), w->screen, w),
					_display->get_left_bottom_of_screen() - Vector2(2.0f),
					_display->get_right_top_of_screen() + Vector2(2.0f), false);
				::System::Video3DPrimitives::fill_rect_2d(TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(hub->get_screen_interior_colour(), w->screen, w), _display->get_left_bottom_of_screen(), _display->get_right_top_of_screen(), false);
			}

			context->pointsLeft = 0;
			context->levelsLeft = Random::get_int_from_range(10, 20);

			context->delay = 0.0f;
			context->waitTime = 2.0f;

			context->clear = max(0, context->clear - 1);
			context->next = false;
		}

		if (!context->clear)
		{
			if (context->levelsLeft > 0)
			{
				context->delay -= context->deltaTime;
				context->delay = max(context->delay, -0.1f); // don't push too many primitives
				while (context->delay <= 0.0f)
				{
					context->delay += 0.0001f;
					--context->pointsLeft;

					if (context->pointsLeft <= 0)
					{
						context->pointsLeft = 20000;
						context->at = Vector2((float)Random::get_int_from_range((int)context->mainResolutionInPixels.x.get_at(0.2f), (int)context->mainResolutionInPixels.x.get_at(0.8f)),
											  (float)Random::get_int_from_range((int)context->mainResolutionInPixels.y.get_at(0.2f), (int)context->mainResolutionInPixels.y.get_at(0.8f)));
						--context->levelsLeft;
						context->colour.r = Random::get_float(0.9f, 1.0f);
						context->colour.g = Random::get_float(0.9f, 1.0f);
						context->colour.b = Random::get_float(0.9f, 1.0f);
						context->colour.a = 1.0f;
						context->colour *= HubColours::text();
						context->colour.a = 1.0f;
						context->colour = TeaForGodEmperor::TutorialSystem::adjust_colour_for_hub(context->colour, w->screen, w);
					}
					if (Random::get_bool())
					{
						context->at.x += Random::get_bool() ? 2 : -2;
					}
					else
					{
						context->at.y += Random::get_bool() ? 2 : -2;
					}
					float border = 5.0f;
					if (context->at.x <= context->mainResolutionInPixels.x.min + border ||
						context->at.x >= context->mainResolutionInPixels.x.max - border ||
						context->at.y <= context->mainResolutionInPixels.y.min + border ||
						context->at.y >= context->mainResolutionInPixels.y.max - border)
					{
						context->pointsLeft = 0;
						continue;
					}
					::System::Video3DPrimitives::point_2d(context->colour, context->at, false);
				}
			}
			else
			{
				context->waitTime -= context->deltaTime;
				if (context->waitTime < 0.0f)
				{
					context->next = true;
					context->clear = 5;
				}
			}
		}
		context->deltaTime = 0.0f;
	};
	w->always_requires_rendering();
	add_widget(w);
}

void SatelliteMap::advance(float _deltaTime, bool _beyondModal)
{
	contextKeeper->deltaTime += _deltaTime;
	contextKeeper->mainResolutionInPixels = mainResolutionInPixels;

	base::advance(_deltaTime, _beyondModal);
}
