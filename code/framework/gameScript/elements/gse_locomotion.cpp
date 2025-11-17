#include "gse_locomotion.h"

#include "..\gameScript.h"

#include "..\..\game\game.h"
#include "..\..\library\library.h"
#include "..\..\module\moduleAI.h"

#include "..\..\..\core\io\xml.h"
#include "..\..\ai\aiLocomotion.h"
#include "..\..\ai\aiMindInstance.h"
#include "..\..\module\moduleAI.h"
#include "..\..\module\modulePresence.h"
#include "..\..\nav\navPath.h"
#include "..\..\nav\tasks\navTask_FindPath.h"
#include "..\..\world\doorInRoom.h"
#include "..\..\world\room.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace GameScript;
using namespace Elements;

//

DEFINE_STATIC_NAME(self);

// object variables (runtime/execution)
DEFINE_STATIC_NAME(gseLocomotionPath);
DEFINE_STATIC_NAME(gseLocomotionPathTask);

//

bool Locomotion::load_from_xml(IO::XML::Node const* _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = true;

	objectVar = _node->get_name_attribute(TXT("objectVar"), objectVar);

	if (!objectVar.is_valid())
	{
		objectVar = NAME(self);
	}

	moveToObjectVar = _node->get_name_attribute(TXT("moveToObjectVar"), moveToObjectVar);
	moveToLoc.load_from_xml(_node, TXT("moveToLoc"));

	moveToRadius.load_from_xml(_node, TXT("moveToRadius"));

	stop = _node->get_bool_attribute_or_from_child_presence(TXT("stop"), stop);
	dontControl = _node->get_bool_attribute_or_from_child_presence(TXT("dontControl"), dontControl);

	move3D = ! _node->get_bool_attribute_or_from_child_presence(TXT("move2D"), ! move3D);
	move3D = _node->get_bool_attribute_or_from_child_presence(TXT("move3D"), move3D);

	moveDirectly = _node->get_bool_attribute_or_from_child_presence(TXT("moveDirectly"), moveDirectly);

	movementParameters = Framework::MovementParameters();
	{
		float v = _node->get_float_attribute_or_from_child(TXT("absoluteSpeed"), 0.0f);
		if (v != 0.0f)
		{
			movementParameters.absoluteSpeed = v;
		}
	}
	{
		float v = _node->get_float_attribute_or_from_child(TXT("relativeSpeed"), 0.0f);
		if (v != 0.0f)
		{
			movementParameters.relativeSpeed = v;
		}
	}
	movementParameters.gaitName = _node->get_name_attribute_or_from_child(TXT("gaitName"), movementParameters.gaitName);

	{
		if (!movementParameters.absoluteSpeed.is_set() &&
			!movementParameters.relativeSpeed.is_set() &&
			!movementParameters.gaitName.is_valid())
		{
			movementParameters.full_speed();
		}
	}

	return result;
}

bool Locomotion::prepare_for_game(Framework::Library* _library, Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

ScriptExecutionResult::Type Locomotion::execute(ScriptExecution& _execution, ScriptExecution::Flags _flags) const
{
	if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(objectVar))
	{
		if (auto* imo = exPtr->get())
		{
			Concurrency::ScopedSpinLock lock(imo->access_lock());
			if (auto* ai = imo->get_ai())
			{
				if (stop)
				{
					ai->get_mind()->access_locomotion().stop();
				}
				if (dontControl)
				{
					ai->get_mind()->access_locomotion().dont_control();
				}
				if (moveToLoc.is_set())
				{
					if (move3D)
					{
						ai->get_mind()->access_locomotion().move_to_3d(moveToLoc.get(), moveToRadius.get(0.2f), movementParameters);
						ai->get_mind()->access_locomotion().turn_in_movement_direction_3d();
					}
					else
					{
						ai->get_mind()->access_locomotion().move_to_2d(moveToLoc.get(), moveToRadius.get(0.2f), movementParameters);
						ai->get_mind()->access_locomotion().turn_in_movement_direction_2d();
					}
				}
				else if (moveToObjectVar.is_valid())
				{
					if (moveDirectly)
					{
						Framework::IAIObject* moveToObject = nullptr;
						if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(moveToObjectVar))
						{
							if (auto* timo = exPtr->get())
							{
								if (imo->get_presence()->get_in_room() == timo->get_presence()->get_in_room())
								{
									moveToObject = timo;
								}
								else
								{
									warn(TXT("in a different room, won't move directly"));
									return ScriptExecutionResult::Continue;
								}
							}
						}
						if (moveToObject)
						{
							if (move3D)
							{
								ai->get_mind()->access_locomotion().move_to_3d(moveToObject, moveToRadius.get(0.2f), movementParameters);
								ai->get_mind()->access_locomotion().turn_in_movement_direction_3d();
							}
							else
							{
								ai->get_mind()->access_locomotion().move_to_2d(moveToObject, moveToRadius.get(0.2f), movementParameters);
								ai->get_mind()->access_locomotion().turn_in_movement_direction_2d();
							}
						}
						else
						{
							warn(TXT("couldn't find a target to move to"));
							return ScriptExecutionResult::Continue;
						}
					}
					else
					{
						::Framework::Nav::PathPtr& path = imo->access_variables().access<::Framework::Nav::PathPtr>(NAME(gseLocomotionPath));
						::Framework::Nav::TaskPtr& pathTask = imo->access_variables().access<::Framework::Nav::TaskPtr>(NAME(gseLocomotionPathTask));

						if (is_flag_set(_flags, ScriptExecution::Flag::Entered))
						{
							path.clear();
							pathTask.clear();

							Framework::Nav::PathRequestInfo pathRequestInfo(imo);
							pathRequestInfo.with_dev_info(TXT("GSE locomotion"));

							bool pathTaskSet = false;
							if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::IModulesOwner>>(moveToObjectVar))
							{
								if (auto* mimo = exPtr->get())
								{
									pathTask = ai->get_mind()->access_navigation().find_path_to(mimo, pathRequestInfo);
									pathTaskSet = true;
								}
							}
							if (auto* exPtr = _execution.get_variables().get_existing<SafePtr<Framework::Room>>(moveToObjectVar))
							{
								if (auto* room = exPtr->get())
								{
									pathTask = ai->get_mind()->access_navigation().find_path_to(room, room->get_doors().get_first()->get_placement().get_translation(), pathRequestInfo);
									pathTaskSet = true;
								}
							}

							if (!pathTaskSet)
							{
								warn(TXT("no valid moveToObjectVar for locomotion gamescript element"));
								return ScriptExecutionResult::Continue;
							}
						}

						if (pathTask.is_set())
						{
							if (!pathTask->is_done())
							{
								return ScriptExecutionResult::Repeat;
							}

							if (pathTask->has_succeed())
							{
								if (auto* task = fast_cast<Framework::Nav::Tasks::PathTask>(pathTask.get()))
								{
									path = task->get_path();
								}
								else
								{
									path.clear();
								}

								if (path.is_set())
								{
									if (move3D)
									{
										ai->get_mind()->access_locomotion().follow_path_3d(path.get(), moveToRadius, movementParameters);
										ai->get_mind()->access_locomotion().turn_follow_path_3d();
									}
									else
									{
										ai->get_mind()->access_locomotion().follow_path_2d(path.get(), moveToRadius, movementParameters);
										ai->get_mind()->access_locomotion().turn_follow_path_2d();
									}
								}
							}
							else
							{
								warn(TXT("couldn't find a valid path"));
								return ScriptExecutionResult::Continue;
							}
						}
					}
				}
			}
		}
	}

	return ScriptExecutionResult::Continue;
}
