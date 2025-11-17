#pragma once

#ifndef INCLUDED_MATH_H
	#error include math.h first
#endif

struct Matrix33
{
public:
	union
	{
		struct
		{
			float m00, m01, m02;
			float m10, m11, m12;
			float m20, m21, m22;
		};
		float m[3][3]; // m[row][column]
	};

	static Matrix33 const zero;
	static Matrix33 const identity;

	inline Matrix33() {}
	inline Matrix33(float _m00, float _m01, float _m02,
					float _m10, float _m11, float _m12,
					float _m20, float _m21, float _m22);

	bool load_from_xml(IO::XML::Node const * _node);
	bool load_from_xml_child_node(IO::XML::Node const * _node, tchar const * _childNode);
	bool load_axes_from_xml(IO::XML::Node const * _node);

	bool build_orthogonal_from_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _forward, Vector3 const & _right, Vector3 const & _up);
	bool build_orthogonal_from_two_axes(Axis::Type _firstAxis, Axis::Type _secondAxis, Vector3 const & _first, Vector3 const & _second);

	inline void remove_scale();
	inline float extract_scale() const;
	inline float extract_scale_squared() const;

	inline Matrix44 to_matrix44() const;
	inline Quat to_quat() const;
	inline Transform to_transform() const;

	inline Vector3 get_x_axis() const { return Vector3(m00, m01, m02); }
	inline Vector3 get_y_axis() const { return Vector3(m10, m11, m12); }
	inline Vector3 get_z_axis() const { return Vector3(m20, m21, m22); }

	inline Vector2 get_x_axis_2() const { return Vector2(m00, m01); }
	inline Vector2 get_y_axis_2() const { return Vector2(m10, m11); }

	inline void set_x_axis(Vector3 const & _axis) { m00 = _axis.x; m01 = _axis.y; m02 = _axis.z; }
	inline void set_y_axis(Vector3 const & _axis) { m10 = _axis.x; m11 = _axis.y; m12 = _axis.z; }
	inline void set_z_axis(Vector3 const & _axis) { m20 = _axis.x; m21 = _axis.y; m22 = _axis.z; }
	inline void set_x_axis(Vector2 const & _axis) { m00 = _axis.x; m01 = _axis.y; m02 = 0.0f; }
	inline void set_y_axis(Vector2 const & _axis) { m10 = _axis.x; m11 = _axis.y; m12 = 0.0f; }

	inline void set_translation(Vector2 const & _a) { m20 = _a.x; m21 = _a.y; m22 = 1.0f; }

	inline float det() const;

	inline Matrix33 inverted() const;
	inline Matrix33 full_inverted() const;
	inline Matrix33 transposed() const;

	inline Vector2 location_to_world(Vector2 const & _a) const;
	inline Vector2 location_to_local(Vector2 const & _a) const;

	inline Vector2 vector_to_world(Vector2 const & _a) const;
	inline Vector2 vector_to_local(Vector2 const & _a) const;

	inline Vector3 to_world(Vector3 const & _a) const;
	inline Vector3 to_local(Vector3 const & _a) const;

	inline Matrix33 to_world(Matrix33 const & _a) const;
	inline Matrix33 to_local(Matrix33 const & _a) const;

	inline bool is_orthonormal() const;
	inline bool is_orthogonal() const;

	inline bool operator ==(Matrix33 const & _a) const;
	inline bool operator !=(Matrix33 const & _a) const;
	inline Matrix33 operator *(float const & _a) const;

	void log(LogInfoContext & _log) const;
};
