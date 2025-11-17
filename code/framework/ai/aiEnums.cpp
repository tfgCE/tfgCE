#include "aiEnums.h"

#include "aiLocomotion.h"

#include "..\module\moduleController.h"

#include "..\..\core\types\string.h"

using namespace Framework;
using namespace AI;

LocomotionState::Type LocomotionState::parse(String const & _value, Type _default)
{
	if (_value == TXT("")) return _default;
	if (_value == TXT("no control")) return NoControl;
	if (_value == TXT("stop")) return Stop;
	if (_value == TXT("keep")) return Keep;
	if (_value == TXT("keep controller only")) return KeepControllerOnly;
	error(TXT("locomotion state \"%S\" not recognised"), _value.to_char());
	return Keep;
}

void LocomotionState::apply(Type _ls, Locomotion & _locomotion, ModuleController* _controller)
{
	switch (_ls)
	{
	case NoControl:
		_locomotion.dont_control();
		if (_controller)
		{
			_controller->clear_all();
		}
		break;
	case Stop:
		_locomotion.stop();
		if (_controller)
		{
			_controller->clear_all();
		}
		break;
	case KeepControllerOnly:
		_locomotion.dont_control();
		break;
	case Keep:
		break;
	}
}
