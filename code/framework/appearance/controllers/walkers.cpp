#include "walkers.h"

#include "..\..\collision\checkCollisionContext.h"
#include "..\..\collision\checkSegmentResult.h"

#include "..\..\library\library.h"

#include "..\..\ai\aiMindInstance.h"
#include "..\..\meshGen\meshGenGenerationContext.h"
#include "..\..\meshGen\meshGenParamImpl.inl"
#include "..\..\module\moduleAppearance.h"
#include "..\..\module\moduleAI.h"
#include "..\..\module\moduleCollision.h"
#include "..\..\module\moduleController.h"
#include "..\..\module\moduleMovement.h"
#include "..\..\module\modulePresence.h"
#include "..\..\module\moduleSound.h"
#include "..\..\world\room.h"

#include "..\..\..\core\debug\debugRenderer.h"
#include "..\..\..\core\debug\extendedDebug.h"

#include "..\..\..\core\mesh\pose.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

#ifdef AN_DEVELOPMENT
//#define INSPECT__START_LEG_MOVEMENT
#endif

//

using namespace Framework;
using namespace AppearanceControllersLib;
using namespace Walkers;

//

DEFINE_STATIC_NAME(walker);

//

bool Event::load_from_xml(IO::XML::Node const * _node, tchar const * _nameAttr)
{
	bool result = true;

	atPt = _node->get_float_attribute_or_from_child(TXT("atPt"), atPt);
	length = _node->get_float_attribute_or_from_child(TXT("length"), length);
	Range pt(atPt, atPt + length);
	pt.load_from_xml(_node, TXT("pt"));
	atPt = pt.min;
	length = pt.length();
	length = _node->get_float_attribute_or_from_child(TXT("length"), length);
	name = _node->get_name_attribute_or_from_child(_nameAttr, name);

	return result;
}

int/*Flags*/ Event::get_state(float _prevPt, float _newPt) const
{
	int result = 0;
	float diff = mod(_newPt - _prevPt, 1.0f);
	if (diff < 0.5f || diff == 1.0f)
	{
		float prevPt = mod(_prevPt - atPt, 1.0f);
		float newPt = mod(_newPt - atPt, 1.0f);

		if (length == 0.0f)
		{
			if (prevPt > newPt) // newPt passed 0, prevPt not yet
			{
				result |= E_Trigerred;
			}
		}
		else
		{
			if (prevPt > newPt) // newPt passed 0, _prevPt not yet
			{
				result |= E_Started;
			}
			if (newPt < length)
			{
				result |= E_Active;
			}
			if (newPt >= length &&
				((prevPt < newPt && prevPt < length) ||
				(prevPt > newPt)))
			{
				result |= E_Ended;
			}
		}
	}
	else
	{
		//todo_important(TXT("backwards?"));
	}
	return result;
}

//

bool EventSound::load_from_xml(IO::XML::Node const * _node, tchar const * _nameAttr)
{
	bool result = base::load_from_xml(_node, _nameAttr);

	detach = _node->get_bool_attribute_or_from_child_presence(TXT("detached"), detach);

	return result;
}

//

bool Events::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	for_every(container, _node->children_named(TXT("sounds")))
	{
		for_every(node, container->children_named(TXT("soundLoop")))
		{
			EventSound e;
			if (e.load_from_xml(node, TXT("id")))
			{
				e.be_full();
				sounds.push_back(e);
			}
			else
			{
				result = false;
			}
		}
		for_every(node, container->children_named(TXT("sound")))
		{
			EventSound e;
			if (e.load_from_xml(node, TXT("id")))
			{
				sounds.push_back(e);
			}
			else
			{
				result = false;
			}
		}
		for_every(node, container->children_named(TXT("footStep")))
		{
			EventSound e;
			if (e.load_from_xml(node, TXT("id")))
			{
				footStepSounds.push_back(e);
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

//

void EventsInstance::do_foot_step(Walkers::Instance & _instance, Walkers::InstanceContext const & _context, Events const * _events, RelativeToPresencePlacement const & _whereToPlay, PhysicalMaterial const * _hitMaterial)
{
	if (_events && !_events->footStepSounds.is_empty())
	{
		if (auto* imo = _context.get_owner()->get_owner())
		{
			if (auto* ms = imo->get_sound())
			{
				auto* inRoom = imo->get_presence()->get_in_room();
				Transform placement = imo->get_presence()->get_centre_of_presence_transform_WS();
				if (_whereToPlay.is_active())
				{
					if (auto* room = _whereToPlay.get_in_final_room())
					{
						inRoom = room;
						placement = _whereToPlay.get_placement_in_final_room();
					}
				}
				if (inRoom)
				{
					// foot step sounds
					for_every(s, _events->footStepSounds)
					{
						// just play it once
						ms->play_sound_in_room(s->name, inRoom, placement);
					}

					// through material
					if (_hitMaterial)
					{
						_hitMaterial->play_foot_step(imo, inRoom, placement);
					}
				}
			}
		}
	}
}

void EventsInstance::stop_sounds()
{
	for_every(sound, sounds)
	{
		sound->sound->stop();
	}
	sounds.clear();
}

void EventsInstance::advance(Walkers::Instance & _instance, Walkers::InstanceContext const & _context, Events const * _events, float _prevPt, float _currPt, bool _justSet, Optional<Name> _soundSocket)
{
	if (!_events)
	{
		stop_sounds();
	}
	else
	{
		if (events != _events)
		{
			_justSet = true;
		}

		for(int idx = 0; idx < sounds.get_size(); ++ idx)
		{
			auto & sound = sounds[idx];
			bool exists = false;
			if (sound.source && _events)
			{
				for_every(s, _events->sounds)
				{
					if (s->name == sound.source->name)
					{
						if (s->get_state(_prevPt, _currPt) & Event::E_Active)
						{
							sound.source = s;
							exists = true;
							break;
						}
					}
				}
			}
			if (!exists)
			{
				sound.source = nullptr;
			}
			if (!sound.source ||
				sound.source->get_state(_prevPt, _currPt) & Event::E_Ended)
			{
				sound.sound->stop();
				sounds.remove_at(idx);
				--idx;
			}
		}
	}

	events = _events;

	if (events)
	{
		if (auto* imo = _context.get_owner()->get_owner())
		{
			if (auto* ms = imo->get_sound())
			{
				for_every(s, events->sounds)
				{
					int state = s->get_state(_prevPt, _currPt);
					SoundSource* played = nullptr;
					if (state & Event::E_Trigerred)
					{
						// just play it once
						played = s->detach ? ms->play_sound_in_room(s->name, _soundSocket) : ms->play_sound(s->name, _soundSocket);
					}
					else
					{
						// check if maybe it should be playing but we're not playing it yet
						if (state & Event::E_Active)
						{
							bool exists = false;
							for_every(sound, sounds)
							{
								if (sound->source && sound->source->name == s->name)
								{
									exists = true;
									break;
								}
							}
							if (!exists)
							{
								state |= Event::E_Started;
							}
						}
						if (state & Event::E_Started)
						{
							played = s->detach ? ms->play_sound_in_room(s->name, _soundSocket) : ms->play_sound(s->name, _soundSocket);
							if (played)
							{
								if (state & Event::E_Ended)
								{
									played->stop();
								}
								else
								{
									sounds.push_back(EventSoundInstance(s, played));
								}
							}
						}
					}
				}
			}
		}
	}
}

//

bool CustomGaitVar::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	varID = _node->get_name_attribute(TXT("name"), varID);
	result &= varID.is_valid();

	value = _node->get_float_attribute(TXT("value"), value);

	return result;
}

//

bool TrajectoryFrame::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	atTime = _node->get_float_attribute(TXT("atTime"), atTime);
	result &= placement.load_from_xml(_node);

	for_every(node, _node->children_named(TXT("var")))
	{
		CustomGaitVar var;
		if (var.load_from_xml(node, _lc))
		{
			insert_or_update(var);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

CustomGaitVar const * TrajectoryFrame::get_gait_var(Name const & _varID) const
{
	for_every(gaitVar, gaitVars)
	{
		if (_varID == gaitVar->varID)
		{
			return gaitVar;
		}
	}
	return nullptr;
}

bool TrajectoryFrame::has_gait_var(Name const & _varID) const
{
	return get_gait_var(_varID) != nullptr;
}

CustomGaitVar & TrajectoryFrame::make_sure_exists(Name const & _varID)
{
	int index = 0;
	for_every(gaitVar, gaitVars)
	{
		if (_varID == gaitVar->varID)
		{
			return *gaitVar;
		}
		if (_varID.get_index() <= gaitVar->varID.get_index())
		{
			break;
		}
		++index;
	}
	gaitVars.insert_at(index, CustomGaitVar(_varID, 0.0f, nullptr));
	return gaitVars[index];
}

void TrajectoryFrame::insert_or_update(CustomGaitVar const & _gaitVar)
{
	make_sure_exists(_gaitVar.varID) = _gaitVar;
}

//

Trajectory::Trajectory()
: length(0.0f)
{
}

bool Trajectory::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for gait"));
		result = false;
	}

	length = _node->get_float_attribute(TXT("length"), length);
	trajectoryObjectSize = _node->get_float_attribute(TXT("trajectoryObjectSize"), trajectoryObjectSize);
	if (trajectoryObjectSize == 0.0f)
	{
		error_loading_xml(_node, TXT("invalid trajectoryObjectSize for trajectory"));
		return false;
	}

	for_every(container, _node->children_named(TXT("frames")))
	{
		for_every(node, container->children_named(TXT("frame")))
		{
			TrajectoryFrame frame;
			if (frame.load_from_xml(node, _lc))
			{
				insert(frame);
				if (node->get_bool_attribute_or_from_child_presence(TXT("footStep")))
				{
					footStepAtTime = frame.atTime;
				}
			}
			else
			{
				result = false;
			}
		}
	}

	result &= events.load_from_xml(_node);

	isRelativeToMovementDirection = _node->get_bool_attribute(TXT("relativeToMovementDirection"), isRelativeToMovementDirection);

	return result;
}

bool Trajectory::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_AFTER_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::StartLevel)
	{
		return result;
	}

	// calculate max length if not provided
	if (length == 0.0f)
	{
		for_every(frame, frames)
		{
			length = max(length, frame->atTime);
		}

		if (length == 0.0f)
		{
			// empty trajectory?
			length = 1.0f;
		}
	}

	// provide at least one frame
	if (frames.is_empty())
	{
		frames.set_size(1);
		frames[0].atTime = 0.0f;
		frames[0].placement = Transform::identity;
	}

	// check if all frames belong to range
	if (frames.get_first().atTime < 0.0f ||
		frames.get_last().atTime > length)
	{
		error(TXT("frames beyond length"));
		result = false;
	}

	// make sure there are frames at the beginning and at the end
	if (frames.get_first().atTime > 0.0f)
	{
		TrajectoryFrame newFrame = frames.get_first();
		newFrame.atTime = 0.0f;
		insert(newFrame);
	}
	if (frames.get_last().atTime < length)
	{
		TrajectoryFrame newFrame = frames.get_last();
		newFrame.atTime = length;
		insert(newFrame);
	}

	Transform firstPlacement = frames.get_first().placement;
	Transform lastPlacement = frames.get_last().placement;

	// update ptTraj and "atPT"+"invToNextPT" to get proper trajPlacement and offset
	// calculate offsets too
	{
		TrajectoryFrame* prev = nullptr;

		Range yRange(firstPlacement.get_translation().y, lastPlacement.get_translation().y);
		for_every(frame, frames)
		{
			frame->atPT = frame->atTime / length;
			frame->invToNextPT = 0.0f;

			frame->ptTraj = yRange.get_pt_from_value(frame->placement.get_translation().y);

			if (prev)
			{
				float const diff = frame->atPT - prev->atPT;
				prev->invToNextPT = diff != 0.0f ? 1.0f / diff : 0.0f;
			}
			prev = frame;
		}

		// after we have all frames done, calculate offsets
		for_every(frame, frames)
		{
			// offset
			float ptTraj = transform_pt_to_pt_traj(frame->atPT);
			Transform trajPlacement = Transform::lerp(ptTraj, firstPlacement, lastPlacement);

			frame->offset = trajPlacement.to_local(frame->placement);
			an_assert(abs(frame->offset.get_translation().y) < 0.01f, TXT("ptTraj should handle this properly!"));
		}
	}

	if (footStepAtTime.is_set())
	{
		footStepAtPt = footStepAtTime.get() / length;
	}
	else
	{
		footStepAtPt = 1.0f;
	}

	// make sure all frames have gaitVars
	{
		Array<Name> gaitVarNames;
		// gather gait var names first
		for_every(frame, frames)
		{
			for_every(gaitVar, frame->gaitVars)
			{
				gaitVarNames.push_back_unique(gaitVar->varID);
			}
		}

		// go through all frames and for each frame, for each gait var check if it has previous and next
		for_every(frame, frames)
		{
			for_every(gaitVarName, gaitVarNames)
			{
				if (!frame->has_gait_var(*gaitVarName))
				{
					// find prev and find next, but because it is possible to have only one value, next might be before, prev might be later, that's why we have two loops
					TrajectoryFrame const * prev = nullptr;
					TrajectoryFrame const * next = nullptr;
					CustomGaitVar const * prevGaitVar = nullptr;
					CustomGaitVar const * nextGaitVar = nullptr;
					// prev
					for_every_reverse(checkFrame, frames)
					{
						if (CustomGaitVar const * found = checkFrame->get_gait_var(*gaitVarName))
						{
							prev = checkFrame;
							prevGaitVar = found;
							if (checkFrame->atPT < frame->atPT)
							{
								break;
							}
						}
					}
					// next
					for_every(checkFrame, frames)
					{
						if (CustomGaitVar const * found = checkFrame->get_gait_var(*gaitVarName))
						{
							next = checkFrame;
							nextGaitVar = found;
							if (checkFrame->atPT > frame->atPT)
							{
								break;
							}
						}
					}
					// interpolate
					float t = clamp(Range(prev->atPT, next->atPT).get_pt_from_value(frame->atPT), 0.0f, 1.0f);
					frame->insert_or_update(CustomGaitVar(*gaitVarName, prevGaitVar->value * (1.0f - t) + nextGaitVar->value * t, nullptr));
				}
			}
		}
	}


	return result;
}

int Trajectory::find_frame_index(float _atPT) const
{
	int index = 0;
	for_every(frame, frames)
	{
		if (_atPT <= frame->atPT)
		{
			break;
		}
		++index;
	}
	return max(0, index - 1);
}

void Trajectory::insert(TrajectoryFrame const & _frame)
{
	int index = 0;
	for_every(frame, frames)
	{
		if (_frame.atTime <= frame->atTime)
		{
			break;
		}
		++index;
	}
	frames.insert_at(index, _frame);
}

float Trajectory::transform_pt_to_pt_traj(float _atPT) const
{
	int index = find_frame_index(_atPT);

	TrajectoryFrame const * curr = &frames[min(index, frames.get_size() - 1)];
	TrajectoryFrame const * next = &frames[min(index + 1, frames.get_size() - 1)];

	float const t = clamp((_atPT - curr->atPT) * curr->invToNextPT, 0.0f, 1.0f);

	return curr->ptTraj * (1.0f - t) + next->ptTraj * t;
}

Transform Trajectory::calculate_offset_placement_at_pt(float _atPT, float _forTrajectoryObjectSize, Rotator3 const & _rotateBy) const
{
	int index = find_frame_index(_atPT);

	TrajectoryFrame const * curr = &frames[min(index, frames.get_size() - 1)];
	TrajectoryFrame const * next = &frames[min(index + 1, frames.get_size() - 1)];

	float const t = clamp((_atPT - curr->atPT) * curr->invToNextPT, 0.0f, 1.0f);

	Transform offset = Transform::lerp(t, curr->offset, next->offset);
	offset.set_translation(offset.get_translation() * _forTrajectoryObjectSize / trajectoryObjectSize);

	if (! _rotateBy.is_zero())
	{
		// offset only translation, keep orientation as it is
		offset.set_translation(_rotateBy.to_quat().rotate(offset.get_translation()));
	}

	return offset;
}

void Trajectory::calculate_and_add_gait_vars(float _atPT, Array<CustomGaitVar> & _gaitVars, Map<Name, Name> const & _translateNames) const
{
	int index = find_frame_index(_atPT);

	TrajectoryFrame const * curr = &frames[min(index, frames.get_size() - 1)];
	TrajectoryFrame const * next = &frames[min(index + 1, frames.get_size() - 1)];

	float const t = clamp((_atPT - curr->atPT) * curr->invToNextPT, 0.0f, 1.0f);
	
	an_assert(curr->gaitVars.get_size() == next->gaitVars.get_size());
	CustomGaitVar const* nextGaitVar = next->gaitVars.begin();
	for_every(currGaitVar, curr->gaitVars)
	{
		an_assert(currGaitVar->varID == nextGaitVar->varID);
		Name storeAs = currGaitVar->varID;
		if (Name const * found = _translateNames.get_existing(currGaitVar->varID))
		{
			storeAs = *found;
		}
		bool found = false;
		for_every(gaitVar, _gaitVars)
		{
			if (gaitVar->varID == currGaitVar->varID)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			_gaitVars.push_back(CustomGaitVar(storeAs, currGaitVar->value * (1.0f - t) + nextGaitVar->value * t, this));
		}
		++nextGaitVar;
	}
}

//

TrajectoryTrackElement::TrajectoryTrackElement()
: startsAt(0.0f)
{
}

bool TrajectoryTrackElement::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	startsAt = _node->get_float_attribute(TXT("at"), startsAt);
	length = _node->get_float_attribute(TXT("length"), length);
	if (length == 0.0f)
	{
		error_loading_xml(_node, TXT("no length provided for trajectory"));
		result = false;
	}

	useTrajectory = _node->get_name_attribute(TXT("useTrajectory"), useTrajectory);
	if (has_own_trajectory())
	{
		// load own
		trajectory = new Trajectory();
		result = trajectory->load_from_xml(_node, _lc);
	}

	return result;
}

bool TrajectoryTrackElement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (has_own_trajectory())
	{
		result &= trajectory->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

Trajectory* TrajectoryTrackElement::find_trajectory(Lib const * _lib) const
{
	if (has_own_trajectory())
	{
		return trajectory.get();
	}
	if (_lib)
	{
		return _lib->find_trajectory(useTrajectory);
	}
	error(TXT("couldn't find trajectory \"%S\" or trajectory not defined"), useTrajectory.to_char());
	return nullptr;
}

//

TrajectoryTrack::TrajectoryTrack()
: length(0.0f)
{
}

bool TrajectoryTrack::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for gait"));
		result = false;
	}

	length = _node->get_float_attribute(TXT("length"), length);
	if (length == 0.0f)
	{
		error_loading_xml(_node, TXT("no length provided for trajectory"));
		result = false;
	}

	for_every(node, _node->children_named(TXT("trajectory")))
	{
		TrajectoryTrackElement element;
		if (element.load_from_xml(node, _lc))
		{
			elements.push_back(element);
		}
	}

	return result;
}

