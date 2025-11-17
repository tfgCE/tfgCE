#include "animationSet.h"

#include "pose.h"
#include "skeleton.h"

#include "..\math\math.h"
#include "..\other\typeConversions.h"

#include "..\serialisation\serialiser.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Meshes;

//

SIMPLE_SERIALISER_AS_INT32(::Meshes::AnimationTrack::Type);

//

Importer<AnimationSet, AnimationSetImportOptions> AnimationSet::s_importer;

void AnimationSet::initialise_static()
{
}

void AnimationSet::close_static()
{
}

AnimationSet* AnimationSet::create_new()
{
	return new AnimationSet();
}

int AnimationSet::find_animation_index(Name const& _id) const
{
	for_every(a, animations)
	{
		if (a->id == _id)
		{
			return for_everys_index(a);
		}
	}
	return NONE;
}

int AnimationSet::find_track_idx(Name const& _id) const
{
	for_every(t, setup.tracks)
	{
		if (t->id == _id)
		{
			return for_everys_index(t);
		}
	}
	return NONE;
}

AnimationTrack const& AnimationSet::get_track(int _idx) const
{
	an_assert(setup.tracks.is_index_valid(_idx));
	return setup.tracks[_idx];
}

void AnimationSet::log(LogInfoContext& _context) const
{
	_context.log(TXT("animations"));
	{
		LOG_INDENT(_context);
		for_every(a, animations)
		{
			_context.log(TXT("%S [%i:%.3f] avgRMspeed"), a->id.to_char(), a->frameCount, a->timeLength, a->averageRootMotionSpeed);
		}
	}
}

void AnimationSet::post_import()
{
	prepare_to_use();
	for_every(a, animations)
	{
		a->post_import();
	}
}

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool AnimationSet::serialise(Serialiser& _serialiser)
{
	version = CURRENT_VERSION;
	bool result = SerialisableResource::serialise(_serialiser);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid animation set version"));
		return false;
	}
	result &= serialise_data(_serialiser, setup);
	result &= serialise_data(_serialiser, animations);
	{
		int frameDataCount = 0;
		if (_serialiser.is_reading())
		{
			setup.prepare_to_use(); // we need frame stride
			serialise_data(_serialiser, frameDataCount);
			animationData.set_size(setup.get_frame_stride() * frameDataCount);
		}
		else
		{
			frameDataCount = animationData.get_size() / setup.get_frame_stride();
			serialise_data(_serialiser, frameDataCount);
		}

		for_count(int, frameIdx, frameDataCount)
		{
			byte* frameAt = &animationData[frameIdx * setup.get_frame_stride()];
			for_every(t, setup.tracks)
			{
				void* data = (void*)(frameAt + t->offset);
				if (t->type == AnimationTrack::Type::RootMotion)
				{
					result &= ((Transform*)data)->serialise(_serialiser);
				}
				if (t->type == AnimationTrack::Type::Transform)
				{
					result &= ((Transform*)data)->serialise(_serialiser);
				}
				if (t->type == AnimationTrack::Type::Quat)
				{
					result &= ((Quat*)data)->serialise(_serialiser);
				}
				if (t->type == AnimationTrack::Type::Float)
				{
					float f;
					if (_serialiser.is_writing())
					{
						f = *(float*)data;
					}
					result &= serialise_data(_serialiser, f);
					if (_serialiser.is_reading())
					{
						*(float*)data = f;
					}
				}
			}
		}
	}
	if (_serialiser.is_reading())
	{
		prepare_to_use();
	}
	return result;
}

#undef CURRENT_VERSION
#undef VER_BASE

void AnimationSet::prepare_to_use()
{
	setup.prepare_to_use();

	for_every(a, animations)
	{
		a->animationSet = this;
	}
}

void AnimationSet::clear()
{
	setup.clear();
	animations.clear();
	animationData.clear();
}

int AnimationSet::add_root_motion_track(Name const & _bone)
{
	if (setup.tracks.is_empty())
	{
		setup.tracks.push_back();
		auto& t = setup.tracks.get_last();
		t.type = AnimationTrack::Type::RootMotion;
		t.id = _bone;
		return setup.tracks.get_size() - 1;
	}
	return NONE;
}

int AnimationSet::add_track(AnimationTrack::Type _type, Name const& _id)
{
	setup.tracks.push_back();
	auto& t = setup.tracks.get_last();
	t.type = _type;
	t.id = _id;
	return setup.tracks.get_size() - 1;
}

