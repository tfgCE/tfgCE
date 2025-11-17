#include "gse_forceSoundCamera.h"

#include "..\..\game\game.h"
#include "..\..\tutorials\tutorialSystem.h"

#include "..\..\..\core\random\random.h"
#include "..\..\..\core\system\core.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;
using namespace GameScript;
using namespace Elements;

//

// variable
DEFINE_STATIC_NAME(wait);
DEFINE_STATIC_NAME(startCameraPlacement);
DEFINE_STATIC_NAME(startCameraFOV);

//

bool ForceSoundCamera::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	placement.load_from_xml(_node, TXT("placement"));

	inRoom.load_from_xml_attribute_or_child_node(_node, TXT("inRoom"));
	inRoomVar = _node->get_name_attribute_or_from_child(TXT("inRoomVar"), inRoomVar);

	drop = _node->get_bool_attribute_or_from_child_presence(TXT("drop"), drop);

	offsetPlacementRelativelyToParam = Name::invalid();
	for_every(node, _node->children_named(TXT("offsetPlacementRelatively")))
	{
		offsetPlacementRelativelyToParam = node->get_name_attribute(TXT("toParam"));
	}

	return result;
}

Framework::GameScript::ScriptExecutionResult::Type ForceSoundCamera::execute(Framework::GameScript::ScriptExecution& _execution, Framework::GameScript::ScriptExecution::Flags _flags) const
{
	if (auto* game = Game::get_as<Game>())
	{
		if (drop)
		{
			game->clear_forced_sound_camera();

			return Framework::GameScript::ScriptExecutionResult::Continue;
		}
		else
		{
			Transform currentPlacement = game->get_forced_sound_camera_placement();
			Framework::Room * currentInRoom = game->get_forced_sound_camera_in_room();

			Transform newPlacement = currentPlacement;
			if (placement.is_set())
			{
				newPlacement = placement.get();
			}

			if (inRoomVar.is_valid())
			{
				if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(inRoomVar))
				{
					currentInRoom = exPtr->get();
				}
			}

			if (!inRoom.is_empty() && game->get_world())
			{
				for_every_ptr(r, game->get_world()->get_all_rooms())
				{
					if (inRoom.check(r->get_tags()))
					{
						currentInRoom = r;
						break;
					}
				}
			}

			{
				if (offsetPlacementRelativelyToParam.is_valid() && currentInRoom)
				{
					Transform offsetPlacement = currentInRoom->get_variables().get_value<Transform>(offsetPlacementRelativelyToParam, Transform::identity);
					newPlacement = offsetPlacement.to_world(newPlacement);
				}
			}

			game->set_forced_sound_camera(newPlacement, currentInRoom);
		}
	}

	return Framework::GameScript::ScriptExecutionResult::Continue;
}
