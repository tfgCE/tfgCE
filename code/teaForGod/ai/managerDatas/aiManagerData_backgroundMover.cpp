#include "aiManagerData_backgroundMover.h"

#include "..\..\..\core\types\name.h"
#include "..\..\..\framework\library\library.h"

//

using namespace TeaForGodEmperor;
using namespace AI;
using namespace ManagerDatasLib;

//

REGISTER_FOR_FAST_CAST(BackgroundMover);

void BackgroundMover::register_itself()
{
	ManagerDatas::register_data(Name(TXT("background mover")), []() { return new BackgroundMover(); });
}

bool BackgroundMover::load_from_xml(IO::XML::Node const * _node, ::Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	useObjectsTagged.load_from_xml_attribute_or_child_node(_node, TXT("useObjectsTagged"));

	restPlacement.load_from_xml_child_node(_node, TXT("restPlacement"));

	for_every(node, _node->children_named(TXT("move")))
	{
		move.speed.load_from_xml(node, TXT("speed"));
		move.curveRadius.load_from_xml(node, TXT("curveRadius"));
		move.acceleration.load_from_xml(node, TXT("acceleration"));
		move.deceleration.load_from_xml(node, TXT("deceleration"));
		move.dir.load_from_xml(node);
		move.dir.normalise();
		move.autoStart = node->get_bool_attribute_or_from_child_presence(TXT("autoStart"), move.autoStart);
		move.backgroundMoverBasePresenceOwnersTagged.load_from_xml_attribute_or_child_node(node, TXT("backgroundMoverBasePresenceOwnersTagged"));
		move.setRoomVelocityCoef.load_from_xml(node, TXT("setRoomVelocityCoef"));
	}

	for_every(node, _node->children_named(TXT("doors")))
	{
		if (node->has_attribute_or_child(TXT("alongOpenWorldDir")))
		{
			doors.alongOpenWorldDir = DirFourClockwise::parse(node->get_string_attribute_or_from_child(TXT("alongOpenWorldDir")));
		}
		doors.moveDoors = node->get_bool_attribute_or_from_child_presence(TXT("moveDoors"), doors.moveDoors);
		doors.order = node->get_int_attribute_or_from_child(TXT("order"), doors.order);
		doors.noStartingDoor = node->get_bool_attribute_or_from_child_presence(TXT("noStartingDoor"), doors.noStartingDoor);
		doors.anchorPOI = node->get_name_attribute_or_from_child(TXT("anchorPOI"), doors.anchorPOI);
		doors.moveWithBackgroundTagged.load_from_xml_attribute_or_child_node(node, TXT("moveWithBackgroundTagged"));
		for_every(nodeOrder, node->children_named(TXT("order")))
		{
			TagCondition byTagged;
			if (byTagged.load_from_xml_attribute_or_child_node(nodeOrder, TXT("byTagged")))
			{
				doors.orderByTagged.push_back(byTagged);
			}
			else
			{
				error_loading_xml(nodeOrder, TXT("missing \"byTagged\""));
				result = false;
			}
		}
		for_every(subNode, node->children_named(TXT("place")))
		{
			Doors::PlaceDoorAtPOI pdapoi;
			if (pdapoi.tagged.load_from_xml_attribute_or_child_node(subNode, TXT("doorTagged")))
			{
				pdapoi.atPOI = subNode->get_name_attribute_or_from_child(TXT("atPOI"), pdapoi.atPOI);
				if (pdapoi.atPOI.is_valid())
				{
					doors.placeDoorsAtPOI.push_back(pdapoi);
				}
				else
				{
					error_loading_xml(subNode, TXT("missing \"atPOI\""));
					result = false;
				}
			}
			else
			{
				error_loading_xml(subNode, TXT("missing \"doorTagged\""));
				result = false;
			}
		}
	}

	for_every(node, _node->children_named(TXT("chain")))
	{
		chain.appearDistance = node->get_float_attribute_or_from_child(TXT("appearDistance"), chain.appearDistance);
		chain.disappearDistance = node->get_float_attribute_or_from_child(TXT("disappearDistance"), chain.disappearDistance);
		warn_loading_xml_on_assert(chain.appearDistance > 0.0f, node, TXT("appear distance expected to be positive, ignore if you're sure what you're doing"));
		warn_loading_xml_on_assert(chain.disappearDistance < 0.0f, node, TXT("disappear distance expected to be positive, ignore if you're sure what you're doing"));
		chain.anchorPOI = node->get_name_attribute_or_from_child(TXT("anchorPOI"), chain.anchorPOI);
		chain.containsEndPlacement = false;

		RefCountObjectPtr<ChainElement> chainRoot;
		chainRoot = new ChainElement();
		if (chainRoot->load_from_xml(node, OUT_ chain.containsEndPlacement))
		{
			chain.root = chainRoot;
		}
		else
		{
			chain.root.clear();
			result = false;
		}
	}

	sounds.clear();
	for_every(node, _node->children_named(TXT("sound")))
	{
		Sound s;
		s.play = node->get_name_attribute_or_from_child(TXT("play"), s.play);
		s.onSocket.load_from_xml(node, TXT("onSocket"));
		s.onTagged.load_from_xml_attribute_or_child_node(node, TXT("onTagged"));
		s.setSpeedVar = node->get_name_attribute_or_from_child(TXT("setSpeedVar"), s.setSpeedVar);
		sounds.push_back(s);
	}

	for_every(node, _node->children_named(TXT("setDistanceCovered")))
	{
		setDistanceCovered.roomVar = node->get_name_attribute_or_from_child(TXT("roomVar"), setDistanceCovered.roomVar);
		setDistanceCovered.mul = node->get_float_attribute_or_from_child(TXT("mul"), setDistanceCovered.mul);
		if (node->has_attribute_or_child(TXT("div")))
		{
			setDistanceCovered.mul = safe_inv(node->get_float_attribute_or_from_child(TXT("div"), safe_inv(setDistanceCovered.mul)));
		}
	}

	return result;
}