bool TrajectoryTrack::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(element, elements)
	{
		result &= element->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

//

GaitLegMovement::GaitLegMovement()
: elementIndex(NONE)
, startsAt(0.0f)
, endsAt(0.0f)
, endPlacementAt(0.0f)
, gaitLength(0.0f)
, trajectory(nullptr)
{}

float GaitLegMovement::calculate_offset_to_contain(float _time) const
{
	float offset = 0.0f;
	if (gaitLength != 0.0f)
	{
		// try to have _time between startsAt and endsAt
		// if not possible just try to have _time before endsAt
		while (_time < startsAt + offset)
		{
			offset -= gaitLength;
		}
		while (_time > endsAt + offset)
		{
			offset += gaitLength;
		}
	}
	else
	{
		offset = _time - startsAt;
	}
	return offset;
}

void GaitLegMovement::apply_offset(float _offset)
{
	startsAt += _offset;
	endsAt += _offset;
	endPlacementAt += _offset;
}

void GaitLegMovement::adjust_to_contain(float _time)
{
	apply_offset(calculate_offset_to_contain(_time));
}

float GaitLegMovement::calculate_clamped_prop_through(float _time) const
{
	return endsAt > startsAt ? clamp((_time - startsAt) / (endsAt - startsAt), 0.0f, 1.0f) : 1.0f;
}

//

GaitLeg::GaitLeg()
: offset(0.0f)
{
}

bool GaitLeg::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, float _gaitLength)
{
	bool result = true;

	offset = _node->get_float_attribute(TXT("offset"), offset);
	float offsetPT = _node->get_float_attribute(TXT("offsetPT"), offset / _gaitLength);
	offset = offsetPT * _gaitLength;
	
	useTrajectoryTrack = _node->get_name_attribute(TXT("useTrajectoryTrack"), useTrajectoryTrack);
	if (_node->has_attribute(TXT("useTrajectorySet")))
	{
		warn_loading_xml(_node, TXT("\"useTrajectorySet\" deprecated, use \"useTrajectoryTrack\""));
		useTrajectoryTrack = _node->get_name_attribute(TXT("useTrajectorySet"), useTrajectoryTrack);
	}
	if (has_own_trajectory_track())
	{
		// load own
		trajectoryTrack = new TrajectoryTrack();
		result = trajectoryTrack->load_from_xml(_node, _lc);
	}

	return result;
}

bool GaitLeg::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (has_own_trajectory_track())
	{
		result = trajectoryTrack->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool GaitLeg::bake(Lib const & _lib, Gait & _forGait)
{
	bool result = true;

	if (!trajectoryTrack.is_set())
	{
		trajectoryTrack = _lib.find_trajectory_track(useTrajectoryTrack);
	}

	if (!trajectoryTrack.is_set())
	{
		error(TXT("couldn't find trajectory track \"%S\" or trajectory track not defined"), useTrajectoryTrack.to_char());
		return false;
	}

	TrajectoryTrack const * ts = trajectoryTrack.get();
	an_assert(ts->get_length() != 0.0f);
	//float const gaitTrajectoryObjectSize = _forGait.get_trajectory_object_size();
	float const gaitLength = _forGait.get_length();
	float const convertTStoGait = gaitLength / ts->get_length();
	
	// get all trajectory elements and create leg movements (use offset, convert to new time frame etc).
	int trajectoryTrackElementIndex = 0;
	for_every(trajectoryTrackElement, ts->get_elements())
	{
		Trajectory * t = trajectoryTrackElement->find_trajectory(&_lib);
		if (!t)
		{
			error(TXT("couldn't find trajectory or trajectory not defined"));
			return false;
		}
		float tAt = mod(convertTStoGait * trajectoryTrackElement->get_starts_at() + offset, gaitLength);
		float tLength = convertTStoGait * trajectoryTrackElement->get_length();

		GaitLegMovement legMovement;
		legMovement.elementIndex = trajectoryTrackElementIndex;
		legMovement.startsAt = tAt;
		legMovement.endsAt = tAt + tLength;
		legMovement.trajectory = t;
		insert(legMovement);

		if (tAt + tLength > gaitLength)
		{
			legMovement.startsAt -= gaitLength;
			legMovement.endsAt -= gaitLength;
			insert(legMovement);
		}

		++trajectoryTrackElementIndex;
	}

	// find placement times
	int movementsNum = movements.get_size();
	for (int index = 0; index < movementsNum; ++index)
	{
		GaitLegMovement & movement = movements[index];
		movement.gaitLength = gaitLength;
		// find next
		// if we're over to boundary, jump by 2
		// if we're only one, this won't matter much
		int jumpToNext = movement.endsAt > gaitLength ? 2 : 1;
		int next = mod(index + jumpToNext, movementsNum);

		float nextStartsAt = movement.endsAt + mod(movements[next].startsAt - movement.endsAt, gaitLength);

		float const toNextCoef = 0.55f;
		movement.endPlacementAt = movement.endsAt * (1.0f - toNextCoef) + toNextCoef * nextStartsAt;

		an_assert(movement.elementIndex != NONE);
		an_assert(movement.endsAt >= movement.startsAt);
		an_assert(movement.endPlacementAt >= movement.endsAt);
	}
	
	return result;
}

void GaitLeg::insert(GaitLegMovement const & _movement)
{
	int index = 0;
	for_every(movement, movements)
	{
		if (_movement.startsAt > movement->startsAt)
		{
			break;
		}
		++index;
	}
	movements.insert_at(index, _movement);
}

//

Gait::Gait()
{
}

bool Gait::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name provided for gait"));
		result = false;
	}

	length = _node->get_float_attribute(TXT("length"), length);
	if (length <= 0.0f)
	{
		error_loading_xml(_node, TXT("no length provided for gait (or invalid)"));
		result = false;
	}

	syncTags.load_from_xml(_node, TXT("syncTags"));

	allowAdjustments.load_from_xml(_node, TXT("allowAdjustments"));

	trajectoryObjectSize = _node->get_float_attribute(TXT("trajectoryObjectSize"), trajectoryObjectSize);

	if (IO::XML::Node const * node = _node->first_child_named(TXT("linearSpeedAffectingPlaybackSpeed")))
	{
		result &= playbackSpeedRange.load_from_xml(node, TXT("playbackSpeedRange"));
		speedXYForNormalPlaybackSpeed = node->get_float_attribute(TXT("speedXYForNormalPlaybackSpeed"), speedXYForNormalPlaybackSpeed);
		if (IO::XML::Attribute const * attr = node->get_attribute(TXT("speedsXYForPlaybackSpeedRange")))
		{
			speedsXYForPlaybackSpeedRange.set(Range(1.0f, 1.0f));
			result &= speedsXYForPlaybackSpeedRange.access().load_from_string(attr->get_as_string());
		}
	}

	if (IO::XML::Node const * node = _node->first_child_named(TXT("turnSpeedAffectingPlaybackSpeed")))
	{
		useTurnsSpeedsYawForPlaybackSpeed = true;
		result &= turnSpeedsYawForPlaybackSpeed_turnSpeedRange.load_from_xml(node, TXT("turnSpeedRange"));
		result &= turnSpeedsYawForPlaybackSpeed_playbackSpeedRange.load_from_xml(node, TXT("playbackSpeedRange"));
	}
	else
	{
		useTurnsSpeedsYawForPlaybackSpeed = false;
	}

	if (length > 0.0f)
	{
		for_every(node, _node->children_named(TXT("leg")))
		{
			GaitLeg leg;
			if (leg.load_from_xml(node, _lc, length))
			{
				legs.push_back(leg);
			}
			else
			{
				result = false;
			}
		}
	}

	result &= events.load_from_xml(_node);

	return result;
}

bool Gait::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(leg, legs)
	{
		result &= leg->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool Gait::bake(Lib const & _lib)
{
	bool result = true;

	for_every(leg, legs)
	{
		result &= leg->bake(_lib, *this);
	}

	return result;
}

GaitLegMovement const * Gait::find_movement(int _legIndex, float _atTime, float _ptBeforeEnd, float _timeBeforeEnd) const
{
	if (legs.is_index_valid(_legIndex)) // allow no legs info
	{
		for_every(movement, legs[_legIndex].get_movements())
		{
			if (_atTime >= movement->startsAt &&
				_atTime < movement->endsAt - min(_timeBeforeEnd, (movement->endsAt - movement->startsAt) * _ptBeforeEnd))
			{
				return movement;
			}
		}
	}

	return nullptr;
}

//

REGISTER_FOR_FAST_CAST(Lib);

bool Lib::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	trajectoryObjectSize = _node->get_float_attribute(TXT("trajectoryObjectSize"), trajectoryObjectSize);
	if (trajectoryObjectSize == 0.0f)
	{
		error_loading_xml(_node, TXT("invalid trajectoryObjectSize for walker lib"));
		return false;
	}

	for_every(node, _node->children_named(TXT("gait")))
	{
		Gait* gait = new Gait();
		gait->trajectoryObjectSize = trajectoryObjectSize;
		gait->add_ref();
		if (gait->load_from_xml(node, _lc))
		{
			gaits[gait->get_name()] = gait;
		}
		gait->release_ref();
	}

	for_every(node, _node->children_named(TXT("trajectory")))
	{
		Trajectory* trajectory = new Trajectory();
		trajectory->trajectoryObjectSize = trajectoryObjectSize;
		trajectory->add_ref();
		if (trajectory->load_from_xml(node, _lc))
		{
			trajectories[trajectory->get_name()] = trajectory;
		}
		trajectory->release_ref();
	}

	for_every(node, _node->children_named(TXT("trajectorySet")))
	{
		error_loading_xml(node, TXT("\"trajectorySet\" depracted, rename to \"trajectoryTrack\", will not be loaded now"));
		result = false;
	}

	for_every(node, _node->children_named(TXT("trajectoryTrack")))
	{
		TrajectoryTrack* trajectoryTrack = new TrajectoryTrack();
		trajectoryTrack->add_ref();
		if (trajectoryTrack->load_from_xml(node, _lc))
		{
			trajectoryTracks[trajectoryTrack->get_name()] = trajectoryTrack;
		}
		trajectoryTrack->release_ref();
	}

	for_every(node, _node->children_named(TXT("include")))
	{
		LibraryName toInclude;
		if (toInclude.load_from_xml(node, TXT("walkerLib"), _lc))
		{
			include.push_back(toInclude);
		}
		else
		{
			error_loading_xml(node, TXT("no include provided for lib"));
			result = false;
		}
	}

	return result;
}

bool Lib::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(gait, gaits)
	{
		result &= gait->get()->prepare_for_game(_library, _pfgContext);
	}

	for_every(trajectory, trajectories)
	{
		result &= trajectory->get()->prepare_for_game(_library, _pfgContext);
	}

	for_every(trajectoryTrack, trajectoryTracks)
	{
		result &= trajectoryTrack->get()->prepare_for_game(_library, _pfgContext);
	}

	IF_PREPARE_LEVEL(_pfgContext, LibraryPrepareLevel::Resolve)
	{
		sort_out_includes(_library);
	}

	return result;
}

void Lib::sort_out_includes(Library* _library)
{
	for_every_reverse(incl, include)
	{
		if (!alreadyIncluded.does_contain(*incl))
		{
			if (CustomLibraryStoredData * found = _library->get_custom_datas().find(*incl))
			{
				if (Lib * foundLib = fast_cast<Lib>(found->access_data()))
				{
					foundLib->sort_out_includes(_library);
					include_other(foundLib);
				}
			}
			alreadyIncluded.push_back(*incl);
		}
	}
}

void Lib::include_other(Lib const * _lib)
{
	for_every(gait, _lib->gaits)
	{
		if (! gaits.has_key(gait->get()->get_name()))
		{
			gaits[gait->get()->get_name()] = gait->get();
		}
	}

	for_every(trajectory, _lib->trajectories)
	{
		if (!trajectories.has_key(trajectory->get()->get_name()))
		{
			trajectories[trajectory->get()->get_name()] = trajectory->get();
		}
	}

	for_every(trajectoryTrack, _lib->trajectoryTracks)
	{
		if (!trajectoryTracks.has_key(trajectoryTrack->get()->get_name()))
		{
			trajectoryTracks[trajectoryTrack->get()->get_name()] = trajectoryTrack->get();
		}
	}
}

bool Lib::bake()
{
	bool result = true;
	for_every(gait, gaits)
	{
		result &= gait->get()->bake(*this);
	}
	return result;
}

Gait* Lib::find_gait(Name const & _name) const
{
	if (RefCountObjectPtr<Gait> const * foundPtr = gaits.get_existing(_name))
	{
		return foundPtr->get();
	}
	return nullptr;
}

Trajectory* Lib::find_trajectory(Name const & _name) const
{
	if (RefCountObjectPtr<Trajectory> const * foundPtr = trajectories.get_existing(_name))
	{
		return foundPtr->get();
	}
	return nullptr;
}

TrajectoryTrack* Lib::find_trajectory_track(Name const & _name) const
{
	if (RefCountObjectPtr<TrajectoryTrack> const * foundPtr = trajectoryTracks.get_existing(_name))
	{
		return foundPtr->get();
	}
	return nullptr;
}

//

InstanceContext::InstanceContext()
: prepared(false)
, owningAppearance(nullptr)
, instance(nullptr)
{
}

void InstanceContext::reset_for_new_frame()
{
	prepared = false;
	justActivated = false;

	poseLS = nullptr;
	collidingWithNonScenery = false;
	gravityPresenceTracesTouchSurroundings = false;
	relativeVelocityLinear.clear();
	relativeRequestedMovementDirection.clear();
	relativeVelocityDirection.clear();
	velocityOrientation.clear();
	turnSpeed.clear();
	speed.clear();
	speedXY.clear();
	currentGait.clear();
	gaitNameRequestedByController.clear();
}

void InstanceContext::prepare()
{
	prepared = true;
	if (relativeVelocityLinear.is_set())
	{
		speed = relativeVelocityLinear.get().length();
		speedXY = (relativeVelocityLinear.get() * Vector3(1.0f, 1.0f, 0.0)).length();
		if (speed.get() != 0.0f)
		{
			relativeVelocityDirection = Rotator3::get_yaw(relativeVelocityLinear.get());
		}
		else
		{
			relativeVelocityDirection.clear();
		}
	}
	else
	{
		speed.clear();
		speedXY.clear();
		relativeVelocityDirection.clear();
	}
	if (velocityOrientation.is_set())
	{
		turnSpeed = velocityOrientation.get().length();
	}
	else
	{
		turnSpeed.clear();
	}
	if (relativeVelocityDirection.is_set())
	{
		// make sure value is within -180 to 180
		relativeVelocityDirection = Rotator3::normalise_axis(relativeVelocityDirection.get());
	}
	if (relativeRequestedMovementDirection.is_set())
	{
		// make sure value is within -180 to 180
		relativeRequestedMovementDirection = Rotator3::normalise_axis(relativeRequestedMovementDirection.get());
	}
}

Transform InstanceContext::where_will_I_be_in_MS(float _time) const
{
	an_assert(instance);
	_time += instance? instance->get_prediction_offset() : 0.0f;
	// predict using current placement
	Transform placementOS = Transform::identity;

	// store velocity and turn speed
	Vector3 useRelativeVelocityLinear = relativeVelocityLinear.is_set() ? relativeVelocityLinear.get() : Vector3::zero;
	Rotator3 useVelocityOrientation = velocityOrientation.is_set() ? velocityOrientation.get() : Rotator3::zero;

	if (relativeRequestedMovementDirection.is_set())
	{
		float use = instance->get_setup()->whereIWillBe.useRelativeRequestedMovementDirection.get();
		if (use > 0.0f)
		{
			Vector3 reqMoveDir = Rotator3(0.0f, relativeRequestedMovementDirection.get(), 0.0f).get_forward() * useRelativeVelocityLinear.length();
			useRelativeVelocityLinear = lerp(use, useRelativeVelocityLinear, reqMoveDir);
		}
	}

	if (!useRelativeVelocityLinear.is_zero())
	{
		if (auto* c = get_owner()->get_owner()->get_collision())
		{
			Vector3 collisionGradient = c->get_gradient();

			if (!collisionGradient.is_zero())
			{
				// get into local / os
				collisionGradient = owningAppearance->get_os_to_ws_transform().vector_to_local(collisionGradient);

				Vector3 velocityDir = useRelativeVelocityLinear.normal();
				Vector3 collisionGradientDir = collisionGradient.normal();

				debug_filter(ac_walkers);
				debug_context(get_owner()->get_owner()->get_presence()->get_in_room());
				debug_subject(get_owner()->get_owner());
				debug_push_transform(get_owner()->get_owner()->get_presence()->get_placement());

				debug_draw_arrow(true, Colour::green, Vector3::zero, useRelativeVelocityLinear);
				debug_draw_arrow(true, Colour::red, Vector3::zero, collisionGradient);

				float dot = Vector3::dot(velocityDir, collisionGradientDir);
				float dotThreshold = -0.6f;
				debug_draw_text(true, Colour::red, Vector3::zero, Vector2::half, true, 0.4f, NP, TXT("vc: %.3f cg:%.3f"), dot, collisionGradient.length());
				if (dot < dotThreshold)
				{
					float dotStopThreshold = -0.95f;
					// at -1 we want to stop almost completely
					float velocityCoef = clamp((dot - dotStopThreshold) / (dotThreshold - dotStopThreshold), 0.0f, 1.0f);
					velocityCoef = lerp(velocityCoef, 0.1f, 1.0f);

					// but if collision gradient is really small, ignore this
					float collisionGradientLength = collisionGradient.length();
					float collisionGradientThreshold = 0.05f;
					float useFullVelocityCoef = clamp(1.0f - collisionGradientLength / collisionGradientThreshold, 0.0f, 1.0f);
					velocityCoef = lerp(useFullVelocityCoef, velocityCoef, 1.0f);

					// slow down so we do not appear walking into collision
					useRelativeVelocityLinear *= velocityCoef;
					debug_draw_arrow(true, Colour::orange, Vector3::zero + Vector3::zAxis * 0.01f, useRelativeVelocityLinear + Vector3::zAxis * 0.01f);
				}

				debug_pop_transform();
				debug_no_subject();
				debug_no_context();
				debug_no_filter();
			}
		}
	}

	// predict where will I be
	float timeLeft = _time;
	float const withTimeStep = 0.03f;
	while (timeLeft > 0.0f)
	{
		float const timeStep = min(timeLeft, withTimeStep);
		placementOS.set_translation(placementOS.get_translation() + placementOS.vector_to_world(useRelativeVelocityLinear) * timeStep);
		placementOS.set_orientation(placementOS.get_orientation().rotate_by((useVelocityOrientation * timeStep).to_quat()));
		timeLeft -= timeStep;
	}
	
	// we want result in MS, not OS
	Transform resultMS = owningAppearance->get_ms_to_os_transform().to_local(placementOS);

	// get from locomotion as it might be more precise
	if (auto* ai = get_owner()->get_owner()->get_ai())
	{
		if (auto* mind = ai->get_mind())
		{
			Room* room;
			Transform placementWS;
			Transform intoRoom;
			if (mind->access_locomotion().where_I_want_to_be_in(room, placementWS, _time, &intoRoom))
			{
				placementWS = intoRoom.to_world(placementWS); // get back to our room
				Transform placementMS = owningAppearance->get_ms_to_ws_transform().to_local(placementWS);

				resultMS = Transform::lerp(instance->get_setup()->whereIWillBe.useLocomotionWhereIWantToBe.get(), resultMS, placementMS);
			}
		}
	}


	return resultMS;
}

