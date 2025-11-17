#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Matrix44
{
public:
	union
	{
		struct
		{
			float m00, m01, m02, m03;
			float m10, m11, m12, m13;
			float m20, m21, m22, m23;
			float m30, m31, m32, m33;
		};
		float m[4][4]; // m[row][column]
	};

	static Matrix44 const zero;
	static Matrix44 const identity;

	inline Matrix44() {}
	inline Matrix44(float _m00, float _m01, float _m02, float _m03,
					float _m10, float _m11, float _m12, float _m13,
					float _m20, float _m21, float _m22, float _m23,
					float _m30, float _m31, float _m32, float _m33);

	bool load_from_xml(IO::XML::Node const * _node);
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode);

	inline void remove_scale();
	inline float extract_scale() const;
	inline float extract_scale_squared() const;
	inline float extract_min_scale() const;
	inline bool has_uniform_scale() const;
	inline bool has_at_least_one_zero_scale() const;

	inline Quat to_quat() const;
	inline Transform to_transform() const;
	inline Matrix33 to_matrix33() const;

	inline Vector3 get_x_axis() const { return Vector3(m00, m01, m02); }
	inline Vector3 get_y_axis() const { return Vector3(m10, m11, m12); }
	inline Vector3 get_z_axis() const { return Vector3(m20, m21, m22); }
	inline Vector3 get_translation() const { return Vector3(m30, m31, m32); }

	inline void set_x_axis(Vector3 const & _axis) { m00 = _axis.x; m01 = _axis.y; m02 = _axis.z; }
	inline void set_y_axis(Vector3 const & _axis) { m10 = _axis.x; m11 = _axis.y; m12 = _axis.z; }
	inline void set_z_axis(Vector3 const & _axis) { m20 = _axis.x; m21 = _axis.y; m22 = _axis.z; }

	inline void set_translation(Vector3 const & _a);

	inline void set_orientation_matrix(Matrix33 const & _orientation);
	inline Matrix33 get_orientation_matrix() const;

	inline float det() const;

	inline Matrix44 inverted() const;
	inline Matrix44 full_inverted() const;
	inline Matrix44 transposed() const;

	inline Vector4 mul(Vector4 const & _a) const;

	inline Vector3 location_to_world(Vector3 const & _a) const;
	inline Vector3 location_to_local(Vector3 const & _a) const;

	inline Vector3 vector_to_world(Vector3 const & _a) const;
	inline Vector3 vector_to_local(Vector3 const & _a) const;

	inline Plane to_world(Plane const & _a) const;
	inline Plane to_local(Plane const & _a) const;

	inline Matrix44 to_world(Matrix44 const & _a) const;
	inline Matrix44 to_local(Matrix44 const & _a) const;

	inline Segment to_world(Segment const & _a) const;
	inline Segment to_local(Segment const & _a) const;

	inline bool is_orthonormal() const;
	inline bool is_orthogonal() const;

	inline bool operator ==(Matrix44 const & _a) const;
	inline bool operator !=(Matrix44 const & _a) const;
	inline Matrix44 operator *(float const & _a) const;

	void fix_axes_forward_up_right(); // y being most important

	void log(LogInfoContext & _log) const;
};
