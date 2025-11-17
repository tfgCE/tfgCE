#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

class CoreRegisteredTypes;

/**
 *		For transform A and B:
 *			AinB = B.to_local(A);
 *		Means that B is world, A is local.
 *		Then we have:
 *			inB = AinB.to_world(inA);
 *			inA = AinB.to_local(inB);
 */
struct Transform
{
public:
	static Transform const zero;
	static Transform const identity;
	static Transform const do180;

	inline Transform();
	inline Transform(Transform const & _other);
	inline explicit Transform(Vector3 _translation, Quat _orientation, float _scale = 1.0f);
	inline Transform& operator=(Transform const & _other);

	inline static Transform lerp(float _t, Transform const & _a, Transform const & _b);

	bool load_from_xml(IO::XML::Node const * _node);
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode);
	bool save_to_xml(IO::XML::Node * _node) const;
	bool save_to_xml_child_node(IO::XML::Node * _node, tchar const * _childNode) const;

	bool build_from_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _forward, Vector3 const & _right, Vector3 const & _up, Vector3 const & _location);
	bool build_from_two_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _first, Vector3 const & _second, Vector3 const & _location);

	inline static bool same(Transform const & _a, Transform const & _b, Optional<float> const& _translationThreshold = NP, Optional<float> const& _scaleThreshold = NP, Optional<float> const& _orientationThreshold = NP); // exactly the same
	inline static bool same_with_orientation(Transform const & _a, Transform const & _b, Optional<float> const& _translationThreshold = NP, Optional<float> const& _scaleThreshold = NP, Optional<float> const& _orientationThreshold = NP);
	inline static bool same_with_rotation(Transform const & _a, Transform const & _b, Optional<float> const& _translationThreshold = NP, Optional<float> const& _scaleThreshold = NP, Optional<float> const& _orientationThreshold = NP); // more strict

	inline bool is_zero() const { return translation == Vector3::zero && scale == 0.0f && orientation == Quat::zero; }
	inline bool is_acceptably_zero() const { return scale == 0.0f; }
	inline bool is_ok() const { return translation.is_ok() && isfinite(scale) && orientation.is_ok() && orientation.is_normalised(); }

	inline Vector3 get_axis(Axis::Type _axis) const;

	inline Matrix44 to_matrix() const;

	inline Transform & make_sane(bool _includingNonZeroScale = true);

	inline Vector3 const & get_translation() const { return translation; }
	inline Quat const & get_orientation() const { return orientation; }
	inline float get_scale() const { return scale; }

	inline void set_translation(Vector3 const & _a);
	inline void set_orientation(Quat const & _a);
	inline void set_scale(float _scale);

	inline void scale_translation(float _scale);
	
	inline Transform& make_non_zero_scale(float _preferredScale = 1.0f) { if (scale == 0.0f) scale = _preferredScale; return *this; }

	inline Transform inverted() const;

	inline void normalise_orientation() { orientation.normalise(); }

	inline Vector3 location_to_world(Vector3 const & _a) const;
	inline Vector3 location_to_local(Vector3 const & _a) const;

	inline Vector3 vector_to_world(Vector3 const & _a) const;
	inline Vector3 vector_to_local(Vector3 const & _a) const;

	inline Transform to_world(Transform const & _a) const;
	inline Transform to_local(Transform const & _a) const;

	inline Quat to_world(Quat const & _a) const;
	inline Quat to_local(Quat const & _a) const;

	inline Plane to_world(Plane const & _a) const;
	inline Plane to_local(Plane const & _a) const;

	inline Segment to_world(Segment const & _a) const;
	inline Segment to_local(Segment const & _a) const;

	bool serialise(Serialiser & _serialiser);

	void log(LogInfoContext & _log) const;

private:
	Vector3 translation;
#ifdef AN_ASSERT_SLOW
	float scale = -1.0f;
#else
	float scale;
#endif
	Quat orientation;

	friend class CoreRegisteredTypes;

private:
	// operators are prohibited! as it is not that clear what to we expect
	inline bool operator == (Transform const & _other) const { return translation == _other.translation && scale == _other.scale && orientation == _other.orientation; }
	inline bool operator != (Transform const & _other) const { return translation != _other.translation || scale != _other.scale || orientation != _other.orientation; }

	friend class Array<Transform>;
};

template <> bool Optional<Transform>::load_from_xml(IO::XML::Node const * _node);
template <> bool Optional<Transform>::load_from_xml(IO::XML::Attribute const * _attr);
