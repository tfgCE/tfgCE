#include "gse_cameraRumble.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\modules\moduleAI.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\..\framework\ai\aiMessage.h"
#include "..\..\..\framework\gameScript\gameScript.h"
#include "..\..\..\framework\library\usedLibraryStored.inl"
#include "..\..\..\framework\module\modulePresence.h"
#include "..\..\..\framework\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

bool CameraRumble::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	{
		float temp_maxLocationOffset = _node->get_float_attribute_or_from_child(TXT("maxLocationOffsetDistance"), 0.0f);
		if (temp_maxLocationOffset > 0.0f)
		{
			maxLocationOffset = Range3(Range(-temp_maxLocationOffset, temp_maxLocationOffset), Range(-temp_maxLocationOffset, temp_maxLocationOffset), Range(-temp_maxLocationOffset, temp_maxLocationOffset));
		}
	}

	maxLocationOffset.load_from_xml(_node, TXT("maxLocationOffset"));
	maxOrientationOffset.load_from_xml(_node, TXT("maxOrientationOffset"));
	blendTime.load_from_xml(_node, TXT("blendTime"));
	interval.load_from_xml(_node, TXT("interval"));

	blendInTime.load_from_xml(_node, TXT("blendInTime"));
	blendOutTime.load_from_xml(_node, TXT("blendOutTime"));

	reset = _node->get_bool_attribute_or_from_child_presence(TXT("reset"), reset);

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type CameraRumble::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (reset)
		{
			game->reset_camera_rumble();
		}
		else
		{
			Game::CameraRumbleParams params;
			params.maxLocationOffset = maxLocationOffset;
			params.maxOrientationOffset = maxOrientationOffset;
			params.blendTime = blendTime;
			params.interval = interval;

			params.blendInTime = blendInTime;
			params.blendOutTime = blendOutTime;
			game->set_camera_rumble(params);
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
