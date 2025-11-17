inline Vector2 ik2_calculate_joint_loc(float _length0, float _length1, float _dist)
{
	if (_dist == 0.0f)
	{
		return Vector2::zero;
	}
	//		(J)
	//		/:\
	//	  0/ h \1
	//	  /  :  \
	//	(U)-a+b-(L)
	//		dist
	//
	// h^2 = "0"^2 - a^2
	// h^2 = "1"^2 - b^2
	// (a + b) = dist
	// "0"^2 - a^2 = "1"^2 - b^2
	// a^2 - b^2 = "0"^2 - "1"^2 = diff
	// a^2 - (dist - a)^2 = diff
	// a^2 - (dist^2 - 2 dist a + a^2) = diff
	// a^2 - dist^2 + 2 dist a - a^2 = diff
	// 2 dist a = diff + dist^2
	// a = ( diff + dist^2 ) / 2 dist
	// a = ( "0"^2 - "1"^2 + dist^2 ) / ( 2 dist )
	float sqr0 = sqr(_length0);
	float a = (sqr0 - sqr(_length1) + sqr(_dist)) / (2.0f * _dist);
	//an_assert(a >= 0.0f); // this assert is a little bit too much

	float h = sqrt(max(0.0f, sqr0 - sqr(a)));

	return Vector2(a, h);

}

inline Matrix44 ik2_bone(Vector3 const & _loc, Vector3 const & _at, Vector3 const _right)
{
	return look_at_matrix_with_right(_loc, _at, _right);
}

