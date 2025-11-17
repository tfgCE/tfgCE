#include "lhc_spirographMandala.h"

#include "..\..\loaderHub.h"
#include "..\..\loaderHubWidget.h"

#include "..\..\widgets\lhw_customDraw.h"

#include "..\..\..\..\..\core\system\video\video3dPrimitives.h"

//

using namespace Loader;
using namespace HubScreens;

//

DEFINE_STATIC_NAME(prayStation);
DEFINE_STATIC_NAME(spirographMandala);

//

REGISTER_FOR_FAST_CAST(SpirographMandala);

Name const& SpirographMandala::name()
{
	return NAME(prayStation);
}

void SpirographMandala::show(Hub* _hub, Optional<Name> const& _name, Optional<Vector2> const& _size, Optional<Rotator3> const& _at, Optional<float> const & _radius, Optional<bool> const& _beVertical, Optional<Rotator3> const& _verticalOffset, Optional<Vector2> const& _pixelsPerAngle)
{
	if (_hub->get_screen(_name.get(name())))
	{
		// already there
		return;
	}

	Vector2 screenSizeInPixels = Vector2(1000.0f, 350.0f);
	Vector2 ppa = _pixelsPerAngle.get(Vector2(10.0f, 10.0f));

	SpirographMandala* newSM = new SpirographMandala(_hub, _name.get(name()), _size.get(screenSizeInPixels / ppa), _at.get(_hub->get_background_dir_or_main_forward()), _radius.get(_hub->get_radius() * 0.7f), _beVertical.get(false), _verticalOffset.get(Rotator3::zero), ppa);

	newSM->activate();
	newSM->notAvailableToCursor = true;
	newSM->dontClearOnRender = true;
	newSM->neverGoesToBackground = true;
	_hub->add_screen(newSM);
}

SpirographMandala::SpirographMandala(Hub* _hub, Name const& _name, Vector2 const& _size, Rotator3 const& _at, float _radius, Optional<bool> const& _beVertical, Optional<Rotator3> const& _verticalOffset, Optional<Vector2> const& _pixelsPerAngle)
: HubScreen(_hub, _name, _size, _at, _radius, _beVertical, _verticalOffset, _pixelsPerAngle)
{
	contextKeeper = new Context();
	RefCountObjectPtr<Context> context = contextKeeper;
	auto* w = new HubWidgets::CustomDraw(NAME(spirographMandala), wholeResolutionInPixels,
		[context](Framework::Display* _display, Range2 const& _at)
	{
		if (context->clear || context->next)
		{
			if (context->clear)
			{
				::System::Video3DPrimitives::fill_rect_2d(Colour::black, Vector2::zero, _display->get_setup().wholeDisplayResolution.to_vector2(), false);
			}

			float nominalRadius = min(_display->get_setup().wholeDisplayResolution.x, _display->get_setup().wholeDisplayResolution.y) * 0.5f;

			context->delta = Random::get_float(20.0f, 160.0f) * (Random::get_bool() ? 1.0f : -1.0f);
			//a = Random::get_float(0.0f, 360.0f);
			//radius = nominalRadius * Random::get_float(0.2f, 0.7f);
			context->radiusDelta = min(0.0f, Random::get_float(-0.01f, 0.02f));
			if (Random::get_chance(0.25f))
			{
				context->radiusDelta = Random::get_float(0.0f, 0.02f);
			}
			if (context->radius < nominalRadius * 0.1f)
			{
				context->radiusDelta = Random::get_float(0.01f, 0.02f);
			}
			if (context->radius > nominalRadius * 1.2f)
			{
				context->radiusDelta = Random::get_float(-0.02f, -0.01f);
			}
			/*
			colour = Colour::black;
			do
			{
				colour.r = clamp((float)((Random::get_int(4)-1) * 128) / 256.0f, 0.0f, 1.0f);
				colour.g = clamp((float)((Random::get_int(4)-1) * 128) / 256.0f, 0.0f, 1.0f);
				colour.b = clamp((float)((Random::get_int(4)-1) * 128) / 256.0f, 0.0f, 1.0f);
			} while (colour == Colour::black && !next);
			*/
			context->colour.r += Random::get_float(-0.1f, 0.1f);
			context->colour.g += Random::get_float(-0.1f, 0.1f);
			context->colour.b += Random::get_float(-0.1f, 0.1f);
			context->colour.r = clamp(context->colour.r, 0.0f, 1.0f);
			context->colour.g = clamp(context->colour.g, 0.0f, 1.0f);
			context->colour.b = clamp(context->colour.b, 0.0f, 1.0f);
			context->linesLeft = 100 + 10 * Random::get_int(20);
			context->waitTime = 0.0f;
			context->delay = 0.0f;

			context->clear = false;
			context->next = false;
		}

		if (context->linesLeft > 0)
		{
			context->delay -= context->deltaTime;
			context->delay = max(context->delay, -0.1f); // don't push too many primitives
			while (context->delay <= 0.0f)
			{
				context->delay += 0.003f;
				--context->linesLeft;

				Vector2 centre = _display->get_setup().wholeDisplayResolution.to_vector2() * Vector2::half;
				Vector2 at = centre + Vector2(0.0f, context->radius).rotated_by_angle(context->a);
				context->a += context->delta;
				context->radius *= 1.0f + context->radiusDelta;
				Vector2 nat = centre + Vector2(0.0f, context->radius).rotated_by_angle(context->a);
				::System::Video3DPrimitives::line_2d(context->colour.mul_rgb(0.5f), at, nat, false);
			}
		}
		else
		{
			context->waitTime -= context->deltaTime;
			if (context->waitTime < 0.0f)
			{
				context->next = true;
			}
		}
		context->deltaTime = 0.0f;
	});
	w->always_requires_rendering();
	add_widget(w);
}

void SpirographMandala::advance(float _deltaTime, bool _beyondModal)
{
	contextKeeper->deltaTime += _deltaTime;

	base::advance(_deltaTime, _beyondModal);
}
