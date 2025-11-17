#include "collisionInfoProvider.h"

#include "shape.h"

#include "checkSegmentResult.h"

using namespace Collision;

CollisionInfoProvider::CollisionInfoProvider()
{
	clear();
}

void CollisionInfoProvider::clear()
{
	providesGravity.clear();
	gravityDir = Vector3::zero;
	gravityForce.unset_with(0.0f);
	isWalkable.unset_with(true);
}

bool CollisionInfoProvider::load_from_xml(IO::XML::Node const * _node, tchar const * _nodeName)
{
	if (tstrlen(_nodeName) > 0)
	{
		_node = _node->first_child_named(_nodeName);
	}
	if (!_node)
	{
		// may be not present, but that's ok
		return true;
	}
	bool result = true;
	if (IO::XML::Node const * gravityNode = _node->first_child_named(TXT("gravity")))
	{
		bool gravityForceRead = false;
		if (IO::XML::Node const * gravityForceNode = gravityNode->first_child_named(TXT("force")))
		{
			gravityForceRead = true;
			result &= gravityForce.load_from_xml(gravityForceNode);
		}
		if (IO::XML::Node const * gravityDirNode = gravityNode->first_child_named(TXT("dir")))
		{
			providesGravity.set(ProvidesGravity::Dir);
			result &= gravityDir.load_from_xml(gravityDirNode);
			gravityDir.normalise();
		}
		if (IO::XML::Node const * gravityAsNormalNode = gravityNode->first_child_named(TXT("asNormal")))
		{
			providesGravity.set(ProvidesGravity::AsNormal);
		}
		if (gravityForceRead && !providesGravity.is_set())
		{
			providesGravity.set(ProvidesGravity::JustForce);
		}
		killGravityDistance.load_from_xml(gravityNode, TXT("killDistance"));
		killGravityAnchorPOI.load_from_xml(gravityNode, TXT("killDistanceAnchorPOI"));
		killGravityAnchorParam.load_from_xml(gravityNode, TXT("killDistanceAnchorParam"));
		killGravityOverrideAnchorPOI.load_from_xml(gravityNode, TXT("killDistanceOverrideAnchorPOI"));
		killGravityOverrideAnchorParam.load_from_xml(gravityNode, TXT("killDistanceOverrideAnchorParam"));
		bool noKillGravityAnchor = gravityNode->get_bool_attribute_or_from_child_presence(TXT("noKillGravityAnchor"), false);
		error_loading_xml_on_assert(!killGravityDistance.is_set() || (killGravityAnchorPOI.is_set() || killGravityAnchorParam.is_set() || noKillGravityAnchor), result, gravityNode, TXT("\"killDistance\" provided but neither \"killDistanceAnchorPOI\", \"killDistanceAnchorParam\" nor \"noKillGravityAnchor\" is"));
	}
	if (IO::XML::Node const * walkableNode = _node->first_child_named(TXT("walkable")))
	{
		isWalkable.set(true);
	}
	if (IO::XML::Node const * walkableNode = _node->first_child_named(TXT("notWalkable")))
	{
		isWalkable.set(false);
	}
	return result;
}

void CollisionInfoProvider::apply_to(REF_ CheckSegmentResult & _result) const
{
	if (!_result.gravityDir.is_set() && providesGravity.is_set())
	{
		if (providesGravity.get() == ProvidesGravity::Dir)
		{
			_result.gravityDir.set(gravityDir);
			an_assert(_result.gravityDir.get().is_normalised());
		}
		else if (providesGravity.get() == ProvidesGravity::AsNormal)
		{
			_result.gravityDir.set(-_result.hitNormal);
			an_assert(_result.gravityDir.get().is_normalised());
		}
		else if (providesGravity.get() == ProvidesGravity::None)
		{
			_result.gravityDir.set(Vector3::zero);
		}
	}
	if (! _result.gravityForce.is_set())
	{
		if (providesGravity.is_set() && providesGravity.get() == ProvidesGravity::None)
		{
			_result.gravityForce.set(0.0f);
		}
		else if (gravityForce.is_set())
		{
			_result.gravityForce.set(gravityForce.get());
		}
	}
	if (!_result.isWalkable.is_set() && isWalkable.is_set())
	{
		_result.isWalkable.set(isWalkable.get());
	}
}

