//

inline int get_gl_clip_distance(int _idx)
{
#ifdef AN_GL
	return GL_CLIP_PLANE0 + _idx;
#else
#ifdef AN_GLES
	return GL_CLIP_DISTANCE0_EXT + _idx;
#else
#error implement
#endif
#endif
}

//

template <unsigned int PlaneCount>
bool ::System::CustomVideoClipPlaneSet<PlaneCount>::operator ==(CustomVideoClipPlaneSet<PlaneCount> const & _other) const
{
	if (planes.get_size() != _other.planes.get_size())
	{
		return false;
	}
	Plane const * otherPlaneCS = _other.planes.get_data();
	for_every(planeCS, planes)
	{
		if (*planeCS != *otherPlaneCS)
		{
			return false;
		}
		++otherPlaneCS;
	}

	return true;
}

template <unsigned int PlaneCount>
void ::System::CustomVideoClipPlaneSet<PlaneCount>::clear()
{
	planes.clear();
}

template <unsigned int PlaneCount>
void ::System::CustomVideoClipPlaneSet<PlaneCount>::add(Plane const& _plane, VideoMatrixStack const& _videoMatrixStack)
{
#ifdef AN_ASSERT
	planes.push_back(_plane.is_garbage()? _plane : _videoMatrixStack.get_current().to_world(_plane));
#else
	planes.push_back(_videoMatrixStack.get_current().to_world(_plane));
#endif
}

template <unsigned int PlaneCount>
void ::System::CustomVideoClipPlaneSet<PlaneCount>::add_as_is(Plane const& _plane)
{
	planes.push_back(_plane);
}

template <unsigned int PlaneCount>
void ::System::CustomVideoClipPlaneSet<PlaneCount>::set_with(PlaneSet const& _planeSet, VideoMatrixStack const& _videoMatrixStack)
{
	planes.clear();
	for_every(plane, _planeSet.planes)
	{
		add(*plane, _videoMatrixStack);
	}
}

template <unsigned int PlaneCount>
void ::System::CustomVideoClipPlaneSet<PlaneCount>::set_with_to_local(Matrix44 const& _viewMatrix, ViewFrustum const& _frustum)
{
	planes.clear();
	for_every(edge, _frustum.get_edges())
	{
		Vector3 loc = edge->point;
		Vector3 nor = edge->normal;
		Plane p(nor, loc);
		p = _viewMatrix.to_local(p);
		planes.push_back(p);
	}
}

//

template <typename UseClipPlaneSet>
::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::CustomVideoClipPlaneStack()
: clipPlanesSent(0)
, clipPlanesEnabled(0)
{
	SET_EXTRA_DEBUG_INFO(stack, TXT("CustomVideoClipPlaneStack.stack"));
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::clear()
{
	stack.clear();
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::push()
{
	stack.set_size(stack.get_size() + 1);
	stack.get_last().clear();
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::push_current_as_is()
{
	stack.set_size(stack.get_size() + 1);
	stack.get_last().clear();
	stack.get_last() = stack[stack.get_size() - 2];
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::pop()
{
	stack.set_size(stack.get_size() - 1);
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::set_current(PlaneSet const& _planes)
{
	stack.get_last().set_with(_planes, *videoMatrixStack);
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::clear_current()
{
	stack.get_last().clear();
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::add_to_current(Plane const& _plane)
{
	stack.get_last().add(_plane, *videoMatrixStack);
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::add_to_current(PlaneSet const& _planes)
{
	for_every(plane, _planes.planes)
	{
		add_to_current(*plane);
	}
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::add_as_is_to_current(ViewFrustum const& _viewFrustum)
{
	for_every(edge, _viewFrustum.get_edges())
	{
		stack.get_last().add_as_is(Plane(edge->normal, edge->point));
	}
	isReadyForRendering = false;
}

template <typename UseClipPlaneSet>
void ::System::CustomVideoClipPlaneStack<UseClipPlaneSet>::ready_current(bool _force)
{
	if (isReadyForRendering)
	{
		return;
	}
	isReadyForRendering = true;
	// check thingsThatIDontUnderstand.txt [1] and [3]
	an_assert(clipPlanesEnabled > 0, TXT("open clip planes!"));
	if (! stack.is_empty())
	{
		VideoClipPlaneSet const * current = &stack.get_last();
		if (!_force && *current == setReady)
		{
			return;
		}
		int idx = 0;
		setReadyForRendering.clear();
		VideoMatrixStackMode::Type matrixStackMode = videoMatrixStack->get_mode();
		for_every_const(planeCS, current->planes)
		{
			Plane planeToReady = *planeCS;
			if (matrixStackMode == VideoMatrixStackMode::xRight_yForward_zUp)
			{
				Video3D::ready_plane_for_video_x_right_y_forward_z_up(REF_ planeToReady);
			}
			setReadyForRendering.planes.push_back(planeToReady);
			++idx;
		}
		int const sent = idx;
		clipPlanesSent = sent;
		setReady = *current;
	}
	else
	{
		if (!_force && clipPlanesSent == 0)
		{
			return;
		}
		// clear all
		setReadyForRendering.clear();
		// resetting all will make it possible to start again with zero
		clipPlanesSent = 0;
		setReady.clear();
	}
}