Transform InstanceContext::where_leg_will_be_in_MS(float _time, int _boneIndex, Transform const & _defaultPlacementMS) const
{
	todo_note(TXT("for now use default placementMS, in future use animations?"));
	return _defaultPlacementMS;
}

//

GaitOption GaitOption::invalid;

bool GaitOption::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	gait = _node->get_name_attribute(TXT("gait"), gait);
	reason = _node->get_name_attribute(TXT("reason"), reason);
	forceDropDown = _node->get_bool_attribute(TXT("forceDropDown"), forceDropDown);
	startAtPT = _node->get_float_attribute(TXT("startAtPT"), startAtPT);

	return result;
}

//

LockGait::LockGait()
{
}

LockGait::~LockGait()
{
}

bool LockGait::load_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;
	if (!_node)
	{
		return true;
	}

	currentGait.clear();
	requestedGait.clear();
	struct LoadUtils
	{
		static bool load_gait(IO::XML::Node const* _node, tchar const* _name, OUT_ Array<Name>& _gaits)
		{
			{
				Name v = _node->get_name_attribute(_name);
				if (v.is_valid())
				{
					_gaits.push_back_unique(v);
				}
			}
			for_every(c, _node->children_named(_name))
			{
				Name v = Name(_node->get_text());
				if (v.is_valid())
				{
					_gaits.push_back_unique(v);
				}
			}
			return true;
		}
	};

	result &= LoadUtils::load_gait(_node, TXT("currentGait"), currentGait);
	result &= LoadUtils::load_gait(_node, TXT("requestedGait"), requestedGait);

	if (_node->get_bool_attribute_or_from_child_presence(TXT("legsUp")))
	{
		legsUp = RangeInt(1, 10000);
	}
	legsUp.load_from_xml(_node, TXT("legsUp"));

	return result;
}

bool LockGait::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

bool LockGait::is_allowed(Name const& _currentGait, Name const& _requestedGait, Array<LockGait> const& _lockGaits, Walkers::Instance const& _walkerInstance, Walkers::InstanceContext const& _walkerInstanceContext)
{
	for_every(l, _lockGaits)
	{
		if ((l->currentGait.is_empty() || l->currentGait.does_contain(_currentGait)) &&
			(l->requestedGait.is_empty() || l->requestedGait.does_contain(_requestedGait)))
		{
			bool locked = true;
			if (!l->legsUp.is_empty())
			{
				if (! l->legsUp.does_contain(_walkerInstance.get_legs_up()))
				{
					locked = false;
				}
			}
			return !locked;
		}
	}
	return true;
}

//

ChooseGait::ChooseGait()
{
}

ChooseGait::~ChooseGait()
{
}

#define CHOOSE_GAIT_LOAD_RANGE(_what) \
	if (_node->has_attribute(TXT(#_what))) \
	{ \
		_what.set(Range::infinite); \
		result &= _what.access().load_from_xml(_node, TXT(#_what)); \
	}

bool ChooseGait::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	if (!_node)
	{
		return true;
	}
	CHOOSE_GAIT_LOAD_RANGE(speed);
	CHOOSE_GAIT_LOAD_RANGE(speedXY);
	CHOOSE_GAIT_LOAD_RANGE(turnSpeed);
	CHOOSE_GAIT_LOAD_RANGE(absTurnSpeed);
	CHOOSE_GAIT_LOAD_RANGE(turnSpeedYaw);
	CHOOSE_GAIT_LOAD_RANGE(absTurnSpeedYaw);
	CHOOSE_GAIT_LOAD_RANGE(turnSpeedPitch);
	CHOOSE_GAIT_LOAD_RANGE(absTurnSpeedPitch);
	CHOOSE_GAIT_LOAD_RANGE(turnSpeedRoll);
	CHOOSE_GAIT_LOAD_RANGE(absTurnSpeedRoll);
	CHOOSE_GAIT_LOAD_RANGE(relativeRequestedMovementDirection);
	CHOOSE_GAIT_LOAD_RANGE(preferredGravityAlignment);
	CHOOSE_GAIT_LOAD_RANGE(differenceBetweenRequestedMovementDirectionAndVelocityDirection);
	collidingWithNonScenery.load_from_xml(_node, TXT("collidingWithNonScenery"));
	gravityPresenceTracesTouchSurroundings.load_from_xml(_node, TXT("gravityPresenceTracesTouchSurroundings"));
	currentGait.load_from_xml(_node, TXT("currentGait"));
	currentGaitSet.load_from_xml(_node, TXT("currentGaitSet"));
	gaitNameRequestedByController.load_from_xml(_node, TXT("gaitNameRequestedByController"));
	ignoreSpeedWhenGaitNameRequestedByControllerSuccessful = _node->get_bool_attribute(TXT("ignoreSpeedWhenGaitNameRequestedByControllerSuccessful"), ignoreSpeedWhenGaitNameRequestedByControllerSuccessful);
	if (_node->has_attribute(TXT("gait")) || _node->has_attribute(TXT("reason")))
	{
		GaitOption go;
		if (go.load_from_xml(_node, _lc))
		{
			chooseGaits.push_back(go);
		}
	}
	for_every(oneOf, _node->children_named(TXT("oneOf")))
	{
		for_every(gait, oneOf->children_named(TXT("option")))
		{
			Name chooseGait = gait->get_name_attribute(TXT("gait"));
			if (chooseGait.is_valid())
			{
				GaitOption go;
				if (go.load_from_xml(gait, _lc))
				{
					chooseGaits.push_back(go);
				}
			}
		}
	}
	for_every(chooseNode, _node->children_named(TXT("choose")))
	{
		ChooseGait choose;
		if (choose.load_from_xml(chooseNode, _lc))
		{
			chooseOptions.push_back(choose);
		}
	}
	if (!result)
	{
		error_loading_xml(_node, TXT("error loading choose gait"));
	}
	return result;
}

#undef CHOOSE_GAIT_LOAD_RANGE

bool ChooseGait::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;
	for_every(chooseOption, chooseOptions)
	{
		result &= chooseOption->prepare_for_game(_library, _pfgContext);
	}
	return result;
}

#define CHOOSE_GAIT_CHECK(_variable, _checkSet, _getValue) \
	if (_variable.is_set() && (_checkSet)) \
	{ \
		if (!_variable.get().does_contain((_getValue))) \
		{ \
			return GaitOption::invalid; \
		} \
	}

#define CHOOSE_GAIT_CHECK_SIMPLE(_variable, _get_func) \
	CHOOSE_GAIT_CHECK(_variable, _context._get_func().is_set(), _context._get_func().get())

GaitOption const & ChooseGait::run_for(InstanceContext const & _context) const
{
	an_assert(_context.is_prepared(), TXT("please .prepare() context"));
	bool checkSpeed = true;
	if (collidingWithNonScenery.is_set())
	{
		if (collidingWithNonScenery.get() ^ _context.is_colliding_with_non_scenery())
		{
			return GaitOption::invalid;
		}
	}
	if (gravityPresenceTracesTouchSurroundings.is_set())
	{
		if (gravityPresenceTracesTouchSurroundings.get() ^ _context.do_gravity_presence_traces_touch_surroundings())
		{
			return GaitOption::invalid;
		}
	}
	if (gaitNameRequestedByController.is_set() && _context.get_gait_name_requested_by_controller().is_set())
	{
		if (gaitNameRequestedByController.get() != _context.get_gait_name_requested_by_controller().get())
		{
			return GaitOption::invalid;
		}
		else if (ignoreSpeedWhenGaitNameRequestedByControllerSuccessful)
		{
			// no need to check speed, because we already checked gait
			checkSpeed = false;
		}
	}
	if (checkSpeed)
	{
		CHOOSE_GAIT_CHECK_SIMPLE(speed, get_speed);
		CHOOSE_GAIT_CHECK_SIMPLE(speedXY, get_speed_XY);
	}
	CHOOSE_GAIT_CHECK_SIMPLE(turnSpeed, get_turn_speed);
	CHOOSE_GAIT_CHECK(turnSpeedYaw, _context.get_velocity_orientation().is_set(), _context.get_velocity_orientation().get().yaw);
	CHOOSE_GAIT_CHECK(turnSpeedPitch, _context.get_velocity_orientation().is_set(), _context.get_velocity_orientation().get().pitch);
	CHOOSE_GAIT_CHECK(turnSpeedRoll, _context.get_velocity_orientation().is_set(), _context.get_velocity_orientation().get().roll);
	CHOOSE_GAIT_CHECK(absTurnSpeedYaw, _context.get_velocity_orientation().is_set(), abs(_context.get_velocity_orientation().get().yaw));
	CHOOSE_GAIT_CHECK(absTurnSpeedPitch, _context.get_velocity_orientation().is_set(), abs(_context.get_velocity_orientation().get().pitch));
	CHOOSE_GAIT_CHECK(absTurnSpeedRoll, _context.get_velocity_orientation().is_set(), abs(_context.get_velocity_orientation().get().roll));
	if (preferredGravityAlignment.is_set() && (_context.get_preferred_gravity_alignment().is_set()))
	{
		Range const& rmd = preferredGravityAlignment.get();
		float actualPGA = _context.get_preferred_gravity_alignment().get();
		if (!rmd.does_contain(actualPGA))
		{
			return GaitOption::invalid;
		}
	}
	if (relativeRequestedMovementDirection.is_set() && (_context.get_relative_requested_movement_direction().is_set()))
	{
		Range const & rmd = relativeRequestedMovementDirection.get();
		float contextrmd = _context.get_relative_requested_movement_direction().get();
		if (rmd.min > -1000.0f && rmd.max < 1000.0f)
		{
			// check -360 and +360 so we cover such cases as: 90, 270
			if (!rmd.does_contain(contextrmd) &&
				!rmd.does_contain(contextrmd - 360.0f) &&
				!rmd.does_contain(contextrmd + 360.0f))
			{
				return GaitOption::invalid;
			}
		}
		else
		{
			if (!rmd.does_contain(contextrmd))
			{
				return GaitOption::invalid;
			}
		}
	}
	if (differenceBetweenRequestedMovementDirectionAndVelocityDirection.is_set() &&
		_context.get_relative_requested_movement_direction().is_set() &&
		_context.get_relative_velocity_direction().is_set())
	{
		float difference = _context.get_relative_requested_movement_direction().get() - _context.get_relative_velocity_direction().get();
		difference = Rotator3::normalise_axis(difference);
		difference = abs(difference);
		if (!differenceBetweenRequestedMovementDirectionAndVelocityDirection.get().does_contain(difference))
		{
			return GaitOption::invalid;
		}
	}
	if (currentGait.is_set() && _context.get_current_gait().is_set())
	{
		if (currentGait.get() != _context.get_current_gait().get())
		{
			return GaitOption::invalid;
		}
	}
	if (currentGaitSet.is_set() && _context.get_current_gait().is_set())
	{
		if (currentGaitSet.get() ^ _context.get_current_gait().get().is_valid())
		{
			return GaitOption::invalid;
		}
	}
	for_every(chooseOption, chooseOptions)
	{
		GaitOption const & chosen = chooseOption->run_for(_context);
		if (chosen.is_valid())
		{
			return chosen;
		}
	}

	if (chooseGaits.is_empty())
	{
		return GaitOption::invalid;
	}
	else if (chooseGaits.get_size() == 1)
	{
		return chooseGaits.get_first();
	}
	else
	{
		// try to keep the same gait
		if (_context.get_current_gait().is_set())
		{
			Name const & currentlyChosenGait = _context.get_current_gait().get();
			for_every(chooseGait, chooseGaits)
			{
				if (chooseGait->gait == currentlyChosenGait)
				{
					return *chooseGait;
				}
			}
		}

		return chooseGaits[_context.access_random_generator().get_int(chooseGaits.get_size())];
	}
}

#undef CHOOSE_GAIT_CHECK_SIMPLE
#undef CHOOSE_GAIT_CHECK

//

ProvideAllowedGait::ProvideAllowedGait()
{
}

ProvideAllowedGait::~ProvideAllowedGait()
{
}

bool ProvideAllowedGait::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (_node)
	{
		for_every(node, _node->all_children())
		{
			if (node->get_name() == TXT("provide"))
			{
				Element element;
				element.gait = node->get_name_attribute(TXT("gait"));
				element.provideAs = node->get_name_attribute(TXT("as"));
				error_loading_xml_on_assert(element.gait.is_valid(), result, node, TXT("no gait given"));
				error_loading_xml_on_assert(element.provideAs.is_valid(), result, node, TXT("no provide as given"));
				elements.push_back(element);
			}
			else if (node->get_name() == TXT("provideOthers"))
			{
				Element element;
				element.provideAs = node->get_name_attribute(TXT("as"));
				error_loading_xml_on_assert(element.provideAs.is_valid(), result, node, TXT("no provide as given"));
				elements.push_back(element);
			}
			else if (node->get_name() == TXT("provideOtherAsTheyAre"))
			{
				Element element;
				elements.push_back(element);
			}
			else
			{
				error_loading_xml(node, TXT("not recognised provide allowed gait node"));
				result = false;
			}
		}
	}

	return result;
}

bool ProvideAllowedGait::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	return true;
}

Name const & ProvideAllowedGait::get_provided(Name const & _gait) const
{
	for_every(e, elements)
	{
		if (e->gait.is_valid() && e->gait == _gait)
		{
			return e->provideAs;
		}
		if (! e->gait.is_valid())
		{
			if (e->provideAs.is_valid())
			{
				return e->provideAs;
			}
			else
			{
				return _gait;
			}
		}
	}

	return Name::invalid();
}

//

bool GaitVar::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	varID = _node->get_name_attribute(TXT("name"), varID);
	error_loading_xml_on_assert(varID.is_valid(), result, _node, TXT("name for gait var not provided"));

	defaultValue = _node->get_float_attribute(TXT("defaultValue"), defaultValue);
	for_every(node, _node->children_named(TXT("for")))
	{
		ForGait fg;
		fg.gait = node->get_name_attribute(TXT("gait"), fg.gait);
		error_loading_xml_on_assert(fg.gait.is_valid(), result, node, TXT("gait's name for \"forGait\" not provided"));
		fg.value = node->get_float_attribute(TXT("value"), fg.value);
		if (forGaits.get_space_left() > 0)
		{
			forGaits.push_back(fg);
		}
		else
		{
			error_loading_xml(node, TXT("too many for-gaits"));
		}
	}

	return true;
}

float GaitVar::get_value(Name const & _gait) const
{
	for_every(fg, forGaits)
	{
		if (fg->gait == _gait)
		{
			return fg->value;
		}
	}
	return defaultValue;
}

//

GaitPlayback::GaitPlayback()
: atTime(0.0f)
, prevTime(0.0f)
, playbackSpeed(1.0f)
, gait(nullptr)
, syncTo(nullptr)
{
}

void GaitPlayback::switch_to(Gait const * _newGait, GaitOption const * _useOption)
{
	float pt = 0.0f; 
	float ptPrev = 0.0f;
	// check if we can sync between gaits
	bool syncGaits = gait && _newGait && gait->get_sync_tags().does_match_any_from(_newGait->get_sync_tags());
	if (_useOption && !syncGaits)
	{
		pt = ptPrev = _useOption->startAtPT;
	}
	else
	{
		if (gait)
		{
			float invGaitLength = 1.0f / gait->get_length();
			pt = atTime * invGaitLength;
			ptPrev = prevTime * invGaitLength;
		}
	}

	gait = _newGait;

	atTime = 0.0f;
	prevTime = 0.0f;
	playbackSpeed = 1.0f;
	if (gait)
	{
		float gaitLength = gait->get_length();
		atTime = pt * gaitLength;
		prevTime = ptPrev * gaitLength;
	}
}

void GaitPlayback::sync_to(GaitPlayback const * _other)
{
	syncTo = _other;
	if (syncTo)
	{
		atTime = syncTo->atTime;
		prevTime = syncTo->prevTime;
		playbackSpeed = syncTo->playbackSpeed;
		gait = syncTo->gait;
	}
}

void GaitPlayback::advance(float _deltaTime)
{
	prevTime = atTime;
	if (syncTo == nullptr || gait != syncTo->gait)
	{
		if (gait)
		{
			atTime += _deltaTime * playbackSpeed;
			float const gaitLength = gait->get_length();
			atTime = mod(atTime, gaitLength);
			while (prevTime > atTime)
			{
				prevTime -= gaitLength;
			}
		}
		else
		{
			atTime += _deltaTime;
		}
	}
	else
	{
		atTime = syncTo->atTime;
		prevTime = syncTo->prevTime;
		playbackSpeed = syncTo->playbackSpeed;
	}
}

//