void CollisionInfoProvider::apply_gravity_to(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _gravityForce) const
{
	if (!_gravityDir.is_set() && providesGravity.is_set())
	{
		if (providesGravity.get() == ProvidesGravity::Dir)
		{
			_gravityDir.set(gravityDir);
			an_assert(_gravityDir.get().is_normalised());
		}
		else if (providesGravity.get() == ProvidesGravity::AsNormal)
		{
			an_assert(TXT("can't provide \"as normal\""));
		}
		else if (providesGravity.get() == ProvidesGravity::None)
		{
			_gravityDir.set(Vector3::zero);
		}
	}
	if (!_gravityForce.is_set())
	{
		if (providesGravity.is_set() && providesGravity.get() == ProvidesGravity::None)
		{
			_gravityForce.set(0.0f);
		}
		else if (gravityForce.is_set())
		{
			_gravityForce.set(gravityForce.get());
		}
	}
}

void CollisionInfoProvider::clear_gravity()
{
	providesGravity.clear();
	gravityDir = Vector3::zero;
	gravityForce.clear();
}

void CollisionInfoProvider::set_zero_gravity()
{
	providesGravity.set(ProvidesGravity::None);
	gravityDir = Vector3::zero;
	gravityForce.clear();
}

void CollisionInfoProvider::set_gravity_dir_based(Vector3 const & _dir, float _force)
{
	providesGravity.set(ProvidesGravity::Dir);
	an_assert(_dir.is_normalised());
	gravityDir = _dir;
	gravityForce.set(_force);
}

void CollisionInfoProvider::fill_missing_with(CollisionInfoProvider const & _source)
{
	if (!providesGravity.is_set() && _source.providesGravity.is_set())
	{
		providesGravity.set(_source.providesGravity.get());
		an_assert(_source.gravityDir.is_normalised());
		gravityDir = _source.gravityDir;
	}
	if (!gravityForce.is_set() && _source.gravityForce.is_set())
	{
		gravityForce.set(_source.gravityForce.get());
	}
	if (!isWalkable.is_set() && _source.isWalkable.is_set())
	{
		isWalkable.set(_source.isWalkable.get());
	}
}

void CollisionInfoProvider::override_with(CollisionInfoProvider const & _source)
{
	if (_source.providesGravity.is_set())
	{
		providesGravity.set(_source.providesGravity.get());
		an_assert(_source.gravityDir.is_normalised());
		gravityDir = _source.gravityDir;
	}
	if (_source.gravityForce.is_set())
	{
		gravityForce.set(_source.gravityForce.get());
	}
	if (_source.isWalkable.is_set())
	{
		isWalkable.set(_source.isWalkable.get());
	}
}

void CollisionInfoProvider::apply_kill_gravity_distance_to(REF_ Optional<Vector3> & _gravityDir, REF_ Optional<float> & _killGD, REF_ Optional<Name> & _killGravityAnchorPOI, REF_ Optional<Name> & _killGravityAnchorParam, REF_ Optional<Name>& _killGravityOverrideAnchorPOI, REF_ Optional<Name>& _killGravityOverrideAnchorParam) const
{
	Optional<float> gravityForce = 0.0f; // does not matter
	apply_gravity_to(_gravityDir, gravityForce);
	if (!_killGD.is_set() && killGravityDistance.is_set())
	{
		_killGD = killGravityDistance.get();
	}
	if (! _killGravityAnchorPOI.is_set() && killGravityAnchorPOI.is_set())
	{
		_killGravityAnchorPOI = killGravityAnchorPOI.get();
	}
	if (! _killGravityAnchorParam.is_set() && killGravityAnchorParam.is_set())
	{
		_killGravityAnchorParam = killGravityAnchorParam.get();
	}
	if (! _killGravityOverrideAnchorPOI.is_set() && killGravityOverrideAnchorPOI.is_set())
	{
		_killGravityOverrideAnchorPOI = killGravityOverrideAnchorPOI.get();
	}
	if (!_killGravityOverrideAnchorParam.is_set() && killGravityOverrideAnchorParam.is_set())
	{
		_killGravityOverrideAnchorParam = killGravityOverrideAnchorParam.get();
	}
}

void CollisionInfoProvider::set_kill_gravity_distance(float _distance)
{
	killGravityDistance = _distance;
}

void CollisionInfoProvider::set_no_kill_gravity_distance()
{
	killGravityDistance.clear();
}
