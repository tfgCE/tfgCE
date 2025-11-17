void PlaneSet::clear()
{
	planes.clear();
}

void PlaneSet::add(Plane const & _plane)
{
	if (planes.get_size() < planes.get_max_size())
	{
		for_every_const(plane, planes)
		{
			if (*plane == _plane)
			{
				return;
			}
		}
		planes.push_back(_plane);
	}
}

void PlaneSet::add(PlaneSet const & _planes)
{
	for_every(plane, _planes.planes)
	{
		add(*plane);
	}
}

void PlaneSet::transform_to_local_of(PlaneSet const & _source, Matrix44 const & _transformToNewSpace)
{
	planes = _source.planes;
	transform_to_local_of(_transformToNewSpace);
}

void PlaneSet::transform_to_local_of(PlaneSet const & _source, Transform const & _transform)
{
	planes = _source.planes;
	transform_to_local_of(_transform);
}

void PlaneSet::transform_to_local_of(Matrix44 const & _transformToNewSpace)
{
	for_every(plane, planes)
	{
		*plane = _transformToNewSpace.to_local(*plane);
	}
}

void PlaneSet::transform_to_local_of(Transform const & _transform)
{
	for_every(plane, planes)
	{
		*plane = _transform.to_local(*plane);
	}
}
