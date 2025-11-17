#ifdef AN_DEBUG_RENDERER
#define DEBUG_CHECK_GRADIENT_CONTEXT
//#define MEASURE_PERFORMANCE_CHECK_GRADIENT_CONTEXT
#endif

template <typename CollisionQueryPrimitive>
float CheckGradientContext<CollisionQueryPrimitive>::get_actual_distance_limit() const
{
	return should_focus_on_gradient_only() ? distances.maxDistance : max(maxDistanceToCheck.get(distances.maxInitialDistance), distances.maxInitialDistance);
}

template <typename CollisionQueryPrimitive>
CheckGradientContext<CollisionQueryPrimitive>::CheckGradientContext(CheckGradientPoints<CollisionQueryPrimitive> const & _points, CheckGradientDistances const & _distances, PlaneSet const & _clipPlanes)
: points(_points)
, distances(_distances)
, clipPlanes(_clipPlanes)
{
#ifdef AN_ASSERT
	for_every(clipPlane, clipPlanes.planes)
	{
		an_assert(!clipPlane->get_normal().is_zero(), TXT("clip plane not normalised?"));
	}
#endif
}

template <typename CollisionQueryPrimitive>
bool CheckGradientContext<CollisionQueryPrimitive>::update(Matrix44 const & _placementMatrix, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, ModelInstanceSet const & _set, float _massDifferenceCoef)
{
	bool result = false;
	for_every(instance, _set.get_instances())
	{
		result |= update(_placementMatrix, _object, _poseMatBonesRef, *instance, _massDifferenceCoef);
		if (result && should_only_check_first_collision())
		{
			break;
		}
	}
	return result;
}

template <typename CollisionQueryPrimitive>
bool CheckGradientContext<CollisionQueryPrimitive>::update(Matrix44 const & _placementMatrix, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, ModelInstance const & _instance, float _massDifferenceCoef)
{
#ifdef MEASURE_PERFORMANCE_CHECK_GRADIENT_CONTEXT
	MEASURE_PERFORMANCE(checkGradientContextUpdate);
#endif
	CheckGradientPoints<CollisionQueryPrimitive> storedPoints = points;

	{
		CollisionQueryPrimitive localQueryPrimitive = points.queryPrimitiveInPlace.transform_to_local_of(_placementMatrix);
		if (!_instance.get_bounding_box().overlaps(localQueryPrimitive.calculate_bounding_box(get_actual_distance_limit())))
		{
			return false;
		}
	}

	Matrix44 modelMatrix = _placementMatrix.to_world(_instance.get_placement_matrix());

	// get placement
	points = points.transformed_to_local_of(modelMatrix);

	PlaneSet currentClipPlanes;
	currentClipPlanes.transform_to_local_of(clipPlanes, modelMatrix);

#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
	debug_push_transform(_placementMatrix.to_world(_instance.get_placement_matrix()).to_transform());
#endif

	bool result = false;

	if (auto* model = _instance.get_model())
	{
		Optional<Collision::Flags> mainMaterialCollisionFlags;
		if (auto * checkMainMaterial = model->get_material())
		{
			if (!Flags::check(get_collision_flags(), checkMainMaterial->get_collision_flags()))
			{
				// no need to check main material, it is ok for collision flags, just go with shape to check if shape's material may reject
				// but because it is not ok, store it
				mainMaterialCollisionFlags = checkMainMaterial->get_collision_flags();
			}
		}

		while (true)
		{
			result |= update_check_gradient_for_shape_array(*this, _object, _poseMatBonesRef, mainMaterialCollisionFlags, model->get_boxes(), currentClipPlanes, _massDifferenceCoef);
			if (result && should_only_check_first_collision()) break;
			result |= update_check_gradient_for_shape_array(*this, _object, _poseMatBonesRef, mainMaterialCollisionFlags, model->get_hulls(), currentClipPlanes, _massDifferenceCoef);
			if (result && should_only_check_first_collision()) break;
			result |= update_check_gradient_for_shape_array(*this, _object, _poseMatBonesRef, mainMaterialCollisionFlags, model->get_spheres(), currentClipPlanes, _massDifferenceCoef);
			if (result && should_only_check_first_collision()) break;
			result |= update_check_gradient_for_shape_array(*this, _object, _poseMatBonesRef, mainMaterialCollisionFlags, model->get_capsules(), currentClipPlanes, _massDifferenceCoef);
			if (result && should_only_check_first_collision()) break;
			result |= update_check_gradient_for_shape_array(*this, _object, _poseMatBonesRef, mainMaterialCollisionFlags, model->get_meshes(), currentClipPlanes, _massDifferenceCoef);
			break;
		}
		distances.update_max_distance(points.epsilon);
	}

#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
	debug_pop_transform();
#endif

	// restore
	points = storedPoints;

	return result;
}