int AnimationSet::add_animation(Name const& _id, int _frameCount, float _timeLength)
{
	an_assert(setup.get_frame_stride() != 0);
	animations.push_back();
	auto& a = animations.get_last();
	a.id = _id;
	a.dataIndex = animationData.get_size();
	a.frameCount = _frameCount;
	a.timeLength = _timeLength;
	int addSize = setup.get_frame_stride() * _frameCount;
	animationData.set_size(animationData.get_size() + addSize);

	for_count(int, frameIdx, _frameCount)
	{
		byte* data = &animationData[a.dataIndex];
		memory_zero(data, addSize);
		for_every(t, setup.tracks)
		{
			if (t->type == AnimationTrack::Type::RootMotion ||
				t->type == AnimationTrack::Type::Transform)
			{
				*((Transform*)data) = Transform::identity;
			}
			if (t->type == AnimationTrack::Type::Quat)
			{
				*((Quat*)data) = Quat::identity;
			}
			if (t->type == AnimationTrack::Type::Float)
			{
				*((float*)data) = 0.0f;
			}
		}
	}

	return animations.get_size() - 1;
}

void AnimationSet::set_data(int _animationIdx, int _trackIdx, int _frameIdx, int _frameCount, void const* _data)
{
	an_assert(animations.is_index_valid(_animationIdx));
	auto& a = animations[_animationIdx];

	an_assert(_frameIdx >= 0 && _frameIdx < a.frameCount);
	an_assert(setup.tracks.is_index_valid(_trackIdx));

	byte* dataDest = &animationData[a.dataIndex];
	dataDest += _frameIdx * setup.get_frame_stride();
	dataDest += setup.tracks[_trackIdx].offset;

	byte const * dataSrc = (byte const*)_data;
	auto& t = setup.tracks[_trackIdx];

	for_count(int, frameI, _frameCount)
	{
		int datumSize = 0;
		if (t.type == AnimationTrack::Type::RootMotion ||
			t.type == AnimationTrack::Type::Transform)
		{
			datumSize = sizeof(Transform);
		}
		if (t.type == AnimationTrack::Type::Quat)
		{
			datumSize = sizeof(Quat);
		}
		if (t.type == AnimationTrack::Type::Float)
		{
			datumSize = sizeof(float);
		}
		if (datumSize)
		{
			memory_copy(dataDest, dataSrc, datumSize);
		}
		dataDest += setup.get_frame_stride();
		dataSrc += datumSize;
	}
}

//--

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool Animation::serialise(Serialiser& _serialiser)
{
	uint8 version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid animation version"));
		return false;
	}
	bool result = true;
	result &= serialise_data(_serialiser, id);
	result &= serialise_data(_serialiser, dataIndex);
	result &= serialise_data(_serialiser, frameCount);
	result &= serialise_data(_serialiser, timeLength);
	result &= serialise_data(_serialiser, averageRootMotionSpeed);
	return result;
}

#undef CURRENT_VERSION
#undef VER_BASE

void Animation::post_import()
{
	averageRootMotionSpeed = 0.0f;
	if (frameCount > 1 &&
		!animationSet->setup.tracks.is_empty())
	{
		auto* track = animationSet->setup.tracks.begin();
		if (track->type == AnimationTrack::Type::RootMotion)
		{
			float rootMotionSpeed = 0.0f;
			float frameTime = timeLength / (float)(frameCount - 1);
			float invFrameTime = frameTime != 0.0f ? 1.0f / frameTime : 0.0f;
			int frameStride = animationSet->setup.get_frame_stride();
			byte const* frameData = &animationSet->animationData[dataIndex];
			for_count(int, frameIdx, frameCount)
			{
				rootMotionSpeed += ((Transform*)frameData)->get_translation().length() * invFrameTime;
				frameData += frameStride;
			}
			averageRootMotionSpeed = rootMotionSpeed / (float)(frameCount - 1);
		}
	}
}

