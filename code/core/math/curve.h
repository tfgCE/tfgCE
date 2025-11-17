#pragma once

#ifndef INCLUDED_MATH_H
#error include math.h first
#endif

template <typename PointClass>
struct Curve
{
public:
	Curve();

	PointClass calculate_at(float _t) const;

	bool is_empty() const { return points.is_empty(); }

	bool load_from_xml(IO::XML::Node const * _node, tchar const * const _pointNodeName, tchar const * const _tAttrName = TXT("t"));

	void add(float _t, PointClass const & _point);

	void update();

private:
	struct Point
	{
		float t = 0;
		float invToNextT = 0;
		PointClass point;
	};
	Array<Point> points;

};