template <typename CollisionQueryPrimitive, typename CollisionShape>
bool update_check_gradient_for_shape_array(CheckGradientContext<CollisionQueryPrimitive> & _cgc, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, Optional<Collision::Flags> const & _mainMaterialCollisionFlags, Array<CollisionShape> const & _shapeArray, PlaneSet const & _clipPlanes, float _massDifferenceCoef)
{
	bool result = false;
	for_every(shape, _shapeArray)
	{
		result |= update_check_gradient_for_shape(_cgc, _object, _poseMatBonesRef, _mainMaterialCollisionFlags, *shape, _clipPlanes, _massDifferenceCoef);
		if (result && _cgc.should_only_check_first_collision())
		{
			break;
		}
	}
	return result;
}

template <typename CollisionQueryPrimitive, typename CollisionShape>
bool update_check_gradient_for_shape(CheckGradientContext<CollisionQueryPrimitive> & _cgc, ICollidableObject const * _object, Meshes::PoseMatBonesRef const & _poseMatBonesRef, Optional<Collision::Flags> const & _mainMaterialCollisionFlags, CollisionShape const & _shape, PlaneSet const & _clipPlanes, float _massDifferenceCoef)
{
#ifdef MEASURE_PERFORMANCE_CHECK_GRADIENT_CONTEXT
	MEASURE_PERFORMANCE(checkGradientContextUpdate_perShape);
#endif
	Optional<Collision::Flags> const & shapeMaterialCollisionFlags = _shape.get_material_collision_flags();
	if (shapeMaterialCollisionFlags.is_set())
	{
		if (!Flags::check(_cgc.get_collision_flags(), shapeMaterialCollisionFlags.get()))
		{
			return false;
		}
	}
	else if (_mainMaterialCollisionFlags.is_set())
	{
		if (!Flags::check(_cgc.get_collision_flags(), _mainMaterialCollisionFlags.get()))
		{
			return false;
		}
	}
	if (!_clipPlanes.planes.is_empty())
	{
#ifdef MEASURE_PERFORMANCE_CHECK_GRADIENT_CONTEXT
		MEASURE_PERFORMANCE(checkGradientContextUpdate_perShape_planes);
#endif
		float const threshold = 0.0001f; // to help against collisions right beyond door
		ShapeAgainstPlanes::Type sap = _shape.check_against_planes(_clipPlanes, threshold);
		if (sap == ShapeAgainstPlanes::Outside)
		{
			// it shouldn't be considered
			return false;
		}
	}
	CheckGradientPoints<CollisionQueryPrimitive> storedPoints = _cgc.points;
	bool pointsModified = false;
	float scale = 1.0f;
	if (_poseMatBonesRef.is_set())
	{
		int boneIdx = _shape.get_bone_idx(_poseMatBonesRef.get_skeleton());
		if (boneIdx != NONE)
		{
#ifdef AN_DEVELOPMENT
			an_assert(boneIdx < _poseMatBonesRef.get_skeleton()->get_num_bones());
			an_assert(_poseMatBonesRef.get_bones().is_index_valid(boneIdx));
#endif
			Matrix44 const & boneTransform = _poseMatBonesRef.get_bones()[boneIdx];
			if (boneTransform.has_at_least_one_zero_scale())
			{
				// skip this one
				return false;
			}
			Matrix44 const applyTransform = boneTransform.to_world(_poseMatBonesRef.get_skeleton()->get_bones_default_inverted_matrix_MS()[boneIdx]);
			scale = applyTransform.extract_scale();
			_cgc.points = _cgc.points.transformed_to_local_of(applyTransform);
			pointsModified = true;
		}
	}
	bool result = false;
	if (scale != 0.0f)
	{
		float invScale = 1.0f / scale;
#ifdef MEASURE_PERFORMANCE_CHECK_GRADIENT_CONTEXT
		MEASURE_PERFORMANCE(checkGradientContextUpdate_perShape_gradient);
#endif
		float distanceLimit = _cgc.get_actual_distance_limit();
		GradientQueryResult gqr = _cgc.points.queryPrimitiveInPlace.get_gradient(_shape, (distanceLimit) * 1.1f * invScale);
		gqr.distance *= scale;
#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
		Colour debugArrow = Colour::blue;
		Colour debugShape = Colour::blue.with_alpha(0.2f);
#endif
		if (!_cgc.maxDistanceToCheck.is_set() || gqr.distance <= _cgc.maxDistanceToCheck.get())
		{
			if (gqr.distance <= distanceLimit)
			{
				bool isOk = true;
				if (!_clipPlanes.planes.is_empty())
				{
					// this check is to prevent collision when we are beyond clip plane and gradient would indicate it is coming from beyond the plane (is pointing in same dir as plane)
					// or shape is beyond the plane and tries to influence us
					Vector3 centre = _cgc.points.queryPrimitiveInPlace.get_centre();
					Vector3 shapeCentre = _shape.get_centre();
					for_every(plane, _clipPlanes.planes)
					{
#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
						if (debug_is_filter_allowed())
						{
							//debug_draw_text(true, Colour::green, shapeCentre + Vector3(0.0f, 0.0f, 0.0f), Vector2::half, true, 0.2f, NP, TXT("pif:c:%.3f"), plane->get_in_front(centre));
							//debug_draw_text(true, Colour::green, shapeCentre + Vector3(0.0f, 0.0f, -0.02f), Vector2::half, true, 0.2f, NP, TXT("pif:sc:%.3f"), plane->get_in_front(shapeCentre));
							//debug_draw_text(true, Colour::green, shapeCentre + Vector3(0.0f, 0.0f, -0.04f), Vector2::half, true, 0.2f, NP, TXT("non:%.3f"), Vector3::dot(plane->get_normal(), gqr.normal));
						}
#endif
						bool objectBeyond = plane->get_in_front(centre) < 0.0f;
						bool shapeBeyond = plane->get_in_front(shapeCentre) < 0.0f;
						float dotNormal = Vector3::dot(plane->get_normal(), gqr.normal);
						if ((objectBeyond || shapeBeyond) &&
							dotNormal > 0.0f) // pushes out of plane
						{
#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
							if (debug_is_filter_allowed())
							{
								debug_draw_text(true, Colour::c64Brown, shapeCentre + Vector3(0.0f, 0.0f, -0.06f), Vector2::half, true, 0.2f, NP, TXT("reject"));
							}
							debugArrow = Colour::c64Brown;
							debugShape = Colour::c64Brown.with_alpha(0.2f);
#endif
							isOk = false;
							break;
						}
						if ((objectBeyond && shapeBeyond) &&
							abs(dotNormal) > 0.7f) // pushes perpendicular to the plane, we want to keep only those along the plane
						{
#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
							if (debug_is_filter_allowed())
							{
								debug_draw_text(true, Colour::grey, shapeCentre + Vector3(0.0f, 0.0f, -0.06f), Vector2::half, true, 0.2f, NP, TXT("reject"));
							}
							debugArrow = Colour::grey;
							debugShape = Colour::grey.with_alpha(0.2f);
#endif
							isOk = false;
							break;
						}
					}
				}
				if (isOk)
				{
#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
					debugArrow = Colour::green;
					debugShape = Colour::red;
#endif
					result = true;
					_cgc.minDistanceFound = min(_cgc.minDistanceFound.get(gqr.distance), gqr.distance);
					if (!_cgc.should_only_check() && !_cgc.should_only_check_first_collision() &&
						(!_cgc.maxDistanceToUpdateGradient.is_set() || (_cgc.maxDistanceToUpdateGradient.get() > 0.0f && gqr.distance <= _cgc.maxDistanceToUpdateGradient.get())))
					{
						CheckGradientDistances::Dir* dir = _cgc.distances.dirs;
						for (int pointIdx = 0; pointIdx < 6; ++pointIdx, ++dir)
						{
							// calculate offset and use normal to modify distance and fill points
							Vector3 const pointOffset = _cgc.points.get_point_offset((CheckGradientPoint::Type)pointIdx);
							float distanceToPointOffset = gqr.distance + Vector3::dot(gqr.normal, pointOffset);
							// apply mass difference
							distanceToPointOffset = _cgc.distances.maxInitialDistance - (_cgc.distances.maxInitialDistance - distanceToPointOffset) * _massDifferenceCoef;
							if (dir->distance >= distanceToPointOffset)
							{
								dir->distance = distanceToPointOffset;
								dir->object = _object;
								dir->shape = &_shape;
							}
						}
					}
				}
			}
		}
#ifdef DEBUG_CHECK_GRADIENT_CONTEXT
		Range3 shapeRange = _shape.calculate_bounding_box(Transform::identity, nullptr, false); // pose?
		Vector3 shapeCentre = shapeRange.centre();
		if (debug_is_filter_allowed())
		{
			debug_draw_arrow(true, debugArrow, shapeCentre * scale, shapeCentre * scale + gqr.normal * gqr.distance);
			//debug_draw_text(true, gqr.distance <= _cgc.distances.maxInitialDistance ? Colour::green : Colour::blue, shapeCentre * scale, NP, NP, 0.3f, false, TXT("%.3f"), gqr.distance);
			if (result)
			{
				_shape.debug_draw(true, false, debugShape, 0.4f, _poseMatBonesRef);
			}
		}
#endif
	}
	if (pointsModified)
	{
		_cgc.points = storedPoints;
	}
	if (!_cgc.should_only_check() && ! _cgc.should_only_check_first_collision())
	{
		_cgc.distances.update_max_distance(_cgc.points.epsilon);
	}

	return result;
}

