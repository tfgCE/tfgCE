#pragma once

#include "..\math\math.h"
#include "..\types\optional.h"

namespace Collision
{
	struct CheckSegmentResult;

	namespace ProvidesGravity
	{
		enum Type
		{
			None,
			Dir,
			AsNormal,
			JustForce
		};
	};

	struct CollisionInfoProvider
	{
	public:
		CollisionInfoProvider();

		void clear();

		bool load_from_xml(IO::XML::Node const * _node, tchar const * _nodeName = TXT(""));

		void apply_to(REF_ CheckSegmentResult & _result) const; // doesn't clear if wasn't set
		void apply_gravity_to(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _gravityForce) const; // doesn't clear if wasn't set

		void clear_gravity();
		void set_zero_gravity();
		void set_gravity_dir_based(Vector3 const & _dir, float _force);

		void fill_missing_with(CollisionInfoProvider const & _source);
		void override_with(CollisionInfoProvider const & _source);

		// start with the most important/highest priority and fill the rest
		void apply_kill_gravity_distance_to(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _killGD, REF_ Optional<Name>& _killGravityAnchorPOI, REF_ Optional<Name>& _killGravityAnchorParam, REF_ Optional<Name>& _killGravityOverrideAnchorPOI, REF_ Optional<Name>& _killGravityOverrideAnchorParam) const;
		void set_kill_gravity_distance(float _distance);
		void set_no_kill_gravity_distance();

	private:
		Optional<ProvidesGravity::Type> providesGravity; // if none, there's no gravity - dir is set to 0, force to 0, if not set, doesn't provide gravity at all
		Vector3 gravityDir; // if provides
		Optional<float> gravityForce;

		Optional<bool> isWalkable;

		Optional<float> killGravityDistance; // in direction of gravity
		Optional<Name> killGravityAnchorPOI; // if set, this is point 0 and distance is just offset
		Optional<Name> killGravityOverrideAnchorPOI; // if set, this is point 0 and distance is just offset
		Optional<Name> killGravityAnchorParam; // if set, this is point 0 and distance is just offset
		
		Optional<Name> killGravityOverrideAnchorParam; // overrides above setup
	};

};