bool LegSetup::ForGait::load_from_xml(IO::XML::Node const * _node, IO::XML::Node const* _mainNode, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return result;
	}

	placementMS.load_from_xml_child_node(_node, TXT("placementMS"));
	placementMS.load_from_xml_child_node(_node, TXT("placement"));
	placementSocket = _node->get_name_attribute_or_from_child(TXT("placementSocket"), placementSocket);
	placementSocketStopped = _node->get_name_attribute_or_from_child(TXT("placementSocketStopped"), placementSocketStopped);

	result &= autoPlacement.load_from_xml(_node->first_child_named(TXT("autoPlacementMS")), _lc);
	result &= autoPlacement.load_from_xml(_node->first_child_named(TXT("autoPlacement")), _lc);

	disallowRedoLegMovementForTime = _mainNode->get_float_attribute(TXT("disallowRedoLegMovementForTime"), disallowRedoLegMovementForTime);
	disallowRedoLegMovementForTime = _node->get_float_attribute(TXT("disallowRedoLegMovementForTime"), disallowRedoLegMovementForTime);
	maxDifferenceOfOwnerToRedoLegMovement = _mainNode->get_float_attribute(TXT("maxDifferenceOfOwnerToRedoLegMovement"), maxDifferenceOfOwnerToRedoLegMovement);
	maxDifferenceOfOwnerToRedoLegMovement = _node->get_float_attribute(TXT("maxDifferenceOfOwnerToRedoLegMovement"), maxDifferenceOfOwnerToRedoLegMovement);
	maxDifferenceOfLegToRedoLegMovement = _mainNode->get_float_attribute(TXT("maxDifferenceOfLegToRedoLegMovement"), maxDifferenceOfLegToRedoLegMovement);
	maxDifferenceOfLegToRedoLegMovement = _node->get_float_attribute(TXT("maxDifferenceOfLegToRedoLegMovement"), maxDifferenceOfLegToRedoLegMovement);
	allowSwitchingLegMovementPT = _mainNode->get_float_attribute(TXT("allowSwitchingLegMovementPT"), allowSwitchingLegMovementPT);
	allowSwitchingLegMovementPT = _node->get_float_attribute(TXT("allowSwitchingLegMovementPT"), allowSwitchingLegMovementPT);
	if (!dropDownTime.load_from_xml(_mainNode, TXT("dropDownTime")) ||
		!dropDownTime.load_from_xml(_node, TXT("dropDownTime")))
	{
		error_loading_xml(_node, TXT("error loading dropDownTime"));
		result = false;
	}
	dropDownDistanceForMaxTime = _mainNode->get_float_attribute(TXT("dropDownDistanceForMaxTime"), dropDownDistanceForMaxTime);
	dropDownDistanceForMaxTime = _node->get_float_attribute(TXT("dropDownDistanceForMaxTime"), dropDownDistanceForMaxTime);
	result &= fallingTime.load_from_xml(_node, TXT("fallingTime"));
	allowAdjustments.load_from_xml(_node, TXT("allowAdjustments"));
	adjustmentRequiredDistance.load_from_xml(_node, TXT("adjustmentRequiredDistance"));
	adjustmentRequiredVerticalDistance.load_from_xml(_node, TXT("adjustmentRequiredVerticalDistance"));
	adjustmentRequiredForwardAngle.load_from_xml(_node, TXT("adjustmentRequiredForwardAngle"));
	adjustmentTime.load_from_xml(_node, TXT("adjustmentTime"));
	disallowAdjustmentMovementForTime.load_from_xml(_node, TXT("disallowAdjustmentMovementForTime"));
	disallowAdjustmentMovementWhenCollidingForTime.load_from_xml(_node, TXT("disallowAdjustmentMovementWhenCollidingForTime"));

	for_every(node, _node->children_named(TXT("var")))
	{
		CustomGaitVar var;
		if (var.load_from_xml(node, _lc))
		{
			bool found = false;
			for_every(gaitVar, gaitVars)
			{
				if (gaitVar->varID == var.varID)
				{
					*gaitVar = var;
					found = true;
					break;
				}
			}
			if (!found)
			{
				gaitVars.push_back(var);
			}
		}
		else
		{
			result = false;
		}
	}

	return result;
}

void LegSetup::ForGait::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	autoPlacement.apply_mesh_gen_params(_context);
	adjustmentRequiredDistance.fill_value_with(_context);
	adjustmentRequiredVerticalDistance.fill_value_with(_context);
	adjustmentRequiredForwardAngle.fill_value_with(_context);
}

//

LegSetup::ForGait::Instance::Instance(ForGait const * _gait, InstanceContext const & _instanceContext)
: gait(_gait)
, placementMS(_gait->placementMS)
, placementSocket(_gait->placementSocket)
, placementSocketStopped(_gait->placementSocketStopped)
{
	_gait->autoPlacement.update_placement_MS(REF_ placementMS, _instanceContext);
}

Transform LegSetup::ForGait::Instance::calculate_placement_MS(InstanceContext const & _instanceContext) const
{
	Transform result = placementMS;
	if (placementSocketStopped.is_valid() &&
		(! _instanceContext.get_speed_XY().is_set() ||
		 _instanceContext.get_speed_XY().get() < 0.1f))
	{
		result = _instanceContext.get_owner()->calculate_socket_ms(placementSocketStopped);
	}
	else if (placementSocket.is_valid())
	{
		result = _instanceContext.get_owner()->calculate_socket_ms(placementSocket);
	}
	return result;
}
//

LegSetup::ForGait::AutoPlacement::AutoPlacement()
{
	at.set_if_not_set(0.0f);
}

bool LegSetup::ForGait::AutoPlacement::update_placement_MS(REF_ Transform & _placementMS, InstanceContext const & _instanceContext) const
{
	if (!bone0.is_set() || !bone0.get().is_valid() || !bone1.is_set() || !bone1.get().is_valid())
	{
		return false;
	}

	if (auto const * skeleton = _instanceContext.get_owner()->get_skeleton())
	{
		if (auto const * skel = skeleton->get_skeleton())
		{
			int bone0idx = skel->find_bone_index(bone0.get());
			int bone1idx = skel->find_bone_index(bone1.get());

			if (bone0idx == NONE)
			{
				error(TXT("bone \"%S\" not found"), bone0.get().to_char());
				return false;
			}
			if (bone1idx == NONE)
			{
				error(TXT("bone \"%S\" not found"), bone1.get().to_char());
				return false;
			}
			_placementMS = Transform::lerp(at.get(), skel->get_bones_default_placement_MS()[bone0idx], skel->get_bones_default_placement_MS()[bone1idx]);
			_placementMS.set_translation(_placementMS.get_translation() * Vector3(1.0f, 1.0f, 0.0f)); // drop onto ground
			if (location.is_set())
			{
				_placementMS.set_translation(location.get());
			}
			if (orientation.is_set())
			{
				_placementMS.set_orientation(orientation.get().to_quat());
			}
			return true;
		}
		else
		{
			error(TXT("no skeleton associated with skeleton"));
		}
	}
	else
	{
		error(TXT("no skeleton for walker"));
	}

	return false;
}

bool LegSetup::ForGait::AutoPlacement::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	if (!_node)
	{
		return true;
	}

	bone0.load_from_xml(_node, TXT("bone0"));
	bone1.load_from_xml(_node, TXT("bone1"));
	at.load_from_xml(_node, TXT("at"));

	// optionals
	location.load_from_xml_child_node(_node, TXT("location"));
	orientation.load_from_xml_child_node(_node, TXT("orientation"));

	return result;
}

void LegSetup::ForGait::AutoPlacement::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	bone0.fill_value_with(_context);
	bone1.fill_value_with(_context);
	at.fill_value_with(_context);
	location.fill_value_with(_context);
	orientation.fill_value_with(_context);
}

//

bool LegSetup::load_from_xml(IO::XML::Node const * _node, IO::XML::Node const* _mainNode, LibraryLoadingContext & _lc)
{
	bool result = true;

	result &= defaultGait.load_from_xml(_node->first_child_named(TXT("defaultGait")), _mainNode, _lc);
	for_every(node, _node->children_named(TXT("for")))
	{
		if (IO::XML::Attribute const * attr = node->get_attribute(TXT("gait")))
		{
			Name forGait = attr->get_as_name();
			if (forGait.is_valid())
			{
				LegSetup::ForGait gait = defaultGait; // use default as base
				if (gait.load_from_xml(node, _mainNode, _lc))
				{
					perGait[forGait] = gait;
				}
			}
		}
	}

	result &= boneForTraceStart.load_from_xml(_node, TXT("boneForTraceStart"));
	result &= boneForLengthEnd.load_from_xml(_node, TXT("boneForLengthEnd"));
	legLength = _node->get_float_attribute(TXT("legLength"), legLength);
	collisionTraceMinAlt = _node->get_float_attribute(TXT("collisionTraceMinAlt"), collisionTraceMinAlt);
	result &= footBone.load_from_xml(_node, TXT("footBone"));
	result &= footBaseSocket.load_from_xml(_node, TXT("footBaseSocket"));

	result &= soundSocket.load_from_xml(_node, TXT("soundSocket"));

	useDropDownTrajectory = _node->get_name_attribute(TXT("dropDownTrajectory"), useDropDownTrajectory);
	if (has_own_drop_down_trajectory())
	{
		if (IO::XML::Node const * node = _node->first_child_named(TXT("dropDownTrajectory")))
		{
			dropDownTrajectory = new Trajectory();
			if (!dropDownTrajectory->load_from_xml(node, _lc))
			{
				error_loading_xml(node, TXT("error loading drop down trajectory"));
				dropDownTrajectory = nullptr;
				result = false;
			}
		}
	}

	useFallingTrajectory = _node->get_name_attribute(TXT("fallingTrajectory"), useFallingTrajectory);
	if (has_own_falling_trajectory())
	{
		if (IO::XML::Node const * node = _node->first_child_named(TXT("fallingTrajectory")))
		{
			fallingTrajectory = new Trajectory();
			if (!fallingTrajectory->load_from_xml(node, _lc))
			{
				error_loading_xml(node, TXT("error loading falling trajectory"));
				fallingTrajectory = nullptr;
				result = false;
			}
		}
	}

	useAdjustmentTrajectory = _node->get_name_attribute(TXT("adjustmentTrajectory"), useAdjustmentTrajectory);
	if (has_own_adjustment_trajectory())
	{
		if (IO::XML::Node const * node = _node->first_child_named(TXT("adjustmentTrajectory")))
		{
			adjustmentTrajectory = new Trajectory();
			if (!adjustmentTrajectory->load_from_xml(node, _lc))
			{
				error_loading_xml(node, TXT("error loading adjustment trajectory"));
				adjustmentTrajectory = nullptr;
				result = false;
			}
		}
	}
	adjustmentTime.load_from_xml(_node, TXT("adjustmentTime"));
	disallowAdjustmentMovementForTime.load_from_xml(_node, TXT("disallowAdjustmentMovementForTime"));
	disallowAdjustmentMovementWhenCollidingForTime.load_from_xml(_node, TXT("disallowAdjustmentMovementWhenCollidingForTime"));

	for_every(node, _node->children_named(TXT("translateVar")))
	{
		result &= load_trajectory_to_apperance_var_from_xml(node, _lc);
	}
	for_every(node, _node->children_named(TXT("var")))
	{
		result &= load_trajectory_to_apperance_var_from_xml(node, _lc);
	}

	return result;
}

bool LegSetup::load_trajectory_to_apperance_var_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;
	Name inTrajectoryName = _node->get_name_attribute(TXT("inTrajectory"));
	Name inAppearanceName = _node->get_name_attribute(TXT("inAppearance"));
	if (inAppearanceName.is_valid())
	{
		TrajectoryToAppearanceVar t2aVar;
		t2aVar.trajectoryVar = inTrajectoryName;
		t2aVar.appearanceVar = inAppearanceName;
		t2aVar.defaultValue.load_from_xml(_node, TXT("defaultValue"));
		t2aVar.defaultValue.load_from_xml(_node, TXT("value"));
		t2aVar.sourceChangeBlendTime = _node->get_float_attribute(TXT("sourceChangeBlendTime"), t2aVar.sourceChangeBlendTime);
		trajectoryToAppearanceVars.push_back(t2aVar);
		if (inTrajectoryName.is_valid())
		{
			translateVarFromTrajectoryToAppearance[inTrajectoryName] = inAppearanceName;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("translateVar defined incorrectly"));
		result = false;
	}
	return result;
}

bool LegSetup::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (has_own_drop_down_trajectory() && dropDownTrajectory.is_set())
	{
		result &= dropDownTrajectory->prepare_for_game(_library, _pfgContext);
	}

	if (has_own_falling_trajectory() && fallingTrajectory.is_set())
	{
		result &= fallingTrajectory->prepare_for_game(_library, _pfgContext);
	}

	if (has_own_adjustment_trajectory() && adjustmentTrajectory.is_set())
	{
		result &= adjustmentTrajectory->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

void LegSetup::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	defaultGait.apply_mesh_gen_params(_context);
	boneForTraceStart.fill_value_with(_context);
	boneForLengthEnd.fill_value_with(_context);
	footBone.fill_value_with(_context);
	footBaseSocket.fill_value_with(_context);
	soundSocket.fill_value_with(_context);

	for_every(gait, perGait)
	{
		gait->apply_mesh_gen_params(_context);
	}
}

bool LegSetup::bake(Lib const & _lib)
{
	bool result = true;

	if (!has_own_drop_down_trajectory())
	{
		dropDownTrajectory = _lib.find_trajectory(useDropDownTrajectory);
		if (!dropDownTrajectory.is_set())
		{
			error(TXT("couldn't find drop down trajectory \"%S\""), useDropDownTrajectory.to_char());
			result = false;
		}
	}

	if (!has_own_falling_trajectory())
	{
		fallingTrajectory = _lib.find_trajectory(useFallingTrajectory);
		if (!fallingTrajectory.is_set())
		{
			error(TXT("couldn't find falling trajectory \"%S\""), useFallingTrajectory.to_char());
			result = false;
		}
	}

	if (!has_own_adjustment_trajectory())
	{
		adjustmentTrajectory = _lib.find_trajectory(useAdjustmentTrajectory);
		if (!adjustmentTrajectory.is_set())
		{
			error(TXT("couldn't find adjustment trajectory \"%S\""), useAdjustmentTrajectory.to_char());
			result = false;
		}
	}

	return result;
}

//

LegInstance::LegInstance()
: instance(nullptr)
, legIndex(NONE)
, legSetup(nullptr)
, action(LegAction::None)
, actionFlags(0)
, startTrajPlacementAtPTTraj(0.0f)
, redoLegMovementDisallowedForTime(0.1f)
, adjustmentMovementDisallowedForTime(0.1f)
, adjustmentMovementDisallowedWhenCollidingForTime(0.1f)
, legPlacementMS(Transform::identity)
, legTrajPlacementMS(Transform::identity)
, trajectoryMS(Vector3::zero)
{
}

void LegInstance::initialise(InstanceContext const & _instanceContext, Walkers::Instance* _instance, int _legIndex)
{
	instance = _instance;
	legIndex = _legIndex;

	an_assert(_instanceContext.get_owner() &&
		   _instanceContext.get_owner()->get_owner() &&
		   _instanceContext.get_owner()->get_owner()->get_presence());
	ModulePresence* presence = _instanceContext.get_owner()->get_owner()->get_presence();
	startTrajPlacement.set_owner_presence(presence);
	endTrajPlacement.set_owner_presence(presence);
	endTrajPlacementDoneForPredictedOwnerPlacement.set_owner_presence(presence);
	endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.set_owner_presence(presence);
#ifdef AN_DEVELOPMENT_OR_PROFILER
	startTrajPlacement.usageInfo = Name(TXT("Wlk, legins::startTrajPlac"));
	endTrajPlacement.usageInfo = Name(TXT("Wlk, legins::endTrajPlac"));
	endTrajPlacementDoneForPredictedOwnerPlacement.usageInfo = Name(TXT("Wlk, legins::endTrajPlacDoneForPredOwnerPlac"));
	endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.usageInfo = Name(TXT("Wlk, legins::endTrajPlacDoneForPredLegPlacBfrTraces"));
#endif

	// allow syncing to main
	gaitPlayback.sync_to(&instance->get_gait_playback());
}

void LegInstance::use_setup(InstanceContext const & _instanceContext, LegSetup const * _legSetup)
{
	legSetup = _legSetup;
	
	// initialise gait setups - 
	gaitSetups.clear();
	gaitSetups.push_back(LegSetup::ForGait::Instance(&legSetup->defaultGait, _instanceContext));
	for_every(gait, legSetup->perGait)
	{
		gaitSetups.push_back(LegSetup::ForGait::Instance(gait, _instanceContext));
	}

	currentDefault = &get_current_default(_instanceContext);
	legMovementDoneForGait = Name::invalid();
	boneForTraceStartIndex = NONE;
	boneForLengthEndIndex = NONE;
	footBoneIndex = NONE;
	if (legSetup->footBone.is_set())
	{
		if (Skeleton const * skeleton = _instanceContext.get_owner()->get_skeleton())
		{
			footBoneIndex = skeleton->get_skeleton()->find_bone_index(legSetup->footBone.get());
			if (footBoneIndex == NONE)
			{
				error(TXT("bone \"%S\" not found"), legSetup->footBone.get().to_char());
			}
		}
	}
	if (legSetup->footBaseSocket.is_set())
	{
		footBaseSocket = legSetup->footBaseSocket.get();
	}
	if (legSetup->soundSocket.is_set())
	{
		soundSocket = legSetup->soundSocket.get();
	}
	legLength = legSetup->legLength;
	collisionTraceMinAlt = legSetup->collisionTraceMinAlt;
	if ((legSetup->boneForTraceStart.is_set() && legSetup->boneForTraceStart.get().is_valid()) ||
		(legSetup->boneForLengthEnd.is_set() && legSetup->boneForLengthEnd.get().is_valid()))
	{
		if (Skeleton const * skeleton = _instanceContext.get_owner()->get_skeleton())
		{
			if (legSetup->boneForTraceStart.is_set() &&
				legSetup->boneForTraceStart.get().is_valid())
			{
				boneForTraceStartIndex = skeleton->get_skeleton()->find_bone_index(legSetup->boneForTraceStart.get());
				if (boneForTraceStartIndex == NONE)
				{
					error(TXT("bone \"%S\" not found"), legSetup->boneForTraceStart.get().to_char());
				}
			}
			if (legSetup->boneForLengthEnd.is_set() &&
				legSetup->boneForLengthEnd.get().is_valid())
			{
				boneForLengthEndIndex = skeleton->get_skeleton()->find_bone_index(legSetup->boneForLengthEnd.get());
				if (boneForLengthEndIndex == NONE)
				{
					error(TXT("bone \"%S\" not found"), legSetup->boneForLengthEnd.get().to_char());
				}
			}
			if (boneForTraceStartIndex != NONE &&
				boneForLengthEndIndex != NONE)
			{
				legLength = (skeleton->get_skeleton()->get_bones_default_placement_MS()[boneForTraceStartIndex].get_translation() -
							 skeleton->get_skeleton()->get_bones_default_placement_MS()[boneForLengthEndIndex].get_translation()).length();
			}
		}
	}

	restart(_instanceContext);
}

