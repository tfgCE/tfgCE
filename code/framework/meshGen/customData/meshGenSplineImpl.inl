template <typename Point>
Framework::MeshGeneration::CustomData::Spline<Point>::Spline()
{
	refPoints.push_back(RefPoint(NAME(zero), zero<Point>()));
}

template <typename Point>
bool Framework::MeshGeneration::CustomData::Spline<Point>::has_ref_point(Name const & _name) const
{
	for_every(refPoint, refPoints)
	{
		if (refPoint->name == _name)
		{
			return true;
		}
	}
	return false;
}

template <typename Point>
Point const * Framework::MeshGeneration::CustomData::Spline<Point>::find_ref_point(Name const & _name) const
{
	for_every(refPoint, refPoints)
	{
		if (refPoint->name == _name)
		{
			return &refPoint->location;
		}
	}
	return nullptr;
}

#ifdef AN_DEVELOPMENT
template <typename Point>
void Framework::MeshGeneration::CustomData::Spline<Point>::ready_for_reload()
{
	base::ready_for_reload();

	segments.clear();
	refPoints.clear();
}
#endif

// we're using temp tool set to avoid memory allocation and deallocation with every load
inline bool load_temp_tool_set(REF_ RefCountObjectPtr<WheresMyPoint::ToolSet> & tempToolSet, IO::XML::Node const * _node)
{
	if (!_node)
	{
		return false;
	}
	if (auto* node = _node->first_child_named(TXT("wheresMyPoint")))
	{
		if (!tempToolSet.is_set() || !tempToolSet->is_empty()) tempToolSet = new WheresMyPoint::ToolSet();
		return tempToolSet->load_from_xml(node) && !tempToolSet->is_empty();
	}
	else
	{
		return false;
	}
}

