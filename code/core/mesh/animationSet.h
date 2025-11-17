#pragma once

#include "skeleton.h"

#include "..\containers\array.h"
#include "..\types\name.h"

#include "..\serialisation\importer.h"
#include "..\serialisation\serialisableResource.h"
#include "..\serialisation\serialiserBasics.h"

struct Transform;

namespace Meshes
{
	struct Animation;
	struct AnimationSet;
	struct AnimationSetSetup;
	struct Pose;

	struct AnimationSetImportOptions
	: public SkeletonImportOptions
	{
		typedef SkeletonImportOptions base;

		String importRootMotionFromNode;
		String importRemovePrefixFromAnimationNames;
		bool importRotationOnly = false;
		float importFramesPerSecond = 0.0f;

		struct AnimationDetails
		{
			String id;
			String importAs;
			Range time = Range::empty;
			float framesPerSecond = 0.0f;

			bool load_from_xml(IO::XML::Node const* _node);
		};
		Array<AnimationDetails> animationDetails;

		AnimationSetImportOptions() {}

		bool load_from_xml(IO::XML::Node const* _node);
	};

	/*
	 *	Animation is just an information about where in the anim set does the animation is placed
	 */
	struct Animation
	{
		void read(float _time, OUT_ Transform& _rootMotion, OUT_ Pose* _intoPose) const;

		void read_track(float _time, Name const & _trackId, OUT_ void* _into) const;

		void post_import();

		bool serialise(Serialiser& _serialiser);

	public:
		float get_length() const { return timeLength; }
		float get_average_root_motion_speed() const { return averageRootMotionSpeed; }

	private:
		Name id;
		AnimationSet* animationSet = nullptr;
		int dataIndex = 0; // where does the animation start in the dataset

		int frameCount = 1; // if looped the last frame should be identical to the first one, we don't blend between the last frame and the first frame (!)
		float timeLength = 0.0f;

		float averageRootMotionSpeed = 0.0f;

		friend struct AnimationSet;
	};

	struct AnimationTrack
	{
		enum Type
		{
			RootMotion,
			Transform,
			Quat,
			Float
		};

		bool serialise(Serialiser& _serialiser);

		Name const& get_id() const { return id; }
		Type get_type() const { return type; }

	private:
		Name id; // bone name or value name
		Type type = AnimationTrack::Transform;
		CACHED_ int offset = 0;
		CACHED_ int stride = 0;

		friend struct Animation;
		friend struct AnimationSet;
		friend struct AnimationSetSetup;
	};

	struct AnimationSetSetup
	{
		Array<AnimationTrack> tracks;

		bool serialise(Serialiser& _serialiser);

		int get_frame_stride() const { return frameStride; }

	private:
		CACHED_ int frameStride;

		void clear();

		void prepare_to_use();

		friend struct AnimationSet;
	};

	struct AnimationSet
	: public SerialisableResource
	{
	public:
		static Importer<AnimationSet, AnimationSetImportOptions>& importer() { return s_importer; }

		static void initialise_static();
		static void close_static();

		static AnimationSet* create_new();

	public:
		Animation const& get_animation(int _index) const { an_assert(animations.is_index_valid(_index)); return animations[_index]; }
		int find_animation_index(Name const & _id) const;
		int find_track_idx(Name const & _id) const;
		AnimationTrack const & get_track(int _idx) const;

		void log(LogInfoContext& _context) const;

	public: // create
		void clear();
		int add_root_motion_track(Name const& _bone); // has to be first
		int add_track(AnimationTrack::Type _type, Name const& _id);
		void prepare_to_use();
		int add_animation(Name const& _id, int _frameCount, float _timeLength);
		void set_data(int _animationIdx, int _trackIdx, int _frameIdx, int _frameCount, void const* _data);
		void post_import();

	public: // SerialisableResource
		virtual bool serialise(Serialiser& _serialiser);

	private:
		static Importer<AnimationSet, AnimationSetImportOptions> s_importer;

		AnimationSetSetup setup;
		Array<byte> animationData; // continuous animation data, broken into animations
		Array<Animation> animations; // all the animations

		friend struct Animation;
	};

};

DECLARE_SERIALISER_FOR(::Meshes::AnimationTrack::Type);