void LegInstance::set_start_traj_placement_to_current(InstanceContext const & _instanceContext)
{
	startTrajPlacement.clear_target();

	if (footBaseSocket.is_set())
	{
		Transform footBonePlacement = _instanceContext.get_owner()->calculate_socket_ms(footBaseSocket.get(), _instanceContext.get_owner()->get_final_pose_LS()); // last frame
		startTrajPlacement.set_placement_in_final_room(_instanceContext.get_owner()->get_ms_to_ws_transform().to_world(footBonePlacement),
			_instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}
	else if (footBoneIndex != NONE)
	{
		Transform footBonePlacement = _instanceContext.get_owner()->get_final_pose_MS()->get_bone(footBoneIndex); // last frame
		startTrajPlacement.set_placement_in_final_room(_instanceContext.get_owner()->get_ms_to_ws_transform().to_world(footBonePlacement),
			_instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}
	else if (boneForLengthEndIndex != NONE)
	{
		Transform footBonePlacement = _instanceContext.get_owner()->get_final_pose_MS()->get_bone(boneForLengthEndIndex); // last frame
		startTrajPlacement.set_placement_in_final_room(_instanceContext.get_owner()->get_ms_to_ws_transform().to_world(footBonePlacement),
			_instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}
	else
	{
		warn(TXT("no socket or bone provided to get proper placement after restarting"));
	}
}

void LegInstance::restart(InstanceContext const & _instanceContext)
{
#ifdef INSPECT__START_LEG_MOVEMENT
	output(TXT("========== RESTART %S ================================================="), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
	endTrajPlacement.clear_target();

	set_start_traj_placement_to_current(_instanceContext);

	action = LegAction::None;
	actionFlags = 0;
	isFalling = false;

	legPlacementVelocityMS = Vector3::zero;
}

void LegInstance::stop_sounds()
{
	events.stop_sounds();
}

void LegInstance::advance(InstanceContext const & _instanceContext, float _deltaTime)
{
	an_assert(is_setup_correctly());

	if (_instanceContext.has_just_activated() ||
		_instanceContext.get_owner()->get_owner()->get_presence()->has_teleported() ||
		(! isActive && isActiveRequested))
	{
		restart(_instanceContext);
	}

	if (isActive && !isActiveRequested)
	{
		stop_sounds();
	}

	isActive = isActiveRequested;

	if (!isActive)
	{
		return;
	}

#ifdef AN_ALLOW_EXTENDED_DEBUG
	legPlacementMSCalculatedInThisFrame = false;
#endif

	// because movement gait could change and we can be dependent on that
	currentDefault = &get_current_default(_instanceContext);

	Transform prevLegPlacementMS = legPlacementMS;

	float prevTime = gaitPlayback.get_at_time(); // to monitor if we looped back
	gaitPlayback.advance(_deltaTime);
	if (gaitPlayback.get_at_time() < prevTime)
	{
		// get movement back so we're still inside movement
		movement.apply_offset(-movement.gaitLength);
	}

	// if placements are not set, initialise them
	if (!startTrajPlacement.is_active())
	{
		startTrajPlacementAtPTTraj = 0.0f;
		startTrajPlacement.clear_target();
		startTrajPlacement.set_placement_in_final_room(_instanceContext.get_owner()->get_ms_to_ws_transform().to_world(get_for_gait_instance(currentDefault)->calculate_placement_MS(_instanceContext)),
													   _instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}
	if (!endTrajPlacement.is_active())
	{
		endTrajPlacement = startTrajPlacement;
		endTrajPlacementValid = false;
		endTrajPhysicalMaterial = nullptr;
	}

#ifdef AN_DEVELOPMENT
	find_end_placement(_instanceContext, true);
#endif

	if (action == LegAction::None)
	{
		find_end_placement(_instanceContext);
		put_leg_down(_instanceContext);
	}

	if (_instanceContext.has_just_activated() ||
		_instanceContext.get_owner()->get_owner()->get_presence()->has_teleported())
	{
		start_drop_down_movement(_instanceContext);
		if ((startTrajPlacement.get_placement_in_owner_room().get_translation() - endTrajPlacement.get_placement_in_owner_room().get_translation()).length() < 0.02f)
		{
			// assume it was already on the ground, ignore further foot steps
			footStepDone = true;
		}
	}

	bool newIsFalling = false;

	if (instance->get_setup()->gravityPresenceTracesTouchSurroundingsRequired.is_set() &&
		instance->get_setup()->gravityPresenceTracesTouchSurroundingsRequired.get())
	{
		newIsFalling = !_instanceContext.get_owner()->get_owner()->get_presence()->do_gravity_presence_traces_touch_surroundings();
	}

	if (newIsFalling ^ isFalling)
	{
		isFalling = newIsFalling;

		if (isFalling)
		{
			start_for_falling_movement(_instanceContext);
		}
		else
		{
			put_leg_down(_instanceContext);
		}
	}

	if (isFalling)
	{
		// udpate end traj placement each frame
		endTrajPlacementValid = false;
		endTrajPhysicalMaterial = nullptr;
		endTrajPlacement.clear_target();
		an_assert(currentDefault);
		Transform const beAtMS = get_for_gait_instance(currentDefault)->calculate_placement_MS(_instanceContext);
		Transform const ms2ws = _instanceContext.get_owner()->get_ms_to_ws_transform();
		Transform const beAtWS = ms2ws.to_world(beAtMS);
		endTrajPlacement.set_placement_in_final_room(beAtWS, _instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}

	if (action == LegAction::Up && ! isFalling)
	{
		if (movement.trajectory &&
			gaitPlayback.get_pt() >= movement.trajectory->get_foot_step_at_pt() &&
			gaitPlayback.get_prev_pt() < movement.trajectory->get_foot_step_at_pt())
		{
			do_foot_step(_instanceContext);
		}
		// check if we moved pass end
		if (gaitPlayback.get_at_time() >= movement.endsAt &&
			(gaitPlayback.get_prev_time() < movement.endsAt || movement.gaitLength == 0.0f))
		{
			put_leg_down(_instanceContext);
		}
		else
		{
			if (!is_flag_set(actionFlags, LegActionFlag::Adjustment)) // don't do redo for adjustments - doesn't make much sense
			{
				bool allowAdjustmentsDueToFootstep = ! movement.trajectory ||
									  gaitPlayback.get_pt() <= movement.trajectory->get_foot_step_at_pt() - 0.1f;
				redoLegMovementDisallowedForTime = max(0.0f, redoLegMovementDisallowedForTime - _deltaTime);
				if (allowAdjustmentsDueToFootstep && redoLegMovementDisallowedForTime == 0.0f)
				{
					// check if we shouldn't modify end placement (because we changed movement path)
					if (endTrajPlacementDoneForPredictedOwnerPlacement.is_active() ||
						(endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.is_active() && currentDefault->maxDifferenceOfLegToRedoLegMovement > 0.0f))
					{
						Transform endPlacementOfOwnerWS;
						Transform endPlacementOfLegWS;
						calculate_end_placements(_instanceContext, REF_ endPlacementOfOwnerWS, REF_ endPlacementOfLegWS);

						bool redoLegMovement = false;

						if (endTrajPlacementDoneForPredictedOwnerPlacement.is_active())
						{
							// get just locations in OS
							Vector3 endPlacementOfOwnerOS = _instanceContext.get_owner()->get_os_to_ws_transform().location_to_local(endPlacementOfOwnerWS.get_translation());
							Transform doneForOwnerOS = endTrajPlacementDoneForPredictedOwnerPlacement.calculate_placement_in_os();
							float diffSquared = (doneForOwnerOS.get_translation() - endPlacementOfOwnerOS).length_squared();

							redoLegMovement |= diffSquared > sqr(currentDefault->maxDifferenceOfOwnerToRedoLegMovement);
						}

						if (!redoLegMovement &&
							endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.is_active() &&
							currentDefault->maxDifferenceOfLegToRedoLegMovement > 0.0f)
						{
							// get just locations in OS
							Vector3 endPlacementOfLegOS = _instanceContext.get_owner()->get_os_to_ws_transform().location_to_local(endPlacementOfLegWS.get_translation());
							Transform doneForLegOS = endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.calculate_placement_in_os();
							float diffSquared = (doneForLegOS.get_translation() - endPlacementOfLegOS).length_squared();

							redoLegMovement |= diffSquared > sqr(currentDefault->maxDifferenceOfLegToRedoLegMovement);
						}

						if (redoLegMovement)
						{
#ifdef AN_ALLOW_EXTENDED_DEBUG
							IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
							{
								output(TXT("leg %i redo leg movement - time left %.3f [%S]"), legIndex, movement.get_time_left(gaitPlayback), legPlacementMS.get_translation().to_string().to_char());
							}
#endif
							// we will need latest leg placement to redo
							calculate_leg_placements_MS(_instanceContext);
#ifdef INSPECT__START_LEG_MOVEMENT
							output(TXT("---- START LEG MOVEMENT %S ---- redo leg movement"), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
							start_leg_movement(_instanceContext, movement, actionFlags | LegActionFlag::RedoLegMovement); // use same flags
						}
					}
				}
			}
		}
	}

	if (action == LegAction::Down && !isFalling)
	{
		// maybe we should move
		bool allowAdjustments = false;
		float adjustmentRequiredDistance = 1000.0f;
		float adjustmentRequiredVerticalDistance = 1000.0f;
		Optional<float> adjustmentRequiredForwardAngle;
		if (auto * is = instance->get_setup())
		{
			allowAdjustments = is->allowAdjustments.get();
			adjustmentRequiredDistance = is->adjustmentRequiredDistance.get();
			if (is->adjustmentRequiredVerticalDistance.is_set())
			{
				adjustmentRequiredVerticalDistance = is->adjustmentRequiredVerticalDistance.get();
			}
			if (is->adjustmentRequiredForwardAngle.is_set())
			{
				adjustmentRequiredForwardAngle = is->adjustmentRequiredForwardAngle.get();
			}
		}
		if (Gait const * gait = instance->get_current_gait())
		{
			if (GaitLegMovement const * newMovement = gait->find_movement(legIndex, gaitPlayback.get_at_time(), 0.1f, 0.05f)) // as parameter? but it actually makes sense to have 10% and small amount of time
			{
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(WalkersLegMovement)
				{
					output(TXT("leg %i start new movement (was down and gait \"%S\" said to move, time left %.3f/%.3f) [%S]"), legIndex, gait->get_name().to_char(), newMovement->get_time_left(gaitPlayback), newMovement->endsAt - newMovement->startsAt, legPlacementMS.get_translation().to_string().to_char());
				}
#endif
#ifdef INSPECT__START_LEG_MOVEMENT
				output(TXT("---- START LEG MOVEMENT %S ---- none?"), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
				start_leg_movement(_instanceContext, *newMovement, LegActionFlag::None);
			}
			if (gait->get_does_allow_adjustments().is_set())
			{
				allowAdjustments = gait->get_does_allow_adjustments().get();
			}
		}

		if (currentDefault->allowAdjustments.is_set())
		{
			allowAdjustments = currentDefault->allowAdjustments.get();
		}
		if (currentDefault->adjustmentRequiredDistance.is_set())
		{
			adjustmentRequiredDistance = currentDefault->adjustmentRequiredDistance.get();
		}
		if (currentDefault->adjustmentRequiredVerticalDistance.is_set())
		{
			adjustmentRequiredVerticalDistance = currentDefault->adjustmentRequiredVerticalDistance.get();
		}
		if (currentDefault->adjustmentRequiredForwardAngle.is_set())
		{
			adjustmentRequiredForwardAngle = currentDefault->adjustmentRequiredForwardAngle.get();
		}


		// check adjustments, but only if we have no gait (we check even with gait!)
		if (allowAdjustments)
		{
			if (_instanceContext.is_colliding_with_non_scenery())
			{
				// limit, we want to do this more often
				adjustmentMovementDisallowedForTime = min(adjustmentMovementDisallowedForTime, adjustmentMovementDisallowedWhenCollidingForTime);
			}
			adjustmentMovementDisallowedForTime = max(0.0f, adjustmentMovementDisallowedForTime - _deltaTime);
			if (instance->can_leg_adjust(legIndex) &&
				adjustmentMovementDisallowedForTime == 0.0f)
			{
				Transform checkAgainstMS = get_for_gait_instance(currentDefault)->calculate_placement_MS(_instanceContext);

				if (defaultLegPlacementForAdjustmentMS.is_set())
				{
					float distance = (defaultLegPlacementForAdjustmentMS.get().get_translation() - checkAgainstMS.get_translation()).length_2d();
					if (distance > adjustmentRequiredDistance)
					{
						// reset the default leg placement
						legPlacementAfterAdjustmentMS.clear();
						defaultLegPlacementForAdjustmentMS = checkAgainstMS;
					}
				}
				else
				{
					defaultLegPlacementForAdjustmentMS = checkAgainstMS;
				}
				if (legPlacementAfterAdjustmentMS.is_set() && legAdjustedForGait == instance->get_current_gait_name())
				{
					checkAgainstMS = legPlacementAfterAdjustmentMS.get();
				}

				bool adjustmentNeeded = false;
				if (!adjustmentNeeded)
				{
					Vector3 againstMS = checkAgainstMS.get_translation();
					Vector3 legMS = legPlacementMS.get_translation();
					float distance = (againstMS - legMS).length_2d();
					if (distance > adjustmentRequiredDistance)
					{
						adjustmentNeeded = true;
					}
					if (abs(againstMS.z - legMS.z) > adjustmentRequiredVerticalDistance)
					{
						adjustmentNeeded = true;
					}
				}
				if (adjustmentRequiredForwardAngle.is_set() && !adjustmentNeeded)
				{
					float dot = Vector3::dot((legPlacementMS.get_axis(Axis::Y) * Vector3(1.0f, 1.0f, 0.0f)).normal(),
											 (checkAgainstMS.get_axis(Axis::Y) * Vector3(1.0f, 1.0f, 0.0f)).normal());
					if (dot < cos_deg(adjustmentRequiredForwardAngle.get()))
					{
						adjustmentNeeded = true;
					}
				}

				if (adjustmentNeeded)
				{
					start_adjustment_movement(_instanceContext);
				}
			}
		}
	}

	// calculate leg placement in MS
	calculate_leg_placements_MS(_instanceContext);

	// update gait vars
	update_gait_vars(_instanceContext, _deltaTime);

	// update events
	events.advance(*instance, _instanceContext, movement.trajectory && action == LegAction::Up && ! isFalling? &movement.trajectory->get_events() : nullptr,
		movement.calculate_clamped_prop_through(gaitPlayback.get_prev_time()),
		movement.calculate_clamped_prop_through(gaitPlayback.get_at_time()),
		false, soundSocket);

	// and finally update velocity (might be useful for drop downs)
	legPlacementVelocityMS = (legPlacementMS.get_translation() - prevLegPlacementMS.get_translation()) / _deltaTime;

	debug_filter(ac_walkers);
	debug_context(_instanceContext.get_owner()->get_owner()->get_presence()->get_in_room());
	debug_subject(_instanceContext.get_owner()->get_owner());
	debug_push_transform(_instanceContext.get_owner()->get_owner()->get_presence()->get_placement());

	if (action == LegAction::Up)
	{
		debug_draw_sphere(true, false, Colour::orange, 0.8f, Sphere(startTrajPlacement.calculate_placement_in_os().get_translation(), 0.01f));
		debug_draw_arrow(true, Colour::green, startTrajPlacement.calculate_placement_in_os().get_translation(), endTrajPlacement.calculate_placement_in_os().get_translation());
		debug_draw_sphere(true, false, Colour::green, 0.8f, Sphere(endTrajPlacement.calculate_placement_in_os().get_translation(), 0.01f));
		debug_draw_transform_size_coloured(true, startTrajPlacement.calculate_placement_in_os(), 0.3f, Colour::red, Colour::green, Colour::blue);
		debug_draw_transform_size_coloured(true, endTrajPlacement.calculate_placement_in_os(), 0.3f, Colour::red, Colour::green, Colour::blue);
	}
	if (action == LegAction::Down)
	{
		debug_draw_sphere(true, false, Colour::blue, 0.8f, Sphere(startTrajPlacement.calculate_placement_in_os().get_translation(), 0.01f));
		debug_draw_transform_size_coloured(true, startTrajPlacement.calculate_placement_in_os(), 0.3f, Colour::red, Colour::green, Colour::blue);
	}
	if (action == LegAction::None)
	{
		debug_draw_sphere(true, false, Colour::magenta, 0.8f, Sphere(startTrajPlacement.calculate_placement_in_os().get_translation(), 0.01f));
		debug_draw_transform_size_coloured(true, startTrajPlacement.calculate_placement_in_os(), 0.3f, Colour::red, Colour::green, Colour::blue);
	}
	if (_instanceContext.get_relative_velocity_linear().is_set())
	{
		debug_draw_arrow(true, Colour::blue, Vector3::zero, _instanceContext.get_relative_velocity_linear().get());
	}
	debug_pop_transform();
	debug_no_subject();
	debug_no_context();
	debug_no_filter();

#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("  leg (%i) act:%C gait %.3f [%.3f] %S[%.3f->%.3f:%.3f] %S"),
				legIndex,
				action == LegAction::Up ? 'U' : (action == LegAction::Down ? 'D' : ' '),
				gaitPlayback.get_at_time(), gaitPlayback.get_playback_speed() * 100.0f,
				movement.trajectory ? TXT("movTracjetory") : TXT(""),
				movement.startsAt, movement.endsAt, movement.endPlacementAt,
				isFalling? TXT("isFalling") : TXT("")
			);
		}
	}
#endif

}

LegSetup::ForGait::Instance const * LegInstance::get_for_gait_instance(LegSetup::ForGait const* _gait) const
{
	if (recentGaitSetup && recentGaitSetup->gait == _gait)
	{
		return recentGaitSetup;
	}
	for_every(gaitSetup, gaitSetups)
	{
		if (gaitSetup->gait == _gait)
		{
			recentGaitSetup = gaitSetup;
			return recentGaitSetup;
		}
	}
	an_assert(nullptr, TXT("did you initialise it properly, set it up? why we haven't found gaitSetup?"));
	return nullptr;
}

void LegInstance::update_gait_vars(InstanceContext const & _instanceContext, float _deltaTime)
{
	ARRAY_STACK(CustomGaitVar, prevGaitVars, gaitVars.get_size());
	prevGaitVars = gaitVars;
	gaitVars.clear();

	// get from trajectory
	if (action == LegAction::Up && movement.trajectory)
	{
		movement.trajectory->calculate_and_add_gait_vars(movement.calculate_clamped_prop_through(gaitPlayback.get_at_time()), gaitVars, legSetup->translateVarFromTrajectoryToAppearance);
	}

	// get defaults
	for_every(defaultGaitVar, currentDefault->gaitVars)
	{
		bool found = false;
		for_every(gaitVar, gaitVars)
		{
			if (gaitVar->varID == defaultGaitVar->varID)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			CustomGaitVar cgv = *defaultGaitVar;
			cgv.source = currentDefault;
			gaitVars.push_back(cgv);
		}
	}

	for_every(t2aVar, legSetup->trajectoryToAppearanceVars)
	{
		if (t2aVar->defaultValue.is_set())
		{
			bool found = false;
			for_every(gaitVar, gaitVars)
			{
				if (gaitVar->varID == t2aVar->appearanceVar)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				gaitVars.push_back(CustomGaitVar(t2aVar->appearanceVar, t2aVar->defaultValue.get(), &legSetup->trajectoryToAppearanceVars));
			}
		}
	}

	for_every(gaitVar, gaitVars)
	{
		for_every(prevGaitVar, prevGaitVars)
		{
			if (gaitVar->varID == prevGaitVar->varID)
			{
				gaitVar->blendTimeLeft = max(0.0f, prevGaitVar->blendTimeLeft - _deltaTime);
				if (gaitVar->source != prevGaitVar->source)
				{
					for_every(t2aVar, legSetup->trajectoryToAppearanceVars)
					{
						if (t2aVar->appearanceVar == gaitVar->varID)
						{
							gaitVar->blendTimeLeft = max(gaitVar->blendTimeLeft, t2aVar->sourceChangeBlendTime);
						}
					}
				}
				if (gaitVar->blendTimeLeft > 0.0f)
				{
					gaitVar->value = blend_to_using_time(prevGaitVar->value, gaitVar->value, gaitVar->blendTimeLeft, _deltaTime);
				}
				break;
			}
		}
	}
}

void LegInstance::calculate_leg_placements_MS(InstanceContext const & _instanceContext)
{
	if (!startTrajPlacement.is_active() || !endTrajPlacement.is_active())
	{
		return; 
	}
#ifdef AN_ALLOW_EXTENDED_DEBUG
	legPlacementMSCalculatedInThisFrame = true;
#endif
	float pt = 1.0f;
	if (action == LegAction::Up)
	{
		pt = movement.calculate_clamped_prop_through(gaitPlayback.get_at_time());
	}
	float ptTraj = movement.trajectory ? movement.trajectory->transform_pt_to_pt_traj(pt) : pt;
	float adjustedPtTraj = clamp((ptTraj - startTrajPlacementAtPTTraj) / (1.0f - startTrajPlacementAtPTTraj), 0.0f, 1.0f);
	Transform startTPOS = startTrajPlacement.calculate_placement_in_os();
	Transform endTPOS = endTrajPlacement.calculate_placement_in_os();
	Transform const ms2os = _instanceContext.get_owner()->get_ms_to_os_transform();
	trajectoryMS = ms2os.vector_to_local(endTPOS.get_translation() - startTPOS.get_translation());
	legTrajPlacementMS = ms2os.to_local(Transform::lerp(adjustedPtTraj, startTPOS, endTPOS));
	legPlacementMS = legTrajPlacementMS;
	an_assert(legPlacementMS.get_orientation().is_normalised());
	if (action == LegAction::Up && movement.trajectory)
	{
		// apply offset to traj to get leg placement
		Transform const offset = movement.trajectory->calculate_offset_placement_at_pt(pt, instance->get_trajectory_object_size(),
			movement.trajectory->is_relative_to_movement_direction()? Rotator3(0.0f, legMovementDirection, 0.0f) : Rotator3::zero);
		legPlacementMS = legPlacementMS.to_world(offset);
		an_assert(legPlacementMS.get_orientation().is_normalised());
	}
}

LegSetup::ForGait const & LegInstance::get_current_default(InstanceContext const & _instanceContext) const
{
	an_assert(legSetup);
	if (ModuleMovement const * movement = _instanceContext.get_owner()->get_owner()->get_movement())
	{
		if (LegSetup::ForGait const * found = legSetup->perGait.get_existing(movement->get_current_gait_name()))
		{
			return *found;
		}
	}
	if (LegSetup::ForGait const * found = legSetup->perGait.get_existing(gaitPlayback.get_gait_name()))
	{
		return *found;
	}
	return legSetup->defaultGait;
}

void LegInstance::on_changed_gait(InstanceContext const & _instanceContext, bool _forceDropDown)
{
	an_assert(is_setup_correctly());

	if (!is_active())
	{
		return;
	}

	currentDefault = &get_current_default(_instanceContext);

	todo_note(TXT("if up, put down with emergency, unless it would take more time to put with emergency than to finish"));
	bool mayStartNewMove = false;
	bool doDropDown = false;
	// try to continue for now?
	if (action == LegAction::Up)
	{
		doDropDown = _forceDropDown;
#ifdef AN_ALLOW_EXTENDED_DEBUG
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("leg %i was up, time left %.3f on sync [%S]"), legIndex, movement.get_time_left(gaitPlayback), legPlacementMS.get_translation().to_string().to_char());
		}
#endif
		// decouple from gait, so we can continue with our current movement (unless we want to stop, but then it won't really mattter)
		gaitPlayback.dont_sync_to();

		if (instance->get_current_gait()) // if we're switching to gait, otherwise let legs finish movement? if they go too far, they will be redone
		{
			if (movement.get_time_left(gaitPlayback) >= currentDefault->dropDownTime.min)
			{
				// only if we have more time left than drop down
				doDropDown = true;
				if (movement.calculate_clamped_prop_through(gaitPlayback.get_at_time()) < currentDefault->allowSwitchingLegMovementPT)
				{
					mayStartNewMove = true;
				}
			}
		}
	}
	else
	{
		// we may sync right now
		gaitPlayback.sync_to(&instance->get_gait_playback());
		mayStartNewMove = true;
	}

	if (mayStartNewMove)
	{
		if (Gait const * gait = instance->get_current_gait())
		{
			if (GaitLegMovement const * newMovement = gait->find_movement(legIndex, instance->get_gait_playback().get_at_time(), 0.5f, 1000.0f)) // don't start movement if we're in second half of it
			{
				// we will need latest leg placement to start
				calculate_leg_placements_MS(_instanceContext);

				// start syncing playback
				gaitPlayback.sync_to(&instance->get_gait_playback());
#ifdef AN_ALLOW_EXTENDED_DEBUG
				IF_EXTENDED_DEBUG(WalkersLegMovement)
				{
					output(TXT("leg %i start on sync [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
				}
#endif
				// force new movement to adjust starting time!
#ifdef INSPECT__START_LEG_MOVEMENT
				output(TXT("---- START LEG MOVEMENT %S ---- forced new movement"), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
				start_leg_movement(_instanceContext, *newMovement, LegActionFlag::ForceNewMovement);
				doDropDown = false;
			}
		}
	}

	if (doDropDown)
	{
		start_drop_down_movement(_instanceContext);
	}
}

void LegInstance::do_foot_step(InstanceContext const & _instanceContext)
{
	if (!footStepDone)
	{
		events.do_foot_step(*instance, _instanceContext, movement.trajectory && action == LegAction::Up && !isFalling ? &movement.trajectory->get_events() : nullptr, endTrajPlacement, endTrajPhysicalMaterial);
		footStepDone = true;
	}
}

void LegInstance::put_leg_down(InstanceContext const & _instanceContext)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(WalkersLegMovement)
	{
		output(TXT("leg %i put leg down [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
	}
#endif
	// should be used only if leg is actually put at the end placement!

	instance->mark_leg_down(legIndex);

	if (is_flag_set(actionFlags, LegActionFlag::Adjustment))
	{
		legPlacementAfterAdjustmentMS = legPlacementMS; 
		legAdjustedForGait = legMovementDoneForGait;
		instance->stop_leg_adjustment();
	}
	else
	{
		legPlacementAfterAdjustmentMS.clear();
		defaultLegPlacementForAdjustmentMS.clear();
		legAdjustedForGait = Name::invalid();
	}

	do_foot_step(_instanceContext);

	action = LegAction::Down;
	actionFlags = 0;
	legMovementDirection = 0.0f;
	todo_note(TXT("clear movement somehow?"));

	// disallow adjustments after putting leg down
	adjustmentMovementDisallowedForTime = calculate_disallow_adjustment_time();
	adjustmentMovementDisallowedWhenCollidingForTime = calculate_disallow_adjustment_time_when_colliding();

	// force to be at traj placement
	startTrajPlacement = endTrajPlacement;

	// allow syncing to main
	gaitPlayback.sync_to(&instance->get_gait_playback());
}

void LegInstance::start_drop_down_movement(InstanceContext const & _instanceContext)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(WalkersLegMovement)
	{
		output(TXT("leg %i start drop down movement [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
	}
#endif
	an_assert(legSetup);
	// we will need latest leg placement to do drop down
	calculate_leg_placements_MS(_instanceContext);
	// restart playback and disallow syncing
	gaitPlayback.switch_to(nullptr);
	gaitPlayback.dont_sync_to();
	// start new movement, drop down
	GaitLegMovement dropDownMovement;
	dropDownMovement.startsAt = gaitPlayback.get_at_time();
	dropDownMovement.endsAt = dropDownMovement.startsAt + currentDefault->dropDownTime.max;
	dropDownMovement.endPlacementAt = dropDownMovement.endsAt;
	dropDownMovement.trajectory = legSetup->dropDownTrajectory.get();
#ifdef INSPECT__START_LEG_MOVEMENT
	output(TXT("---- START LEG MOVEMENT %S ---- drop down movement"), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
	start_leg_movement(_instanceContext, dropDownMovement, LegActionFlag::DropDown | LegActionFlag::ForceNewMovement);
}

void LegInstance::start_for_falling_movement(InstanceContext const & _instanceContext)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(WalkersLegMovement)
	{
		output(TXT("leg %i start for falling movement [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
	}
#endif
	an_assert(legSetup);
	// we will need latest leg placement to do drop down
	calculate_leg_placements_MS(_instanceContext);
	// restart playback and disallow syncing
	gaitPlayback.switch_to(nullptr);
	gaitPlayback.dont_sync_to();
	// start new movement, drop down
	GaitLegMovement forFallingMovement;
	forFallingMovement.startsAt = gaitPlayback.get_at_time();
	forFallingMovement.endsAt = forFallingMovement.startsAt + currentDefault->fallingTime.get_at(Random::get_float(0.0f, 1.0f));
	forFallingMovement.endPlacementAt = forFallingMovement.endsAt;
	forFallingMovement.trajectory = legSetup->fallingTrajectory.get();
#ifdef INSPECT__START_LEG_MOVEMENT
	output(TXT("---- START LEG MOVEMENT %S ---- for falling movement"), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
	start_leg_movement(_instanceContext, forFallingMovement, LegActionFlag::Falling | LegActionFlag::ForceNewMovement);
}

float LegInstance::calculate_adjustment_time() const
{
	float useAdjustmentTime = 0.2f;
	if (auto * is = instance->get_setup())
	{
		useAdjustmentTime = instance->get_setup()->adjustmentTime.get();
	}
	if (legSetup->adjustmentTime.is_set())
	{
		useAdjustmentTime = legSetup->adjustmentTime.get();
	}
	if (currentDefault->adjustmentTime.is_set())
	{
		useAdjustmentTime = currentDefault->adjustmentTime.get();
	}

	return useAdjustmentTime;
}

float LegInstance::calculate_disallow_adjustment_time() const
{
	float adjustmentMovementDisallowedForTime = 0.2f;
	if (auto* is = instance->get_setup())
	{
		adjustmentMovementDisallowedForTime = is->disallowAdjustmentMovementForTime.get().get_at(Random::get_float(0.0f, 1.0f));
	}
	if (legSetup->disallowAdjustmentMovementForTime.is_set())
	{
		adjustmentMovementDisallowedForTime = legSetup->disallowAdjustmentMovementForTime.get().get_at(Random::get_float(0.0f, 1.0f));
	}
	if (currentDefault->disallowAdjustmentMovementForTime.is_set())
	{
		adjustmentMovementDisallowedForTime = currentDefault->disallowAdjustmentMovementForTime.get().get_at(Random::get_float(0.0f, 1.0f));
	}
	return adjustmentMovementDisallowedForTime;
}

float LegInstance::calculate_disallow_adjustment_time_when_colliding() const
{
	float adjustmentMovementDisallowedWhenCollidingForTime = 0.2f;
	if (auto* is = instance->get_setup())
	{
		adjustmentMovementDisallowedWhenCollidingForTime = is->disallowAdjustmentMovementWhenCollidingForTime.get().get_at(Random::get_float(0.0f, 1.0f));
	}
	if (legSetup->disallowAdjustmentMovementWhenCollidingForTime.is_set())
	{
		adjustmentMovementDisallowedWhenCollidingForTime = legSetup->disallowAdjustmentMovementWhenCollidingForTime.get().get_at(Random::get_float(0.0f, 1.0f));
	}
	if (currentDefault->disallowAdjustmentMovementWhenCollidingForTime.is_set())
	{
		adjustmentMovementDisallowedWhenCollidingForTime = currentDefault->disallowAdjustmentMovementWhenCollidingForTime.get().get_at(Random::get_float(0.0f, 1.0f));
	}
	return adjustmentMovementDisallowedWhenCollidingForTime;
}

void LegInstance::start_adjustment_movement(InstanceContext const & _instanceContext)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(WalkersLegMovement)
	{
		output(TXT("leg %i start adjustment movement [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
	}
#endif
	an_assert(legSetup);
	// restart playback and disallow syncing
	gaitPlayback.switch_to(nullptr);
	gaitPlayback.dont_sync_to();
	// start new movement, drop down
	GaitLegMovement adjustmentMovement;
	adjustmentMovement.startsAt = gaitPlayback.get_at_time();
	float useAdjustmentTime = calculate_adjustment_time();
	adjustmentMovement.endsAt = adjustmentMovement.startsAt + useAdjustmentTime;
	adjustmentMovement.endPlacementAt = adjustmentMovement.endsAt + useAdjustmentTime * 0.5f; // in case we would have to adjust again
	adjustmentMovement.trajectory = legSetup->adjustmentTrajectory.get();
#ifdef INSPECT__START_LEG_MOVEMENT
	output(TXT("---- START LEG MOVEMENT %S ---- adjustment"), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
	start_leg_movement(_instanceContext, adjustmentMovement, LegActionFlag::Adjustment);
}

void LegInstance::start_leg_movement(InstanceContext const & _instanceContext, GaitLegMovement const & _newMovement, int _actionFlags)
{
	// it has to be set
	if (!startTrajPlacement.is_active())
	{
#ifdef INSPECT__START_LEG_MOVEMENT
		output(TXT("========== NO START TRAJ ON START LEG MOVEMENT %S ================================================="), _instanceContext.get_owner()->get_owner()->ai_get_name().to_char());
#endif
		set_start_traj_placement_to_current(_instanceContext);
	}

#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
	{
		output(TXT("leg %i start leg movement [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
	}
#endif
	bool wasLegUp = action == LegAction::Up;
	bool continued = wasLegUp;
	bool allowMovementTimeAdjustments = true;
	bool forceMovementTimeAdjustments = false;

	// manage flags that should be not stored
	if (is_flag_set(_actionFlags, LegActionFlag::ForceNewMovement))
	{
		continued = false;
		forceMovementTimeAdjustments = true;
		clear_flag(_actionFlags, LegActionFlag::ForceNewMovement);
	}
	if (is_flag_set(_actionFlags, LegActionFlag::RedoLegMovement))
	{
		allowMovementTimeAdjustments = false;
		clear_flag(_actionFlags, LegActionFlag::RedoLegMovement);
	}

	// inform instance about legs up/down
	instance->mark_leg_up(legIndex);
	if (is_flag_set(_actionFlags, LegActionFlag::Adjustment) &&
		!is_flag_set(actionFlags, LegActionFlag::Adjustment))
	{
		instance->start_leg_adjustment();
	}
	if (is_flag_set(actionFlags, LegActionFlag::Adjustment) &&
		!is_flag_set(_actionFlags, LegActionFlag::Adjustment))
	{
		instance->stop_leg_adjustment();
	}

	if (wasLegUp)
	{
#ifdef AN_ALLOW_EXTENDED_DEBUG
		an_assert_info(legPlacementMSCalculatedInThisFrame);
		if (!legPlacementMSCalculatedInThisFrame)
		{
			warn(TXT("leg %i didn't have leg placement MS calculated in this frame yet!"), legIndex);
		}
#endif
	}

#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("leg %i start leg movement (post was leg up) [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
		}
	}
#endif

	action = LegAction::Up;
	actionFlags = _actionFlags;
	footStepDone = false;

	legMovementDoneForGait = instance->get_current_gait_name();

	// copy movement and adjust it to contain current gait playback
	movement = _newMovement;
	movement.adjust_to_contain(gaitPlayback.get_at_time());
	if ((!continued && allowMovementTimeAdjustments) || forceMovementTimeAdjustments)
	{
		// if started movement pretend that we started just now - do this only if we didn't pass "startsAt" in this frame
		if (forceMovementTimeAdjustments ||
			gaitPlayback.get_at_time() < movement.startsAt ||
			gaitPlayback.get_prev_time() > movement.startsAt)
		{
			// adjust start, but leave 0.1s before end safe
			movement.startsAt = max(movement.startsAt, min(movement.endsAt - 0.1f, gaitPlayback.get_at_time()));
		}
	}

	find_end_placement(_instanceContext);

	// calculate where leg will be (using end traj placement) from start traj placement - if we were already moving, just adjust to that, if we were not moving, it will be our current placement) - do that to rotate offset
	legMovementDirection = Rotator3::normalise_axis(Rotator3::get_yaw(endTrajPlacement.calculate_placement_in_os().get_translation() - startTrajPlacement.calculate_placement_in_os().get_translation()));

#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("leg %i start leg movement (post find end placement) [%S]"), legIndex, legPlacementMS.get_translation().to_string().to_char());
		}
	}
#endif

	if (wasLegUp)
	{
		// update start traj placement for when leg was up
		float pt = movement.calculate_clamped_prop_through(gaitPlayback.get_at_time());
		float ptTraj = movement.trajectory ? movement.trajectory->transform_pt_to_pt_traj(pt) : pt;
		startTrajPlacementAtPTTraj = min(0.999f, ptTraj);

		// use current leg placement and find traj placement
		Transform trajPlacementMS = legPlacementMS;
		if (movement.trajectory)
		{
			Transform const offset = movement.trajectory->calculate_offset_placement_at_pt(pt, instance->get_trajectory_object_size(),
				movement.trajectory->is_relative_to_movement_direction() ? Rotator3(0.0f, legMovementDirection, 0.0f) : Rotator3::zero);
			trajPlacementMS = trajPlacementMS.to_world(offset.inverted());
		}
		Transform trajPlacementWS = _instanceContext.get_owner()->get_ms_to_ws_transform().to_world(trajPlacementMS);

		// update start traj placement
		startTrajPlacement.clear_target();
		startTrajPlacement.set_placement_in_final_room(trajPlacementWS, _instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}
	else
	{
		// we just started
		startTrajPlacementAtPTTraj = 0.0f;
	}

	legPlacementAfterAdjustmentMS.clear();
	defaultLegPlacementForAdjustmentMS.clear();
	legAdjustedForGait = Name::invalid();

	redoLegMovementDisallowedForTime = currentDefault->disallowRedoLegMovementForTime;
	adjustmentMovementDisallowedForTime = calculate_disallow_adjustment_time();

	todo_note(TXT("disconnect start traj from any object - it should be just in air"));

#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("leg %i leg movement will go for %.3f [%S]"), legIndex, movement.get_time_left(gaitPlayback), legPlacementMS.get_translation().to_string().to_char());
		}
	}
#endif
}

void LegInstance::calculate_end_placements(InstanceContext const & _instanceContext, REF_ Transform & _endPlacementOfOwnerWS, REF_ Transform & _endPlacementOfLegWS) const
{
	float timeLeftToEnd = action == LegAction::Up? movement.get_time_left_to_end_placement(gaitPlayback) : 0.0f;
#ifdef AN_ALLOW_EXTENDED_DEBUG
	IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
	{
		output(TXT("calculating end placements for %i time left %.3f gp:%.3f(%.1f%%)->e:%.3f"), legIndex, timeLeftToEnd, gaitPlayback.get_at_time(), gaitPlayback.get_playback_speed() * 100.0f, movement.endPlacementAt);
	}
#endif
	Transform endPlacementOfOwnerMS = _instanceContext.where_will_I_be_in_MS(timeLeftToEnd);
	todo_note(TXT("now no bone is provided!"));
	Transform endPlacementOfLegMS = _instanceContext.where_leg_will_be_in_MS(timeLeftToEnd, NONE, get_for_gait_instance(currentDefault)->calculate_placement_MS(_instanceContext));

	// calculate where in world we are going to be and where leg should be
	_endPlacementOfOwnerWS = _instanceContext.get_owner()->get_ms_to_ws_transform().to_world(endPlacementOfOwnerMS);
	_endPlacementOfLegWS = _endPlacementOfOwnerWS.to_world(endPlacementOfLegMS);
}

void LegInstance::find_end_placement(InstanceContext const & _instanceContext, bool _forDebug)
{
#ifdef AN_DEVELOPMENT
	if (_forDebug)
	{
		debug_filter(ac_walkers_hitLocations);
		bool shouldContinue = debug_is_filter_allowed();
		debug_no_filter();
		if (!shouldContinue)
		{
			return;
		}
	}
#endif
#ifdef AN_DEVELOPMENT
	if (! _forDebug)
#endif
	{
		endTrajPlacementValid = false;
		endTrajPhysicalMaterial = nullptr;
		endTrajPlacement.clear_target();
		endTrajPlacementDoneForPredictedOwnerPlacement.clear_target();
		endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.clear_target();
	}

	Transform const ms2ws = _instanceContext.get_owner()->get_ms_to_ws_transform();
	Transform const ms2os = _instanceContext.get_owner()->get_ms_to_os_transform();

#ifdef AN_ALLOW_EXTENDED_DEBUG
#ifndef AN_CLANG
	if (!_forDebug)
	{
		int catchMeHere = true;
	}
#endif
#endif

	// those are in current room
	Transform endPlacementOfOwnerWS;
	Transform endPlacementOfLegWS;
	calculate_end_placements(_instanceContext, REF_ endPlacementOfOwnerWS, REF_ endPlacementOfLegWS);

	if (is_flag_set(actionFlags, LegActionFlag::DropDown))
	{
		Transform endLegPlacementMS = legPlacementMS;
		// use just 30% of that movement - should be as parameter?
		endLegPlacementMS.set_translation(endLegPlacementMS.get_translation() + 0.3f * legPlacementVelocityMS * movement.get_time_left(gaitPlayback));
		// get z of leg placement from default location
		Vector3 endLegLocationMS = endLegPlacementMS.get_translation();
		endLegLocationMS.z = get_for_gait_instance(currentDefault)->calculate_placement_MS(_instanceContext).get_translation().z;
		endLegPlacementMS.set_translation(endLegLocationMS);
		// store as base
		endPlacementOfLegWS.set_translation(ms2ws.to_world(endLegPlacementMS).get_translation() * 0.7f + endPlacementOfLegWS.get_translation() * 0.3f);
	}
	else
#ifdef AN_DEVELOPMENT
		if (!_forDebug)
#endif
	{
		// store where should we be, where leg should be
		endTrajPlacementDoneForPredictedOwnerPlacement.set_placement_in_final_room(endPlacementOfOwnerWS, _instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
		endTrajPlacementDoneForPredictedLegPlacementBeforeTraces.set_placement_in_final_room(endPlacementOfLegWS, _instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
	}

	// do traces to find where leg should really go
	if (IModulesOwner const * modulesOwner = _instanceContext.get_owner()->get_owner())
	{
		ModulePresence const * modulePresence = modulesOwner->get_presence();
		ModuleMovement const * moduleMovement = modulesOwner->get_movement();

		// find owner placement WS
		Transform const ownerPlacementWS = modulePresence->get_placement();
		Vector3 const ownerUpWS = ownerPlacementWS.get_orientation().get_z_axis();

		// find starting point for traces
		Vector3 startTraceMS = Vector3::zero;
		if (boneForTraceStartIndex != NONE)
		{
			startTraceMS = _instanceContext.get_pose_LS()->get_bone_ms_from_ls(boneForTraceStartIndex).get_translation();
		}
		else
		{
			Vector3 const presenceCentreOS = modulePresence->get_centre_of_presence_os().get_translation();
			Vector3 const presenceCentreMS = ms2os.location_to_local(presenceCentreOS);
			startTraceMS = presenceCentreMS;
		}

		// store flags for later use
		Collision::Flags ikTraceFlags = Collision::DefinedFlags::get_ik_trace();
		Collision::Flags ikTraceRejectFlags = Collision::DefinedFlags::get_ik_trace_reject();
		if (auto const * mc = modulesOwner->get_collision())
		{
			ikTraceFlags = mc->get_ik_trace_flags();
			ikTraceRejectFlags = mc->get_ik_trace_reject_flags();
		}

		CheckCollisionContext checkCollisionContext;
		checkCollisionContext.collision_info_needed();
		checkCollisionContext.use_collision_flags(ikTraceFlags);

		if (instance->get_setup()->nonAlignedGravitySlideDotThreshold.is_set() &&
			moduleMovement && moduleMovement->get_use_gravity() > 0.0f)
		{
			checkCollisionContext.set_non_aligned_gravity_slide_dot_threshold(instance->get_setup()->nonAlignedGravitySlideDotThreshold.get());
		}

		// work on MS, go back to WS at the end of trace
		Transform endPlacementOfLegMS = ms2ws.to_local(endPlacementOfLegWS);

		Vector3 finalLocationWS;

		CollisionTrace collisionTrace(CollisionTraceInSpace::WS);
	
		{	// calculate locations for trace
			float startTraceAlt = startTraceMS.z;
			float endLocationOfLegAlt = endPlacementOfLegMS.get_translation().z;
			float useLegLength = legLength != 0.0f ? legLength : endPlacementOfLegMS.get_translation().length_2d();
			Vector3 upAxis = ms2ws.vector_to_world(Vector3::zAxis);
			Vector3 startTraceWSFlatOffset = ms2ws.vector_to_world(startTraceMS * Vector3(1.0f, 1.0f, 0.0f));
			Vector3 endLocationOfLegWSFlatOffset = ms2ws.vector_to_world(endPlacementOfLegMS.get_translation() * Vector3(1.0f, 1.0f, 0.0f));
			Vector3 objectPlacementWS = ownerPlacementWS.get_translation();
			collisionTrace.add_location(ms2ws.location_to_world(startTraceMS));
			collisionTrace.add_location(objectPlacementWS + startTraceWSFlatOffset * 0.3f + endLocationOfLegWSFlatOffset * 0.7f + upAxis * (endLocationOfLegAlt + (startTraceAlt - endLocationOfLegAlt) * 0.7f));
			collisionTrace.add_location(endPlacementOfLegWS.get_translation());
			collisionTrace.add_location(objectPlacementWS + startTraceWSFlatOffset * 0.3f + endLocationOfLegWSFlatOffset * 0.7f + upAxis * min(startTraceAlt - useLegLength * 0.7f, max(collisionTraceMinAlt, (endLocationOfLegAlt - (startTraceAlt - endLocationOfLegAlt) * 1.2f))));
			finalLocationWS = objectPlacementWS + startTraceWSFlatOffset * 0.7f + endLocationOfLegWSFlatOffset * 0.3f + upAxis * min(startTraceAlt - useLegLength * 0.9f, max(collisionTraceMinAlt, (endLocationOfLegAlt - (startTraceAlt - endLocationOfLegAlt) * 2.0f)));
			collisionTrace.add_location(finalLocationWS);
			if (instance->does_adjust_to_gravity())
			{
				todo_hack(TXT("hacks to make it not float in the air"));
				// catch wall in case we're climbing onto something
				finalLocationWS = ms2ws.location_to_local(finalLocationWS);
				finalLocationWS.x = -finalLocationWS.x * 1.5f;
				finalLocationWS.y = -finalLocationWS.y * 1.5f;
				finalLocationWS = ms2ws.location_to_world(finalLocationWS);
				collisionTrace.add_location(finalLocationWS);
			}
			collisionTrace.add_location(objectPlacementWS); // to allow hitting something, anything when coming back
		}

		int collisionTraceFlags = CollisionTraceFlag::ResultInPresenceRoom;

#ifdef AN_DEVELOPMENT
		if (_forDebug)
		{
			debug_filter(ac_walkers_hitLocations);
			debug_context(collisionTrace.get_start_in_room());
			debug_subject(_instanceContext.get_owner()->get_owner());
			collisionTrace.debug_draw();
			debug_no_context();
			CheckSegmentResult result;
			if (modulePresence->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext))
			{
				debug_context(result.inRoom);
				if (result.has_collision_flags(ikTraceRejectFlags, false))
				{
					debug_draw_sphere(true, false, Colour::red, 0.8f, Sphere(result.hitLocation, 0.01f));
				}
				else
				{
					debug_draw_sphere(true, false, Colour::green, 0.8f, Sphere(result.hitLocation, 0.01f));
				}
				debug_no_context();
			}
			debug_no_subject();
			debug_no_filter();
		}
		else
#endif
		{
			// clear now, as we will be adding to this one
			endTrajPlacementValid = false;
			endTrajPhysicalMaterial = nullptr;
			endTrajPlacement.clear_target();

			int triesLeft = 5;
			while (triesLeft)
			{
				CheckSegmentResult result;
				if (modulePresence->trace_collision(AgainstCollision::Movement, collisionTrace, result, collisionTraceFlags, checkCollisionContext, &endTrajPlacement))
				{
					float useGravityFromTraceDotLimit = 0.6f;
					float floorLevelOffset = 0.0f;
					bool allowUsingGravityFromOutboundTraces = false;
					if (moduleMovement)
					{
						useGravityFromTraceDotLimit = moduleMovement->find_use_gravity_from_trace_dot_limit_of_current_gait();
						floorLevelOffset = moduleMovement->find_floor_level_offset_of_current_gait();
						allowUsingGravityFromOutboundTraces = moduleMovement->find_does_allow_using_gravity_from_outbound_traces_of_current_gait();
					}
					// check if normal is valid, on option, if not valid, continue
					bool useTraceForGravity = Vector3::dot(result.hitNormal, ownerUpWS) > useGravityFromTraceDotLimit;
					if (!useTraceForGravity)
					{
						Vector3 const relativeHitLocation = (result.hitLocation - ownerPlacementWS.get_translation()).drop_using_normalised(ownerUpWS);
						Vector3 const hitNormal = result.hitNormal.drop_using_normalised(ownerUpWS);
						if (Vector3::dot(hitNormal, relativeHitLocation) > 0.0f)
						{
							useTraceForGravity = true;
						}
					}
					if (!useTraceForGravity ||
						result.has_collision_flags(ikTraceRejectFlags, false))
					{
						Transform intoRoomTransform = endTrajPlacement.calculate_final_room_transform();
						collisionTrace = CollisionTrace(endTrajPlacement.get_in_final_room(), intoRoomTransform);
						collisionTraceFlags |= CollisionTraceFlag::StartInProvidedRoom;

						// start at hit location
						collisionTrace.add_location(intoRoomTransform.location_to_world(result.hitLocation));
					
						// add point slightly off hit location, mark that first trace should not hit anything
						collisionTraceFlags |= CollisionTraceFlag::FirstTraceThroughDoorsOnly;
						collisionTrace.add_location(intoRoomTransform.location_to_world(result.hitLocation + result.hitNormal * 0.01f));
					
						/* this has to be rewritten
						Vector3 hitLocationMS = intoRoomTransform.location_to_local(result.hitLocation);
						Vector3 hitNormalMS = intoRoomTransform.vector_to_local(result.hitNormal);
						if (hitNormalMS.z >= 0.0f)
						{
							hitLocationMS += hitNormalMS * 0.01f;
							todo_note(TXT("drop it somehow"));
						}
						else
						{
							hitLocationMS.z = finalLocationMS.z;
						}
						// uncomment when drop is calculated properly collisionTrace.add_location(intoRoomTransform.location_to_world(ms2ws.location_to_world(hitLocationMS)));
						*/

						// add end location - same as was before
						collisionTrace.add_location(intoRoomTransform.location_to_world(finalLocationWS));
					}
					else
					{
						// if everything is ok...
						// have rotation same as in default placement
						Transform tracedEndPlacementOfLegMS = ms2os.to_local(endTrajPlacement.calculate_placement_in_os());
						Quat gaitLegOrientationMS = get_for_gait_instance(currentDefault)->calculate_placement_MS(_instanceContext).get_orientation();
						Quat legOrientationMS = gaitLegOrientationMS;

						// if we want to align to the floor, we should calculate the orientation using hit normal and then offset legOrientationMS (place in hit-normal's placement)
						float useHitNormalForPlacement = instance->get_setup()->useHitNormalForPlacement.get(1.0f);
						if (useHitNormalForPlacement > 0.0f)
						{
							Vector3 hitNormalWS = result.hitNormal;
							Vector3 hitNormalOS = ownerPlacementWS.vector_to_local(hitNormalWS);
							Vector3 hitNormalMS = ms2os.vector_to_local(hitNormalOS);
							Vector3 hitForwardMS = Vector3::yAxis;
							if (hitNormalMS.y > 0.7f) hitForwardMS = -Vector3::zAxis;
							if (hitNormalMS.y < -0.7f) hitForwardMS = Vector3::zAxis;
							Quat alignedOrientationMS = look_matrix33(hitForwardMS, hitNormalMS).to_quat().to_world(legOrientationMS);
							legOrientationMS = Quat::slerp(useHitNormalForPlacement, legOrientationMS, alignedOrientationMS);
						}

						// calculate rotation needed to go from result to adjusted orientation
						Quat rotateTracedToEnd = tracedEndPlacementOfLegMS.get_orientation().to_local(legOrientationMS);

						// and apply rotation
						Transform endPlacementWS = endTrajPlacement.get_placement_in_final_room();
						endPlacementWS.set_orientation(endPlacementWS.get_orientation().to_world(rotateTracedToEnd));
						endTrajPlacement.set_placement_in_final_room(endPlacementWS, endTrajPlacement.get_target());
						endTrajPlacementValid = true;
						endTrajPhysicalMaterial = PhysicalMaterial::get(result.material); // get default in worst case
						break;
					}
				}
				else
				{
					//todo_important(TXT("what now?"));
					endTrajPlacementValid = false;
					endTrajPhysicalMaterial = nullptr;
					endTrajPlacement.clear_target();
					endTrajPlacement.set_placement_in_final_room(endPlacementOfLegWS, _instanceContext.get_owner()->get_owner()->get_presence()->get_based_on());
					break;
				}
				--triesLeft;
			}

			endPlacementOfLegWS = ms2ws.to_world(endPlacementOfLegMS);
		}
	}

#ifdef AN_DEVELOPMENT
	if (!_forDebug)
#endif
	if (is_flag_set(actionFlags, LegActionFlag::DropDown))
	{
		if (currentDefault->dropDownDistanceForMaxTime != 0.0f)
		{
			// adjust time depending on distance
			float dropDownDistance = (ms2ws.to_world(legPlacementMS).get_translation() - endPlacementOfLegWS.get_translation()).length();
			float prop = clamp(dropDownDistance / currentDefault->dropDownDistanceForMaxTime, 0.0f, 1.0f);
			movement.endsAt = movement.startsAt + max(currentDefault->dropDownTime.min, currentDefault->dropDownTime.max * prop);
			movement.endPlacementAt = movement.endsAt + 0.1f;
		}
	}
}

//

InstanceSetup::InstanceSetup()
: alignWithGravityType(NAME(walker))
{

}

bool InstanceSetup::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	playbackRate.load_from_xml(_node, TXT("playbackRate"));
	predictionOffset.load_from_xml(_node, TXT("predictionOffset"));
	trajectoryObjectSize.load_from_xml(_node, TXT("trajectoryObjectSize"));
	disallowGaitChangingForTime.load_from_xml(_node, TXT("disallowGaitChangingForTime"));
	playbackSpeedUnitBlendTime.load_from_xml(_node, TXT("playbackSpeedUnitBlendTime"));
	matchGravityOrientationSpeed.load_from_xml(_node, TXT("matchGravityOrientationSpeed"));
	matchLocationXY.load_from_xml(_node, TXT("matchLocation"));
	matchLocationXY.load_from_xml(_node, TXT("matchLocationXY"));
	matchLocationZ.load_from_xml(_node, TXT("matchLocation"));
	matchLocationZ.load_from_xml(_node, TXT("matchLocationZ"));

	alignWithGravityType.load_from_xml(_node, TXT("alignWithGravity"));
	alignWithGravityType.load_from_xml(_node, TXT("alignWithGravityType"));

	useHitNormalForPlacement.load_from_xml(_node, TXT("useHitNormalForPlacement"));

	gravityPresenceTracesTouchSurroundingsRequired.load_from_xml(_node, TXT("gravityPresenceTracesTouchSurroundingsRequired"));

	allowAdjustments.load_from_xml(_node, TXT("allowAdjustments"));

	adjustmentTime.load_from_xml(_node, TXT("adjustmentTime"));
	disallowAdjustmentMovementForTime.load_from_xml(_node, TXT("disallowAdjustmentMovementForTime"));
	disallowAdjustmentMovementWhenCollidingForTime.load_from_xml(_node, TXT("disallowAdjustmentMovementWhenCollidingForTime"));
	adjustmentRequiredDistance.load_from_xml(_node, TXT("adjustmentRequiredDistance"));
	adjustmentRequiredVerticalDistance.load_from_xml(_node, TXT("adjustmentRequiredVerticalDistance"));
	adjustmentRequiredForwardAngle.load_from_xml(_node, TXT("adjustmentRequiredForwardAngle"));
	
	for_every(node, _node->children_named(TXT("whereIWillBe")))
	{
		whereIWillBe.useRelativeRequestedMovementDirection.load_from_xml(_node, TXT("useRelativeRequestedMovementDirection"));
		whereIWillBe.useLocomotionWhereIWantToBe.load_from_xml(_node, TXT("useLocomotionWhereIWantToBe"));
	}

	nonAlignedGravitySlideDotThreshold.load_from_xml(_node, TXT("nonAlignedGravitySlideDotThreshold"));

	return result;
}

bool InstanceSetup::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	return result;
}

void InstanceSetup::apply_mesh_gen_params(MeshGeneration::GenerationContext const & _context)
{
	playbackRate.fill_value_with(_context);
	predictionOffset.fill_value_with(_context);
	trajectoryObjectSize.fill_value_with(_context);
	disallowGaitChangingForTime.fill_value_with(_context);
	playbackSpeedUnitBlendTime.fill_value_with(_context);
	matchGravityOrientationSpeed.fill_value_with(_context);
	matchLocationXY.fill_value_with(_context);
	matchLocationZ.fill_value_with(_context);
	alignWithGravityType.fill_value_with(_context);
	gravityPresenceTracesTouchSurroundingsRequired.fill_value_with(_context);
	allowAdjustments.fill_value_with(_context);
	adjustmentTime.fill_value_with(_context);
	disallowAdjustmentMovementForTime.fill_value_with(_context);
	disallowAdjustmentMovementWhenCollidingForTime.fill_value_with(_context);
	adjustmentRequiredDistance.fill_value_with(_context);
	adjustmentRequiredVerticalDistance.fill_value_with(_context);
	adjustmentRequiredForwardAngle.fill_value_with(_context);
	nonAlignedGravitySlideDotThreshold.fill_value_with(_context);

	whereIWillBe.useRelativeRequestedMovementDirection.fill_value_with(_context);
	whereIWillBe.useLocomotionWhereIWantToBe.fill_value_with(_context);
}

//

Walkers::Instance::Instance()
: requested(nullptr)
, instanceSetup(nullptr)
, gaitChangingDisallowedForTime(0.0f)
, legAllowedToAdjust(0)
, adjustingLegsCount(0)
, adjustingLegsLimit(1)
, legsUpCount(0)
, legsUpBits(0)
{
}

void Walkers::Instance::mark_leg_up(int _legIndex)
{
	set_flag(legsUpBits, bit(_legIndex));
	count_legs_up();
}

void Walkers::Instance::mark_leg_down(int _legIndex)
{
	clear_flag(legsUpBits, bit(_legIndex));
	count_legs_up();
}

void Walkers::Instance::count_legs_up()
{
	legsUpCount = 0;
	for_count(int, legIdx, legs.get_size())
	{
		legsUpCount += is_flag_set(legsUpBits, bit(legIdx)) ? 1 : 0;
	}
}

void Walkers::Instance::setup(InstanceContext const & _instanceContext, int _legCount, InstanceSetup const & _instanceSetup)
{
	legs.set_size(_legCount);
	instanceSetup = &_instanceSetup;
	int legIndex = 0;
	for_every(leg, legs)
	{
		leg->initialise(_instanceContext, this, legIndex);
		++legIndex;
	}
	requested = nullptr;
	requestedAsOption = nullptr;
}

void Walkers::Instance::stop_sounds()
{
	for_every(leg, legs)
	{
		leg->stop_sounds();
	}
}

void Walkers::Instance::advance(InstanceContext const & _instanceContext, float _deltaTime)
{
#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("-- advance walkers instance ---"));
		}
	}
#endif

	gaitChangingDisallowedForTime = max(0.0f, gaitChangingDisallowedForTime - _deltaTime);

	if (requested != gaitPlayback.get_gait() || (requestedAsOption && requestedAsOption->reason != currentGaitReason))
	{
		switch_to(_instanceContext, requested, requestedAsOption);
	}

	// adjust playback speed
	float preferredPlaybackSpeed = calculate_preferred_gait_playback_speed(_instanceContext);
	if (gaitPlayback.get_playback_speed() != preferredPlaybackSpeed)
	{
		gaitPlayback.set_playback_speed(blend_to_using_speed_based_on_time(gaitPlayback.get_playback_speed(), preferredPlaybackSpeed, instanceSetup->playbackSpeedUnitBlendTime.get(), _deltaTime));
#ifdef AN_ALLOW_EXTENDED_DEBUG
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("changed gait playback speed to %.3f (wants %.3f)"), gaitPlayback.get_playback_speed(), preferredPlaybackSpeed);
		}
#endif
	}

	gaitPlayback.advance(_deltaTime);
#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("gait \"%S\" %.3f [%.3f]"), get_current_gait_name().to_char(), gaitPlayback.get_at_time(), gaitPlayback.get_playback_speed() * 100.0f);
		}
	}
#endif

	// update events
	events.advance(*this, _instanceContext, gaitPlayback.get_gait()? &gaitPlayback.get_gait()->get_events() : nullptr,
		gaitPlayback.get_prev_pt(),
		gaitPlayback.get_pt(), false, NP);

	if (!legs.is_empty())
	{
		legAllowedToAdjust = (legAllowedToAdjust + 1) % legs.get_size();
		for_every(leg, legs)
		{
			leg->advance(_instanceContext, _deltaTime);
		}
	}

	adjust_placement_to_leg_placement(_instanceContext, _deltaTime);
}

void Walkers::Instance::adjust_placement_to_leg_placement(InstanceContext const & _instanceContext, float _deltaTime)
{
	adjustsToGravity = false;
	shouldFall = false;
	preferredGravityAlignment.clear();
	if (!legs.is_empty())
	{
		an_assert(_instanceContext.get_owner() &&
			   _instanceContext.get_owner()->get_owner());
		float adjustToGravity = 1.0f;
		float matchGravityOrientationSpeed = 90.0f;
		float matchLocationXY = instanceSetup->matchLocationXY.get();
		float matchLocationZ = instanceSetup->matchLocationZ.get();
		ModuleMovement* movement = _instanceContext.get_owner()->get_owner()->get_movement();
		if (movement)
		{
			adjustToGravity = movement->get_align_with_gravity_weight(instanceSetup->alignWithGravityType.get());
			matchGravityOrientationSpeed = movement->get_match_gravity_orientation_speed();
		}
		if (instanceSetup->matchGravityOrientationSpeed.is_set())
		{
			matchGravityOrientationSpeed = instanceSetup->matchGravityOrientationSpeed.get();
		}
		if (adjustToGravity > 0.0f)
		{
			adjustsToGravity = true;

			an_assert(_instanceContext.get_owner()->get_owner()->get_presence());
			ModulePresence* presence = _instanceContext.get_owner()->get_owner()->get_presence();
			// debug_context(presence->get_in_room());

			ARRAY_STACK(Vector3, legsWS, legs.get_size());
			Vector3 centre = Vector3::zero;

			debug_filter(ac_walkers);
			debug_context(presence->get_in_room());
			debug_subject(_instanceContext.get_owner()->get_owner());

			int validCount = 0;
			// calculate centre and gather leg placements
			{
				Transform const ms2ws = _instanceContext.get_owner()->get_ms_to_ws_transform();
				float weightSum = 0.0f;
				for_every(leg, legs)
				{
					if (leg->is_end_traj_placement_valid() &&
						leg->is_active())
					{
						++validCount;

						//

						Vector3 legWS = ms2ws.location_to_world(leg->get_traj_placement_MS().get_translation());
						// !@# Vector3 legWS = os2ws.location_to_world(leg->calculate_end_traj_placement_os().get_translation());
						legsWS.push_back(legWS);
						centre += legWS;
						weightSum += 1.0f;
					}
				}
				centre = centre / weightSum;
			}

			Vector3 preferredGravityDir = Vector3::zero;

			// check hit locations against centre and upDir to determine what each location prefers as up dir
			if (!legsWS.is_empty())
			{
				Transform const placement = presence->get_placement();
				Vector3 upDir = placement.get_orientation().get_z_axis();
				float weightSum = 0.0f;
				debug_draw_sphere(true, false, Colour::orange, 0.8f, Sphere(centre, 0.005f));
				for_every(legWS, legsWS)
				{
					debug_draw_sphere(true, false, Colour::orange, 0.8f, Sphere(*legWS, 0.01f));
					debug_draw_line(true, Colour::orange, centre, *legWS);
					Vector3 offset = *legWS - centre;
					// debug_draw_line(true, Colour::cyan, centre, centre + offset);
					Vector3 const right = Vector3::cross(offset, upDir);
					Vector3 const gravityDir = Vector3::cross(offset, right).normal();
					preferredGravityDir += gravityDir;
					weightSum += 1.0f;
				}
				preferredGravityDir = (preferredGravityDir / weightSum).normal();
			}
			// debug_draw_line(true, Colour::green, centre, centre + preferredGravityDir);

			if (preferredGravityDir.is_zero() || !validCount)
			{
				if (! presence->has_teleported() && presence->does_allow_falling())
				{
					++shouldFallFrames;
					if (shouldFallFrames > 3)
					{
						shouldFall = true;
						todo_hack(TXT("hacks to make it not float in the air"));
						preferredGravityDir = -presence->get_environment_up_dir();
						presence->add_next_location_displacement(preferredGravityDir * max(1.0f, presence->get_gravity().length() * 0.25f) * min(0.05f, _deltaTime)); // pull down (but not too much in case we're rarely advancing)
					}
				}
			}
			else
			{
				shouldFallFrames = 0;
			}

			if (!shouldFall && preferredGravityDir.is_zero())
			{
				preferredGravityDir = -presence->get_placement().get_axis(Axis::Up);
			}

			debug_draw_arrow(true, Colour::cyan, presence->get_placement().get_translation(), presence->get_placement().get_translation() - preferredGravityDir * 0.2f);

			if (!preferredGravityDir.is_zero())
			{
				// calculate how to rotate to match that gravity
				Vector3 gravityLocal = presence->get_placement().vector_to_local(preferredGravityDir).normal();
				preferredGravityAlignment = Vector3::dot(gravityLocal, -Vector3::zAxis);
				Vector3 forwardFromGravityLocal = Vector3::yAxis;
				if (abs(gravityLocal.y) > 0.99f)
				{
					// we're looking right at gravity or away from it - simplify dirs!
					float dirMultiplier = gravityLocal.y > 0.0f ? 1.0f : -1.0f;
					forwardFromGravityLocal = Vector3::zAxis * dirMultiplier;
					gravityLocal = Vector3::yAxis * dirMultiplier;
				}
				else
				{
					// simplify vectors to get pitch from forwardFromGravityLocal and roll from gravityLocal
					forwardFromGravityLocal = Vector3::cross(gravityLocal, Vector3::cross(forwardFromGravityLocal, gravityLocal));
					forwardFromGravityLocal = Vector3(0.0f, forwardFromGravityLocal.y, forwardFromGravityLocal.z).normal();
					gravityLocal = Vector3(gravityLocal.x, 0.0f, gravityLocal.z).normal();
				}

				Rotator3 requiredToMatchGravity = Rotator3::zero;
				// only pitch and roll! keep yaw as it is
				requiredToMatchGravity.pitch -= asin_deg(clamp(-forwardFromGravityLocal.z, -1.0f, 1.0f));
				requiredToMatchGravity.roll -= asin_deg(clamp(gravityLocal.x, -1.0f, 1.0f));

				float const requiredToMatchGravityLength = requiredToMatchGravity.length();
				if (requiredToMatchGravityLength > 0.01f)
				{
					float const maxRotationSpeed = matchGravityOrientationSpeed;
					float const howMuchCanBeApplied = clamp(requiredToMatchGravityLength, -maxRotationSpeed * _deltaTime, maxRotationSpeed * _deltaTime);
					Rotator3 rotateThisFrame = requiredToMatchGravity * clamp(howMuchCanBeApplied / requiredToMatchGravityLength, 0.0f, 1.0f);
					if (movement)
					{
						movement->store_align_with_gravity_displacement(instanceSetup->alignWithGravityType.get(), rotateThisFrame); // do not multiply by adjustToGravity as it is weight
					}
					else
					{
						presence->add_next_orientation_displacement((rotateThisFrame * adjustToGravity).to_quat());
					}
				}
			}

			debug_no_subject();
			debug_no_context();
			debug_no_filter();
		}
		if (matchLocationXY > 0.0f ||
			matchLocationZ > 0.0f)
		{
			ModulePresence* presence = _instanceContext.get_owner()->get_owner()->get_presence();

			debug_filter(ac_walkers_hitLocations);
			debug_context(presence->get_in_room());
			debug_subject(_instanceContext.get_owner()->get_owner());

			// get requested location (in MS) according to where legs are placed
			Vector3 requestedLocationMS = Vector3::zero;
			int validCount = 0;
			{
#ifdef AN_DEBUG_RENDERER
				Transform const ms2ws = _instanceContext.get_owner()->get_ms_to_ws_transform();
#endif
				float weightSum = 0.0f;
				for_every(leg, legs)
				{
					if (leg->is_active())
					{
						// where relatively to default placementMS is current placement - we should try to match it
						Vector3 offsetMS = leg->get_placement_MS().get_translation() - leg->get_current_gait_instance()->calculate_placement_MS(_instanceContext).get_translation();
						requestedLocationMS += offsetMS;
						debug_draw_arrow(true, Colour::magenta.with_alpha(0.5f), ms2ws.location_to_world(leg->get_placement_MS().get_translation()), ms2ws.location_to_world(leg->get_placement_MS().get_translation() + offsetMS));
						weightSum += 1.0f;
						++validCount;
					}
				}
				requestedLocationMS /= weightSum;
			}

			if (validCount)
			{
				Vector3 requestedLocationOS = _instanceContext.get_owner()->get_ms_to_os_transform().vector_to_world(requestedLocationMS);
				debug_draw_arrow(true, Colour::magenta, presence->get_placement().get_translation(), presence->get_placement().get_translation() + presence->get_placement().vector_to_world(requestedLocationOS));

				Vector3 requestedLocationOSThisFrame = Vector3::zero;
				if (matchLocationXY > 0.0f)
				{
					float const howMuchCanBeApplied = min(_deltaTime, matchLocationXY);
					float const mul = howMuchCanBeApplied / matchLocationXY;
					requestedLocationOSThisFrame.x = requestedLocationOS.x * mul;
					requestedLocationOSThisFrame.y = requestedLocationOS.y * mul;
				}
				if (matchLocationZ > 0.0f)
				{
					float const howMuchCanBeApplied = min(_deltaTime, matchLocationZ);
					float const mul = howMuchCanBeApplied / matchLocationZ;
					requestedLocationOSThisFrame.z = requestedLocationOS.z * mul;
				}

				if (!requestedLocationOSThisFrame.is_zero())
				{
					presence->add_next_location_displacement(presence->get_placement().vector_to_world(requestedLocationOSThisFrame));
				}
			}

			debug_no_subject();
			debug_no_context();
			debug_no_filter();
		}
	}
}

void Walkers::Instance::switch_to(InstanceContext const & _instanceContext, Gait const * _newGait, GaitOption const * _asOption)
{
	if (gaitChangingDisallowedForTime > 0.0f)
	{
#ifdef AN_ALLOW_EXTENDED_DEBUG
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("would like to switch gait to %S but can't... waiting %.3f"), _newGait ? _newGait->get_name().to_char() : TXT("--"), gaitChangingDisallowedForTime);
		}
#endif
		// block for time being
		return;
	}
	gaitChangingDisallowedForTime = instanceSetup->disallowGaitChangingForTime.get();
	currentGaitReason = _asOption ? _asOption->reason : Name::invalid();
#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("switch gait to %S"), _newGait ? _newGait->get_name().to_char() : TXT("--"));
			if (_asOption)
			{
				if (_asOption->forceDropDown)
				{
					output(TXT("  force drop down"));
				}
			}
		}
	}
