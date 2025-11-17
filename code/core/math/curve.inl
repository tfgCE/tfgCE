template <typename PointClass>
Curve<PointClass>::Curve()
{
}

template <typename PointClass>
PointClass Curve<PointClass>::calculate_at(float _t) const
{
	Point const * prev = nullptr;
	for_every(point, points)
	{
		if (_t < point->t)
		{
			break;
		}
		prev = point;
	}
	Point const * next = prev ? prev + 1 : points.get_data();
	prev = prev ? prev : next;
	_t = clamp((_t - prev->t) * prev->invToNextT, 0.0f, 1.0f);
	return prev->point * (1.0f - _t) +
		   next->point * _t;
}

template <typename PointClass>
void Curve<PointClass>::update()
{
	Point * prev = nullptr;
	for_every(point, points)
	{
		point->invToNextT = 0.0f;
		if (prev)
		{
			prev->invToNextT = point->t != prev->t? 1.0f / (point->t - prev->t) : 0.0f;
		}
		prev = point;
	}
}

template <typename PointClass>
bool Curve<PointClass>::load_from_xml(IO::XML::Node const * _node, tchar const * const _pointNodeName, tchar const * const _tAttrName)
{
	bool result = true;

	for_every(node, _node->children_named(_pointNodeName))
	{
		Point point;
		point.t = node->get_float_attribute(_tAttrName, point.t);
		point.point = node->get<PointClass>(zero<PointClass>());
		points.push_back(point);
	}

	update();

	return result;
}

template <typename PointClass>
void Curve<PointClass>::add(float _t, PointClass const & _point)
{
	Point point;
	point.t = _t;
	point.point = _point;
	points.push_back(point);

	update();
}