void Animation::read(float _time, OUT_ Transform& _rootMotion, OUT_ Pose* _intoPose) const
{
	float frameTime = _time * (float)(frameCount - 1) / timeLength;

	int frameLo, frameHi;
	float framePt;

	if (frameCount > 1)
	{
		frameLo = TypeConversions::Normal::f_i_cells(frameTime);
		framePt = frameTime - (float)frameLo;
		frameLo = mod(frameLo, frameCount);
		frameHi = mod(frameLo + 1, frameCount);
	}
	else
	{
		frameLo = frameHi = 0;
		framePt = 0.0f;
	}

	_rootMotion = Transform::identity;

	if (auto* skeleton = _intoPose->get_skeleton())
	{
		int frameStride = animationSet->setup.get_frame_stride();
		byte const* frameLoData = &animationSet->animationData[dataIndex + frameLo * frameStride];
		byte const* frameHiData = &animationSet->animationData[dataIndex + frameHi * frameStride];
		auto* outPose = _intoPose->access_bones().begin();
		auto* defPose = skeleton->get_bones_default_placement_LS().begin();
		auto* track = animationSet->setup.tracks.begin();
		auto* trackEnd = animationSet->setup.tracks.end();
		an_assert(track != trackEnd);
		if (track->type == AnimationTrack::Type::RootMotion)
		{
			_rootMotion = lerp(framePt, *(Transform*)frameLoData, *(Transform*)frameHiData);
		}
		for_every(b, skeleton->get_bones())
		{
			*outPose = *defPose;
			auto const& bName = b->get_name();
			while (track != trackEnd)
			{
				if (track->id == bName)
				{
					while (track != trackEnd &&
						track->id == bName)
					{
						if (track->type == AnimationTrack::Type::Transform)
						{
							*outPose = lerp(framePt, *(Transform*)frameLoData, *(Transform*)frameHiData);
						}
						if (track->type == AnimationTrack::Type::Quat)
						{
							outPose->set_orientation(lerp(framePt, *(Quat*)frameLoData, *(Quat*)frameHiData));
						}
						frameLoData += track->stride;
						frameHiData += track->stride;
						++track;
					}
					break;
				}
				else
				{
					frameLoData += track->stride;
					frameHiData += track->stride;
					++track;
				}

			}
			if (track == trackEnd)
			{
				break;
			}
			++outPose;
			++defPose;
		}
	}
}

void Animation::read_track(float _time, Name const& _trackId, void* _into) const
{
	todo_implement;
}

//--

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool AnimationTrack::serialise(Serialiser& _serialiser)
{
	uint8 version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid animation track version"));
		return false;
	}
	bool result = true;
	result &= serialise_data(_serialiser, id);
	result &= serialise_data(_serialiser, type);
	return result;
}

#undef CURRENT_VERSION
#undef VER_BASE

//--

#define VER_BASE 0
#define CURRENT_VERSION VER_BASE

bool AnimationSetSetup::serialise(Serialiser& _serialiser)
{
	uint8 version = CURRENT_VERSION;
	serialise_data(_serialiser, version);
	if (version > CURRENT_VERSION)
	{
		error(TXT("invalid animation track version"));
		return false;
	}
	bool result = true;
	result &= serialise_data(_serialiser, tracks);
	return result;
}

#undef CURRENT_VERSION
#undef VER_BASE

void AnimationSetSetup::clear()
{
	frameStride = 0;
	tracks.clear();
}

void AnimationSetSetup::prepare_to_use()
{
	frameStride = 0;
	for_every(t, tracks)
	{
		t->offset = frameStride;
		if (t->type == AnimationTrack::Type::RootMotion)
		{
			t->stride = sizeof(Transform);
		}
		if (t->type == AnimationTrack::Type::Transform)
		{
			t->stride = sizeof(Transform);
		}
		if (t->type == AnimationTrack::Type::Quat)
		{
			t->stride = sizeof(Quat);
		}
		if (t->type == AnimationTrack::Type::Float)
		{
			t->stride = sizeof(float);
		}
		frameStride += t->stride;
	}
}

//--

bool AnimationSetImportOptions::load_from_xml(IO::XML::Node const* _node)
{
	bool result = base::load_from_xml(_node);

	importRootMotionFromNode = _node->get_string_attribute(TXT("importRootMotionFromNode"), importRootMotionFromNode);
	importRemovePrefixFromAnimationNames = _node->get_string_attribute(TXT("importRemovePrefixFromAnimationNames"), importRemovePrefixFromAnimationNames);
	importRotationOnly = _node->get_bool_attribute(TXT("importRotationOnly"), importRotationOnly);
	importFramesPerSecond = _node->get_float_attribute(TXT("importFramesPerSecond"), importFramesPerSecond);

	for_every(node, _node->children_named(TXT("importAnimationDetails")))
	{
		for_every(aNode, node->children_named(TXT("animation")))
		{
			animationDetails.push_back();
			auto& ad = animationDetails.get_last();
			if (ad.load_from_xml(aNode))
			{
				// ok
			}
			else
			{
				animationDetails.pop_back();
				result = false;
			}
		}
	}

	return result;
}

//--

bool AnimationSetImportOptions::AnimationDetails::load_from_xml(IO::XML::Node const* _node)
{
	bool result = true;

	id = _node->get_string_attribute(TXT("id"), id);
	importAs = _node->get_string_attribute(TXT("importAs"), importAs);
	time.load_from_attribute_or_from_child(_node, TXT("time"));
	if (time.min == time.max)
	{
		time.min = 0.0f;
	}
	framesPerSecond = _node->get_float_attribute(TXT("framesPerSecond"), framesPerSecond);

	return result;
}