#endif
	gaitPlayback.switch_to(_newGait, _asOption);
	gaitPlayback.set_playback_speed(calculate_preferred_gait_playback_speed(_instanceContext));
#ifdef AN_ALLOW_EXTENDED_DEBUG
	{
		IF_EXTENDED_DEBUG(WalkersLegMovementContinuous)
		{
			output(TXT("new gait playback speed %.3f"), gaitPlayback.get_playback_speed());
		}
	}
#endif

	for_every(leg, legs)
	{
		leg->on_changed_gait(_instanceContext, _asOption ? _asOption->forceDropDown : false);
	}
}

float Walkers::Instance::calculate_preferred_gait_playback_speed(InstanceContext const & _instanceContext) const
{
	float resultPlaybackSpeed = 0.0f;

	if (Gait const * gait = gaitPlayback.get_gait())
	{
		if (_instanceContext.get_speed_XY().is_set())
		{
			// move speedXY to gait's space
			float const speedXY = _instanceContext.get_speed_XY().get() * (gait->get_trajectory_object_size() / get_trajectory_object_size()) * get_playback_rate();
			Range const playbackSpeedRange = gait->get_playback_speed_range();
			float const speedXYforNormalPlaybackSpeed = gait->get_speed_XY_for_normal_playback_speed();
			if (speedXYforNormalPlaybackSpeed != 0.0f)
			{
				if (gait->get_speeds_XY_for_playback_speed_range().is_set())
				{
					// use extremes
					Range const speedsXYforPlaybackSpeedRange = gait->get_speeds_XY_for_playback_speed_range().get();
					if (speedXY >= speedXYforNormalPlaybackSpeed)
					{
						float const pt = clamp(Range(speedXYforNormalPlaybackSpeed, speedsXYforPlaybackSpeedRange.max).get_pt_from_value(speedXY), 0.0f, 1.0f);

						resultPlaybackSpeed = max(resultPlaybackSpeed, Range(1.0f, playbackSpeedRange.max).get_at(pt));
					}
					else
					{
						float const pt = clamp(Range(speedsXYforPlaybackSpeedRange.min, speedXYforNormalPlaybackSpeed).get_pt_from_value(speedXY), 0.0f, 1.0f);

						resultPlaybackSpeed = max(resultPlaybackSpeed, Range(playbackSpeedRange.min, 1.0f).get_at(pt));
					}
				}
				else
				{
					resultPlaybackSpeed = max(resultPlaybackSpeed, playbackSpeedRange.clamp(speedXY / speedXYforNormalPlaybackSpeed));
				}
			}
			else
			{
				// maybe just extremes?
				if (gait->get_speeds_XY_for_playback_speed_range().is_set())
				{
					float const pt = clamp(gait->get_speeds_XY_for_playback_speed_range().get().get_pt_from_value(speedXY), 0.0f, 1.0f);

					resultPlaybackSpeed = max(resultPlaybackSpeed, playbackSpeedRange.get_at(pt));
				}
			}
		}

		if (_instanceContext.get_velocity_orientation().is_set())
		{
			float const turnSpeedYaw = abs(_instanceContext.get_velocity_orientation().get().yaw) * get_playback_rate();
			Range playbackSpeedRange;
			Range turnSpeedRange;
			if (gait->get_turn_speeds_yaw_and_playback_speed_ranges(turnSpeedRange, playbackSpeedRange))
			{
				float const pt = clamp(turnSpeedRange.get_pt_from_value(turnSpeedYaw), 0.0f, 1.0f);

				resultPlaybackSpeed = max(resultPlaybackSpeed, playbackSpeedRange.get_at(pt));
			}
		}
	}

	return resultPlaybackSpeed != 0.0f ? resultPlaybackSpeed : 1.0f;
}