template <typename Point>
bool Framework::MeshGeneration::CustomData::Spline<Point>::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	UVDef uvDef;
	uvDef.set_u(0.0f);

	Point point = zero<Point>();
	RefCountObjectPtr<WheresMyPoint::ToolSet> pointToolSet;
	RefCountObjectPtr<WheresMyPoint::ToolSet> tempToolSet;
	bool thereWasAtLeastOnePoint = false;

	for_every(node, _node->all_children())
	{
		if (node->get_name() == TXT("refPoint"))
		{
			RefPoint refPoint;
			refPoint.name = node->get_name_attribute(TXT("name"));
			if (!refPoint.name.is_valid())
			{
				error(TXT("no name provided for ref point"));
				result = false;
				continue;
			}
			refPoint.location = zero<Point>();
			refPoint.location.load_from_xml(node);
			bool found = false;
			for_every(existingRefPoint, refPoints)
			{
				if (existingRefPoint->name == refPoint.name)
				{
					*existingRefPoint = refPoint;
					found = true;
					break;
				}
			}
			if (!found)
			{
				refPoints.push_back(refPoint);
			}
		}
		if (node->get_name() == TXT("u") ||
			node->get_name() == TXT("uv") || 
			node->get_name() == TXT("uvDef"))
		{
			uvDef.load_from_xml_with_clearing(node);
		}
		if (node->get_name() == TXT("point"))
		{
			Point prevPoint = point;
			point.load_from_xml(node);
			uvDef.load_from_xml_with_clearing(node);
			if (thereWasAtLeastOnePoint)
			{
				Segment newSegment;
				newSegment.uvDef = uvDef;
				newSegment.segment.p0 = prevPoint;
				newSegment.segment.p3 = point;
				newSegment.segment.make_linear();

				if (pointToolSet.is_set() && segments.is_empty())
				{
					newSegment.p0ToolSet = pointToolSet;
					pointToolSet.clear();
				}
				if (load_temp_tool_set(REF_ tempToolSet, node))
				{
					newSegment.p3ToolSet = tempToolSet;
					tempToolSet.clear();
				}
				newSegment.forceLinear = true;
				segments.push_back(newSegment);
				pointToolSet = newSegment.p3ToolSet;
			}
			else
			{
				if (load_temp_tool_set(REF_ tempToolSet, node))
				{
					pointToolSet = tempToolSet;
					tempToolSet.clear();
				}
			}
			thereWasAtLeastOnePoint = true;
		}
		if (node->get_name() == TXT("lineTo"))
		{
			Point to = point;
			to.load_from_xml(node);
			uvDef.load_from_xml_with_clearing(node);
			RefCountObjectPtr<WheresMyPoint::ToolSet> thisToolSet;
			if (load_temp_tool_set(REF_ tempToolSet, node))
			{
				thisToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (to == point && (! thisToolSet.is_set() && !pointToolSet.is_set()))
			{
				error_loading_xml(node, TXT("zero line"));
				result = false;
			}
			else
			{
				if (!thereWasAtLeastOnePoint)
				{
					error_loading_xml(node, TXT("there should be point before lineTo!"));
					result = false;
				}
				else
				{
					Segment newSegment;
					newSegment.uvDef = uvDef;
					newSegment.segment.p0 = point;
					newSegment.segment.p3 = to;
					newSegment.segment.make_linear();

					if (pointToolSet.is_set() && segments.is_empty())
					{
						newSegment.p0ToolSet = pointToolSet;
						pointToolSet.clear();
					}
					if (thisToolSet.is_set())
					{
						newSegment.p3ToolSet = thisToolSet;
						thisToolSet.clear();
					}
					newSegment.forceLinear = true;
					segments.push_back(newSegment);
					pointToolSet = newSegment.p3ToolSet;
				}
				point = to;
				thereWasAtLeastOnePoint = true;
			}
		}
		if (node->get_name() == TXT("curve"))
		{
			Segment curveSegment;
			curveSegment.segment.p0 = point;
			curveSegment.segment.p1 = point;
			curveSegment.segment.p2 = point;
			curveSegment.segment.p3 = point;
			curveSegment.segment.p3.load_from_xml(node);
			curveSegment.segment.make_linear();
			curveSegment.segment.load_from_xml(node, true, false); // we read as it is written and then we process it to be linear or round
			if (load_temp_tool_set(REF_ tempToolSet, node->first_child_named(TXT("p"))))
			{
				// useful when multiplying values
				// keep p0 - we should not modify it!
				if (pointToolSet.is_set() && segments.is_empty())
				{
					// for the first segment we want to store point tool set
					curveSegment.p0ToolSet = pointToolSet;
				}
				pointToolSet.clear(); // clear p to not override p0 for next
				curveSegment.p1ToolSet = tempToolSet;
				curveSegment.p2ToolSet = tempToolSet;
				curveSegment.p3ToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (load_temp_tool_set(REF_ tempToolSet, node->first_child_named(TXT("p0"))))
			{
				curveSegment.p0ToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (load_temp_tool_set(REF_ tempToolSet, node->first_child_named(TXT("p1"))))
			{
				curveSegment.p1ToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (load_temp_tool_set(REF_ tempToolSet, node->first_child_named(TXT("p2"))))
			{
				curveSegment.p2ToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (load_temp_tool_set(REF_ tempToolSet, node->first_child_named(TXT("p12"))))
			{
				curveSegment.p1ToolSet = tempToolSet;
				curveSegment.p2ToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (load_temp_tool_set(REF_ tempToolSet, node->first_child_named(TXT("p3"))))
			{
				curveSegment.p3ToolSet = tempToolSet;
				tempToolSet.clear();
			}
			if (thereWasAtLeastOnePoint &&
				(curveSegment.segment.p0 != point ||
				 (pointToolSet.is_set() && curveSegment.p0ToolSet.is_set() && curveSegment.p0ToolSet != pointToolSet)))
			{
				// add element before curve from last point, storing old uvDef
				Segment newSegment;
				newSegment.uvDef = uvDef;
				newSegment.segment.p0 = point;
				newSegment.segment.p3 = curveSegment.segment.p0;
				newSegment.p3ToolSet = curveSegment.p0ToolSet;
				newSegment.segment.make_linear();
				if (pointToolSet.is_set())
				{
					newSegment.p0ToolSet = pointToolSet;
					pointToolSet.clear();
				}
				segments.push_back(newSegment);
				pointToolSet = newSegment.p3ToolSet;
			}
			if (!curveSegment.p0ToolSet.is_set() && pointToolSet.is_set())
			{
				curveSegment.p0ToolSet = pointToolSet;
				pointToolSet.clear();
			}
			uvDef.load_from_xml_with_clearing(node);
			curveSegment.uvDef = uvDef;
			if (node->get_bool_attribute_or_from_child_presence(TXT("withEvenSeparation")) ||
				node->get_bool_attribute_or_from_child_presence(TXT("evenSeparation")) ||
				node->get_bool_attribute_or_from_child_presence(TXT("evenlySeparated")))
			{
				warn_loading_xml(node, TXT("use round* instead of even*"));
				curveSegment.withRoundSeparation = true;
			}
			if (node->get_bool_attribute_or_from_child_presence(TXT("withRoundSeparation")) ||
				node->get_bool_attribute_or_from_child_presence(TXT("roundSeparation")) ||
				node->get_bool_attribute_or_from_child_presence(TXT("roundlySeparated")))
			{
				curveSegment.withRoundSeparation = true;
			}
			segments.push_back(curveSegment);
			//
			thereWasAtLeastOnePoint = true;
			point = curveSegment.segment.p3;
			pointToolSet = curveSegment.p3ToolSet;
		}
		if (node->get_name() == TXT("circle"))
		{
			Point at = Point::zero;
			at.load_from_xml(node);
			Point radius = Point::one;
			{
				float r = node->get_float_attribute(TXT("radius"), 1.0f);
				radius *= r;
			}
			{
				float r = node->get_float_attribute(TXT("radiusX"), radius.x);
				radius.x = r;
			}
			{
				float r = node->get_float_attribute(TXT("radiusY"), radius.y);
				radius.y = r;
			}

			bool usingTempToolSet = false;
			tempToolSet.clear();
			if (load_temp_tool_set(REF_ tempToolSet, node))
			{
				usingTempToolSet = true;
			}

			Point qp[] = { Point::zero, Point::zero, Point::zero, Point::zero };
			Point qc[] = { Point::zero, Point::zero, Point::zero, Point::zero };
			qp[0].x =  0.0f; qp[0].y =  1.0f;
			qp[1].x =  1.0f; qp[1].y =  0.0f;
			qp[2].x =  0.0f; qp[2].y = -1.0f;
			qp[3].x = -1.0f; qp[3].y =  0.0f;
			qc[0].x =  1.0f; qc[0].y =  1.0f;
			qc[1].x =  1.0f; qc[1].y = -1.0f;
			qc[2].x = -1.0f; qc[2].y = -1.0f;
			qc[3].x = -1.0f; qc[3].y =  1.0f;

			for_count(int, qIdx, 4)
			{
				Segment curveSegment;
				curveSegment.uvDef = uvDef;
				curveSegment.segment.p0 = at + radius * qp[qIdx];
				curveSegment.p0ToolSet = tempToolSet;
				curveSegment.segment.p1 = at + radius * qc[qIdx];
				curveSegment.segment.p2 = at + radius * qc[qIdx];
				curveSegment.segment.p3 = at + radius * qp[mod(qIdx + 1, 4)];
				curveSegment.withRoundSeparation = true;
				if (usingTempToolSet)
				{
					curveSegment.p1ToolSet = tempToolSet;
					curveSegment.p2ToolSet = tempToolSet;
					curveSegment.p3ToolSet = tempToolSet;
				}
				{
					thereWasAtLeastOnePoint = true;
					point = curveSegment.segment.p3;
					if (usingTempToolSet)
					{
						pointToolSet = curveSegment.p3ToolSet;
					}
					else
					{
						pointToolSet.clear();
					}
				}
				segments.push_back(curveSegment);
			}
			tempToolSet.clear();
		}
	}
	if (_node->get_bool_attribute_or_from_child_presence(TXT("reversed")))
	{
		auto oldSegments = segments;
		segments.clear();
		for_every_reverse(s, oldSegments)
		{
			Segment newSegment = *s;
			newSegment.uvDef = s->uvDef;
			swap(newSegment.segment.p0, newSegment.segment.p3);
			swap(newSegment.segment.p1, newSegment.segment.p2);
			swap(newSegment.p0ToolSet, newSegment.p3ToolSet);
			swap(newSegment.p1ToolSet, newSegment.p2ToolSet);
			segments.push_back(newSegment);
		}
		// fix p3 that always has defined at expense of p0 not defined in the following segment, just move it back
		CustomData::Spline<Point>::Segment* prevS = nullptr;
		for_every(s, segments)
		{
			if (prevS && !prevS->p3ToolSet.is_set() &&
				s->p0ToolSet.is_set())
			{
				prevS->p3ToolSet = s->p0ToolSet;
				s->p0ToolSet.clear();
			}
			prevS = s;
		}
	}
	return result;
}

template <typename Point>
bool Framework::MeshGeneration::CustomData::Spline<Point>::get_segments_for(OUT_ Array<Segment> & _outSegments, WheresMyPoint::Context & _wmpContext, SplineContext<Point> const & _splineContext) const
{
	bool result = true;

	_outSegments.clear();
	_outSegments.make_space_for(segments.get_size());

	Optional<Point> prevPoint;
	for_every(segment, segments)
	{
		Segment newSegment;
		newSegment.segment = segment->segment;
		newSegment.uvDef = segment->uvDef;

		if (prevPoint.is_set())
		{
			newSegment.segment.p0 = prevPoint.get();
		}

		// first do p0 and p3 to assume linear curve
		if (segment->p0ToolSet.is_set())
		{
			if (!segment->p0ToolSet->update(newSegment.segment.p0, _wmpContext))
			{
				error(TXT("error processing wmp p0 toolset"));
				result = false;
			}
			if (prevPoint.is_set() && 
				! (newSegment.segment.p0 - prevPoint.get()).is_almost_zero())
			{
				warn(TXT("p0 overriden for not first point!"));
				newSegment.segment.p0 = prevPoint.get(); // force it to be the same
				//result = false;
			}
		}
		if (segment->p3ToolSet.is_set())
		{
			if (!segment->p3ToolSet->update(newSegment.segment.p3, _wmpContext))
			{
				error(TXT("error processing wmp p3 toolset"));
				result = false;
			}
			else if(! segment->p1ToolSet.is_set() || !segment->p2ToolSet.is_set())
			{
				newSegment.segment.make_linear();
			}
		}

		// now control points p1 and p2
		if (segment->p1ToolSet.is_set())
		{
			if (!segment->p1ToolSet->update(newSegment.segment.p1, _wmpContext))
			{
				error(TXT("error processing wmp p1 toolset"));
				result = false;
			}
		}
		if (segment->p2ToolSet.is_set())
		{
			if (segment->p1ToolSet.is_set() && segment->p2ToolSet.get() == segment->p1ToolSet.get())
			{
				newSegment.segment.p2 = newSegment.segment.p1;
			}
			else if (!segment->p2ToolSet->update(newSegment.segment.p2, _wmpContext))
			{
				error(TXT("error processing wmp p2 toolset"));
				result = false;
			}
		}
		prevPoint = newSegment.segment.p3; // it has to be done before transform!

		newSegment.segment.p0 = _splineContext.transform.location_to_world(newSegment.segment.p0);
		newSegment.segment.p1 = _splineContext.transform.location_to_world(newSegment.segment.p1);
		newSegment.segment.p2 = _splineContext.transform.location_to_world(newSegment.segment.p2);
		newSegment.segment.p3 = _splineContext.transform.location_to_world(newSegment.segment.p3);
		if (segment->forceLinear)
		{
			newSegment.segment.make_linear();
		}
		if (segment->withRoundSeparation)
		{
			newSegment.segment.make_roundly_separated();
		}
		_outSegments.push_back(newSegment);
	}

	return true;
}

template <typename Point>
void Framework::MeshGeneration::CustomData::Spline<Point>::change_segments_into_points(Array<Segment> const & _segments, Point const & _usingScale, SubStepDef const & _subStepDef, GenerationContext const & _context, OUT_ Array<Point> & _points, OUT_ Array<UVDef> & _us, Optional<bool> const& _ignoreAutoModifiers)
{
	for_every(segment, _segments)
	{
		if (segment->does_require_generation())
		{
			_points.clear();
			_us.clear();
			error(TXT("generate segments into segments (to drop wmp toolset)"));
			an_assert(false);
			return;
		}
	}
	Array<int> numSubStepsPerSegment;
	change_segments_into_number_of_points(_segments, _usingScale, _subStepDef, _context, numSubStepsPerSegment, _ignoreAutoModifiers);
	change_segments_into_points(_segments, _usingScale, numSubStepsPerSegment, _points, _us);
}

template <typename Point>
void Framework::MeshGeneration::CustomData::Spline<Point>::change_segments_into_number_of_points(Array<Segment> const & _segments, Point const & _usingScale, SubStepDef const & _subStepDef, GenerationContext const & _context, OUT_ Array<int> & _numSubStepsPerSegment, Optional<bool> const& _ignoreAutoModifiers)
{
	_numSubStepsPerSegment.clear();
	for_every(segment, _segments)
	{
		if (segment->does_require_generation())
		{
			error(TXT("generate segments into segments (to drop wmp toolset)"));
			an_assert(false);
			return;
		}
	}
	_numSubStepsPerSegment.make_space_for(_segments.get_size());
	for_every(segment, _segments)
	{
		BezierCurve<Point> scaledCurve = segment->segment;
		scaledCurve *= _usingScale;
		int segmentSubStepCount = max(1, _subStepDef.calculate_sub_step_count_for(scaledCurve.length(), _context, NP, _ignoreAutoModifiers));
		if (segment->segment.is_linear() && ! _subStepDef.should_sub_divide_linear(_context))
		{
			segmentSubStepCount = 1;
		}
		_numSubStepsPerSegment.push_back(segmentSubStepCount);
	}
}

template <typename Point>
void Framework::MeshGeneration::CustomData::Spline<Point>::change_segments_into_points(Array<Segment> const & _segments, Point const & _usingScale, Array<int> const & _numSubStepsPerSegment, OUT_ Array<Point> & _points, OUT_ Array<UVDef> & _us)
{
	for_every(segment, _segments)
	{
		if (segment->does_require_generation())
		{
			_points.clear();
			_us.clear();
			error(TXT("generate segments into segments (to drop wmp toolset)"));
			an_assert(false);
			return;
		}
	}
	if (_numSubStepsPerSegment.get_size() != _segments.get_size())
	{
		_points.clear();
		_us.clear();
		error(TXT("number of sub steps per segment doesn't match number of segments!"));
		return;
	}
	int subStepCount = 0;
	for_every(segment, _segments)
	{
		subStepCount += _numSubStepsPerSegment[for_everys_index(segment)];
	}
	_points.clear();
	_us.clear();
	_points.make_space_for(subStepCount + 1);  // for the last one
	_us.make_space_for(subStepCount);

	// build points
	Point lastPoint = zero<Point>();
	for_every(segment, _segments)
	{
		BezierCurve<Point> scaledCurve = segment->segment;
		scaledCurve *= _usingScale;
		int segmentSubStepCount = _numSubStepsPerSegment[for_everys_index(segment)];
		float invCsSegmentSubStepCount = 1.0f / (float)segmentSubStepCount;
		for (int i = 0; i < segmentSubStepCount; ++i)
		{
			float t = (float)i * invCsSegmentSubStepCount;
			Point point = scaledCurve.calculate_at(t);
			_points.push_back(point);
			_us.push_back(segment->uvDef);
		}
		lastPoint = scaledCurve.calculate_at(1.0f);
	}
	_points.push_back(lastPoint);
}

template <typename Point>
void Framework::MeshGeneration::CustomData::Spline<Point>::make_sure_segments_are_clockwise(Array<Segment> & _segments)
{
	todo_note(TXT("how? is it even possible? if cross section is closed, then yes, but how otherwise? for convex shapes it is possible to determine too"));
}

template <typename Point>
void Framework::MeshGeneration::CustomData::Spline<Point>::do_for_every_segment(std::function<void(Segment& _segment)> _do)
{
	for_every(segment, segments)
	{
		_do(*segment);
	}
}

template <typename Point>
bool Framework::MeshGeneration::CustomData::Spline<Point>::load_replace_u_from_xml(IO::XML::Node const* _node, LibraryLoadingContext& _lc)
{
	bool result = true;
	for_every(node, _node->children_named(TXT("replaceU")))
	{
		UVDef with;
		with.load_from_xml(node);
		Array<UVDef> keep;
		for_every(n, node->children_named(TXT("keep")))
		{
			UVDef k;
			if (k.load_from_xml(n))
			{
				keep.push_back(k);
			}
			else
			{
				result = false;
			}
		}

		for_every(s, segments)
		{
			bool shouldBeKept = false;
			for_every(k, keep)
			{
				if (s->uvDef == *k)
				{
					shouldBeKept = true;
					break;
				}
			}

			if (! shouldBeKept)
			{
				s->uvDef = with;
			}
		}
	}
	return result;
}

