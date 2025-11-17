#include "lhc_exitIExitable.h"

#include "..\loaderHub.h"
#include "..\loaderHubScreen.h"
#include "..\loaderHubWidget.h"
#include "..\widgets\lhw_button.h"

#ifdef AN_CLANG
#include "..\..\..\..\framework\library\usedLibraryStored.inl"
#endif

//

// localised strings
DEFINE_STATIC_NAME_STR(lsExit, TXT("hub; common; back"));

// screens
DEFINE_STATIC_NAME(exitIExitable);

// id
DEFINE_STATIC_NAME(exitIExitableButton);

//

using namespace Loader;
using namespace HubScreens;

REGISTER_FOR_FAST_CAST(ExitIExitable);

Name const & ExitIExitable::name()
{
	return NAME(exitIExitable);
}

void ExitIExitable::show(Hub* _hub, HubInterfaces::IExitable* _iExitable, Vector2 const& _extraSizePt)
{
	if (_hub->get_screen(name()))
	{
		return;
	}
	
	Vector2 size;
	Rotator3 at;
	float radius;
	bool beVertical;
	Rotator3 verticalOffset;
	Vector2 ppa = Vector2(20.0f, 20.0f);

	_iExitable->get_placement_and_size_for(_hub, name(), size, at, radius, beVertical, verticalOffset, ppa);
	ppa = Vector2(20.0f, 20.0f); // override; what's the reason to override?

	String exit = LOC_STR(NAME(lsExit));

	size = Vector2((float)(2 + exit.get_length()) * HubScreen::s_fontSizeInPixels.x / ppa.x,
				   (float)(3) * HubScreen::s_fontSizeInPixels.y / ppa.y);

	size *= Vector2::one + _extraSizePt;

	ExitIExitable* screen = new ExitIExitable(_hub, _iExitable, size, at, radius, beVertical, verticalOffset, ppa);

	screen->activate();
	_hub->add_screen(screen);
}

ExitIExitable::ExitIExitable(Hub* _hub, HubInterfaces::IExitable* _iExitable, Vector2 const & _size, Rotator3 const & _at, float _radius, bool _beVertical, Rotator3 const & _verticalOffset, Vector2 const & _pixelsPerAngle)
: HubScreen(_hub, name(), _size, _at, _radius, _beVertical, _verticalOffset, _pixelsPerAngle)
, exitable(_iExitable)
{
	Range2 whole = mainResolutionInPixels;
	whole.expand_by(Vector2(-2.0f, -2.0f));
	{
		auto* w = new HubWidgets::Button(NAME(exitIExitableButton), whole, NAME(lsExit));
		w->on_click = [this](HubWidget* _widget, Vector2 const & _at, HubInputFlags::Flags _flags) { exitable->exit_exitable(); };
		add_widget(w);
	}
}