bool BackgroundMover::prepare_for_game(::Framework::Library* _library, ::Framework::LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

void BackgroundMover::log(LogInfoContext& _log) const
{
	base::log(_log);
	LOG_INDENT(_log);
}

//

bool BackgroundMover::ChainElement::load_from_xml(IO::XML::Node const* _node, OUT_ bool& _loadedEndPlacement)
{
	bool result = true;
	if (_node->get_name() == TXT("chain")) type = ChainRoot; else
	if (_node->get_name() == TXT("atZero")) type = AtZero; else
	if (_node->get_name() == TXT("atAppearDistance")) type = AtAppearDistance; else
	if (_node->get_name() == TXT("atDisappearDistance")) type = AtDisappearDistance; else
	if (_node->get_name() == TXT("move")) type = MoveBy; else
	if (_node->get_name() == TXT("addAndMove")) type = AddAndMove; else
	if (_node->get_name() == TXT("addAndStay")) type = AddAndStay; else
	if (_node->get_name() == TXT("setAppearDistance")) type = SetAppearDistance; else
	if (_node->get_name() == TXT("repeat")) type = Repeat; else
	if (_node->get_name() == TXT("repeatTillEnd")) type = RepeatTillEnd; else
	if (_node->get_name() == TXT("endPlacement")) { type = EndPlacement; _loadedEndPlacement = true; } else
	if (_node->get_name() == TXT("end")) type = End; else
	if (_node->get_name() == TXT("idle")) type = Idle; else
	{
		error_loading_xml(_node, TXT("couldn't recognise chain element type \"%S\""), _node->get_name().to_char());
		result = false;
	}

	if (type == Repeat)
	{
		amount.load_from_xml(_node, TXT("amount"));
	}
	if (type == MoveBy)
	{
		valueR.load_from_xml(_node, TXT("by"));
	}
	if (type == SetAppearDistance)
	{
		value = _node->get_float_attribute(TXT("to"), value);
	}
	if (type == AddAndMove ||
		type == AddAndStay)
	{
		for_every(node, _node->children_named(TXT("offset")))
		{
			transformOffset.x.load_from_xml(node, TXT("x"));
			transformOffset.y.load_from_xml(node, TXT("y"));
			transformOffset.z.load_from_xml(node, TXT("z"));
			transformOffset.pitch.load_from_xml(node, TXT("pitch"));
			transformOffset.yaw.load_from_xml(node, TXT("yaw"));
			transformOffset.roll.load_from_xml(node, TXT("roll"));
		}
	}
	tagged.load_from_xml_attribute(_node, TXT("tagged"));
	force = _node->get_bool_attribute_or_from_child_presence(TXT("force"), force);
	endPlacementInCentre = _node->get_bool_attribute_or_from_child_presence(TXT("endPlacementInCentre"), endPlacementInCentre);
	if (endPlacementInCentre)
	{
		_loadedEndPlacement = true;
	}

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("offset"))
		{
			continue;
		}
		RefCountObjectPtr<BackgroundMover::ChainElement> ch;
		ch = new BackgroundMover::ChainElement();
		if (ch->load_from_xml(node, OUT_ _loadedEndPlacement))
		{
			elements.push_back(ch);
		}
		else
		{
			result = false;
		}
	}

	return result;
}
