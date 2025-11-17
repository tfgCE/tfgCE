// NOTE: clear methods set up masks, as otherwise it could not clear properly

void Video3D::clear_colour(Colour const & _colour)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClearColor(_colour.r, _colour.g, _colour.b, _colour.a); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClear(GL_COLOR_BUFFER_BIT); AN_GL_CHECK_FOR_ERROR
	// store current state we set explicitly
	currentState.colourDisplay = true;
}

void Video3D::clear_depth(float _depth)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ glDepthMask(GL_TRUE); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
	DIRECT_GL_CALL_ glClearDepth((GLclampd)_depth); AN_GL_CHECK_FOR_ERROR
#else
#ifdef AN_GLES
	DIRECT_GL_CALL_ glClearDepthf(_depth); AN_GL_CHECK_FOR_ERROR
#else
	#error implement
#endif
#endif
	DIRECT_GL_CALL_ glClear(GL_DEPTH_BUFFER_BIT); AN_GL_CHECK_FOR_ERROR
	// store current state we set explicitly
	currentState.depthDisplay = true;
}

void Video3D::clear_stencil(int32 _stencil)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ glClearStencil(_stencil); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClear(GL_STENCIL_BUFFER_BIT); AN_GL_CHECK_FOR_ERROR
}

void Video3D::clear_colour_depth(Colour const & _colour, float _depth)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glDepthMask(GL_TRUE); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glStencilMask(0xffffffff); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClearColor(_colour.r, _colour.g, _colour.b, _colour.a); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
	DIRECT_GL_CALL_ glClearDepth((GLclampd)_depth); AN_GL_CHECK_FOR_ERROR
#else
#ifdef AN_GLES
	DIRECT_GL_CALL_ glClearDepthf(_depth); AN_GL_CHECK_FOR_ERROR
#else
#error implement
#endif
#endif
	DIRECT_GL_CALL_ glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); AN_GL_CHECK_FOR_ERROR
	// store current state we set explicitly
	currentState.colourDisplay = true;
	currentState.depthDisplay = true;
	currentState.stencilDisplayMask = 0xffffffff;
}

void Video3D::clear_colour_depth_stencil(Colour const & _colour, float _depth, int32 _stencil)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glDepthMask(GL_TRUE); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glStencilMask(0xffffffff); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClearColor(_colour.r, _colour.g, _colour.b, _colour.a); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
	DIRECT_GL_CALL_ glClearDepth((GLclampd)_depth); AN_GL_CHECK_FOR_ERROR
#else
#ifdef AN_GLES
	DIRECT_GL_CALL_ glClearDepthf(_depth); AN_GL_CHECK_FOR_ERROR
#else
#error implement
#endif
#endif
	DIRECT_GL_CALL_ glClearStencil(_stencil); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); AN_GL_CHECK_FOR_ERROR
	// store current state we set explicitly
	currentState.colourDisplay = true;
	currentState.depthDisplay = true;
	currentState.stencilDisplayMask = 0xffffffff;
}

void Video3D::clear_depth_stencil(float _depth, int32 _stencil)
{
	assert_rendering_thread();
	DIRECT_GL_CALL_ glDepthMask(GL_TRUE); AN_GL_CHECK_FOR_ERROR
#ifdef AN_GL
	DIRECT_GL_CALL_ glClearDepth((GLclampd)_depth); AN_GL_CHECK_FOR_ERROR
#else
#ifdef AN_GLES
	DIRECT_GL_CALL_ glClearDepthf(_depth); AN_GL_CHECK_FOR_ERROR
#else
#error implement
#endif
#endif
	DIRECT_GL_CALL_ glClearStencil(_stencil); AN_GL_CHECK_FOR_ERROR
	DIRECT_GL_CALL_ glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); AN_GL_CHECK_FOR_ERROR
	// store current state we set explicitly
	currentState.depthDisplay = true;
}

void Video3D::mark_buffer_ready_to_display()
{
	an_assert(!bufferIsReadyToDisplay, TXT("buffer should be displayed before"));
	bufferIsReadyToDisplay = true;
}

void Video3D::mark_buffer_displayed()
{
	bufferIsReadyToDisplay = false;
}

void Video3D::display_buffer_if_ready()
{
	if (bufferIsReadyToDisplay)
	{
		display_buffer();
	}
}

void Video3D::activate_viewport()
{
	if (activeViewportLeftBottom != viewportLeftBottom ||
		activeViewportSize != viewportSize)
	{
		activeViewportLeftBottom = viewportLeftBottom;
		activeViewportSize = viewportSize;
		glViewport(activeViewportLeftBottom.x, activeViewportLeftBottom.y, activeViewportSize.x, activeViewportSize.y);
	}
}

void Video3D::set_viewport(LOCATION_ VectorInt2 const & _leftBottom, SIZE_ VectorInt2 const & _size, bool _activate)
{
	assert_rendering_thread();
	viewportLeftBottom = _leftBottom;
	viewportSize = _size;
	if (_activate)
	{
		activate_viewport();
	}
}

void Video3D::set_default_viewport(bool _activate)
{
	set_viewport(VectorInt2::zero, get_screen_size(), _activate);
}

void Video3D::set_3d_projection_matrix()
{
	isFor3d = true;
	set_3d_projection_matrix(projection_matrix(fov, get_aspect_ratio(), nearFarPlane.min, nearFarPlane.max));
}

void Video3D::set_2d_projection_matrix_ortho(Vector2 const & _size)
{
	isFor3d = false;
	Vector2 useSize = _size;
	if (useSize.is_zero())
	{
		useSize = viewportSize.to_vector2();
	}
	set_2d_projection_matrix(orthogonal_matrix_for_2d(Vector2::zero, useSize, nearFarPlane.min, nearFarPlane.max));
}

void Video3D::set_3d_projection_matrix(Matrix44 const & _mat)
{
	isFor3d = true;
	access_model_view_matrix_stack().set_mode(VideoMatrixStackMode::xRight_yForward_zUp);
	set_projection_matrix(_mat);
}

void Video3D::set_2d_projection_matrix(Matrix44 const & _mat)
{
	isFor3d = false;
	access_model_view_matrix_stack().set_mode(VideoMatrixStackMode::xRight_yUp);
	set_projection_matrix(_mat);
}

void Video3D::set_projection_matrix(Matrix44 const & _mat)
{
	assert_rendering_thread();
#ifdef AN_DEVELOPMENT_GL
	if (projectionMatrix != _mat)
	{
		load_matrix_for_rendering(VideoMatrixMode::Projection, _mat);
	}
#endif
	projectionMatrix = _mat;
}

void Video3D::depth_range(float _min, float _max)
{
	lazyState.depthRange.min = _min;
	lazyState.depthRange.max = _max;
}

void Video3D::send_lazy_depth_range_internal()
{
	assert_rendering_thread();
	if (currentState.depthRange.min != lazyState.depthRange.min ||
		currentState.depthRange.max != lazyState.depthRange.max)
	{
#ifdef AN_GL
		DIRECT_GL_CALL_ glDepthRange(lazyState.depthRange.min, lazyState.depthRange.max); AN_GL_CHECK_FOR_ERROR
#else
#ifdef AN_GLES
		DIRECT_GL_CALL_ glDepthRangef(lazyState.depthRange.min, lazyState.depthRange.max); AN_GL_CHECK_FOR_ERROR
#else
#error implement
#endif
#endif
		currentState.depthRange = lazyState.depthRange;
	}
}

void Video3D::depth_test(Video3DCompareFunc::Type _depthFunc)
{
	lazyState.depthFunc = _depthFunc;
}

void Video3D::depth_clamp(bool _depthClamp)
{
	lazyState.depthClamp = _depthClamp;
}

void Video3D::send_lazy_depth_test_internal()
{
	assert_rendering_thread();
	if (currentState.depthFunc != lazyState.depthFunc)
	{
		if (lazyState.depthFunc != Video3DCompareFunc::None)
		{
			if (currentState.depthFunc == Video3DCompareFunc::None)
			{
				DIRECT_GL_CALL_ glEnable(GL_DEPTH_TEST); AN_GL_CHECK_FOR_ERROR
			}
			DIRECT_GL_CALL_ glDepthFunc(lazyState.depthFunc); AN_GL_CHECK_FOR_ERROR
		}
		else if (currentState.depthFunc != Video3DCompareFunc::None && lazyState.depthFunc == Video3DCompareFunc::None)
		{
			DIRECT_GL_CALL_ glDisable(GL_DEPTH_TEST); AN_GL_CHECK_FOR_ERROR
		}
		currentState.depthFunc = lazyState.depthFunc;
	}
}

void Video3D::send_lazy_depth_clamp_internal()
{
	assert_rendering_thread();
	if (currentState.depthClamp != lazyState.depthClamp)
	{
#ifdef AN_GL
		int const glDeathClamp = GL_DEPTH_CLAMP;
#endif
#ifdef AN_GLES
		int const glDeathClamp = 0x864F; // it's the same but kept as a reference from where did we take it
#endif
		if (lazyState.depthClamp)
		{
			DIRECT_GL_CALL_ glEnable(glDeathClamp); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			DIRECT_GL_CALL_ glDisable(glDeathClamp); AN_GL_CHECK_FOR_ERROR
		}
		currentState.depthClamp = lazyState.depthClamp;
	}
}

void Video3D::stencil_test(Video3DCompareFunc::Type _stencilFunc, int32 _stencilFuncRef, uint32 _stencilFuncMask)
{
	lazyState.stencilFunc = _stencilFunc;
	lazyState.stencilFuncRef = _stencilFuncRef;
	lazyState.stencilFuncMask = _stencilFuncMask;
}

void Video3D::send_lazy_stencil_test_internal()
{
	assert_rendering_thread();
	if (currentState.stencilFunc != lazyState.stencilFunc ||
		currentState.stencilFuncRef != lazyState.stencilFuncRef ||
		currentState.stencilFuncMask != lazyState.stencilFuncMask)
	{
		if (lazyState.stencilFunc != Video3DCompareFunc::None)
		{
			if (currentState.stencilFunc == Video3DCompareFunc::None)
			{
				DIRECT_GL_CALL_ glEnable(GL_STENCIL_TEST); AN_GL_CHECK_FOR_ERROR
			}
			DIRECT_GL_CALL_ glStencilFunc(lazyState.stencilFunc, lazyState.stencilFuncRef, lazyState.stencilFuncMask); AN_GL_CHECK_FOR_ERROR
		}
		else if (currentState.stencilFunc != Video3DCompareFunc::None && lazyState.stencilFunc == Video3DCompareFunc::None)
		{
			DIRECT_GL_CALL_ glDisable(GL_STENCIL_TEST); AN_GL_CHECK_FOR_ERROR
		}
		currentState.stencilFunc = lazyState.stencilFunc;
		currentState.stencilFuncRef = lazyState.stencilFuncRef;
		currentState.stencilFuncMask = lazyState.stencilFuncMask;
	}
}

void Video3D::stencil_op(Video3DOp::Type _bothPassed, Video3DOp::Type _depthTestFails, Video3DOp::Type _stencilTestFails)
{
	lazyState.stencilOpBothPassed = _bothPassed;
	lazyState.stencilOpDepthTestFails = _depthTestFails;
	lazyState.stencilOpStencilTestFails = _stencilTestFails;
}

void Video3D::send_lazy_stencil_op_internal()
{
	assert_rendering_thread();
	if (currentState.stencilOpBothPassed != lazyState.stencilOpBothPassed ||
		currentState.stencilOpDepthTestFails != lazyState.stencilOpDepthTestFails ||
		currentState.stencilOpStencilTestFails != lazyState.stencilOpStencilTestFails)
	{
		DIRECT_GL_CALL_ glStencilOp(lazyState.stencilOpStencilTestFails, lazyState.stencilOpDepthTestFails, lazyState.stencilOpBothPassed); AN_GL_CHECK_FOR_ERROR
		currentState.stencilOpBothPassed = lazyState.stencilOpBothPassed;
		currentState.stencilOpDepthTestFails = lazyState.stencilOpDepthTestFails;
		currentState.stencilOpStencilTestFails = lazyState.stencilOpStencilTestFails;
	}
}

void Video3D::depth_mask(bool _display)
{
	lazyState.depthDisplay = _display;
}

void Video3D::send_lazy_depth_mask_internal()
{
	assert_rendering_thread();
	if (currentState.depthDisplay != lazyState.depthDisplay)
	{
		DIRECT_GL_CALL_ glDepthMask(lazyState.depthDisplay ? GL_TRUE : GL_FALSE); AN_GL_CHECK_FOR_ERROR
		currentState.depthDisplay = lazyState.depthDisplay;
	}
}

void Video3D::colour_mask(bool _display)
{
	lazyState.colourDisplay = _display;
}

void Video3D::send_lazy_colour_mask_internal()
{
	assert_rendering_thread();
	if (currentState.colourDisplay != lazyState.colourDisplay)
	{
		GLboolean glDisplay = lazyState.colourDisplay ? GL_TRUE : GL_FALSE;
		DIRECT_GL_CALL_ glColorMask(glDisplay, glDisplay, glDisplay, glDisplay); AN_GL_CHECK_FOR_ERROR
		currentState.colourDisplay = lazyState.colourDisplay;
	}
}

void Video3D::stencil_mask(uint32 _mask)
{
	lazyState.stencilDisplayMask = _mask;
}

void Video3D::send_lazy_stencil_mask_internal()
{
	assert_rendering_thread();
	if (currentState.stencilDisplayMask != lazyState.stencilDisplayMask)
	{
		DIRECT_GL_CALL_ glStencilMask(lazyState.stencilDisplayMask); AN_GL_CHECK_FOR_ERROR
		currentState.stencilDisplayMask = lazyState.stencilDisplayMask;
	}
}

void Video3D::requires_send_use_textures()
{
	// as textures might be applied to shader program, send shader program first
	send_lazy_shader_program_internal();
	send_lazy_textures_internal();
}

void Video3D::send_lazy_textures_internal()
{
	auto currentTextureID = currentTexturesID.begin();
	int textureSlot = 0;
	for_every(lazyTextureID, lazyTexturesID)
	{
		if (*lazyTextureID != *currentTextureID)
		{
			send_lazy_use_texture_internal(textureSlot, *lazyTextureID);
		}
		++textureSlot;
		++currentTextureID;
	}
}

void Video3D::mark_use_texture(int _textureSlot, TextureID _textureID)
{
	assert_rendering_thread();
	lazyTexturesID[_textureSlot] = _textureID;
	return;
}

void Video3D::force_send_use_texture_slot(int _textureSlot)
{
	assert_rendering_thread();
	activeTextureSlot = _textureSlot;
	DIRECT_GL_CALL_ glActiveTexture(GL_TEXTURE0 + activeTextureSlot); AN_GL_CHECK_FOR_ERROR
	return;
}

void Video3D::send_lazy_use_texture_internal(int _textureSlot, TextureID _textureID)
{
	assert_rendering_thread();
	TextureID & currentTextureID = currentTexturesID[_textureSlot];
	if (_textureID == currentTextureID)
	{
		return;
	}
	// switch slot
	if (activeTextureSlot != _textureSlot)
	{
		activeTextureSlot = _textureSlot;
		DIRECT_GL_CALL_ glActiveTexture(GL_TEXTURE0 + activeTextureSlot); AN_GL_CHECK_FOR_ERROR
	}
	/*
	only makes sense for fixed-function pipeline?
#ifdef AN_GL
	if (_textureID && !currentTextureID)
	{
		DIRECT_GL_CALL_ glEnable(GL_TEXTURE_2D); AN_GL_CHECK_FOR_ERROR
	}
	else if (!_textureID && currentTextureID)
	{
		DIRECT_GL_CALL_ glDisable(GL_TEXTURE_2D); AN_GL_CHECK_FOR_ERROR
	}
#endif
	*/

	// set only if needed, if we disable, then it is disabled
	if (_textureID)
	{
		DIRECT_GL_CALL_ glBindTexture(GL_TEXTURE_2D, _textureID); AN_GL_CHECK_FOR_ERROR
	}
	currentTextureID = _textureID;
}

void Video3D::mark_use_texture_if_not_set(int _textureSlot, TextureID _textureID)
{
	TextureID & lazyTextureID = lazyTexturesID[_textureSlot];
	if (lazyTextureID == texture_id_none())
	{
		lazyTextureID = _textureID;
	}
	return;
}

void Video3D::mark_use_no_textures()
{
	for_count(int, slot, currentTexturesID.get_size())
	{
		mark_use_texture(slot, texture_id_none());
	}
}

void Video3D::push_state()
{
	stateStack.push_back(lazyState);
}

void Video3D::pop_state()
{
	lazyState = stateStack.get_last();
	stateStack.pop_back();
}

void Video3D::invalidate_pop_variables()
{
	variable_invalidate(currentState);
	variable_invalidate(lazyState);
}

void Video3D::front_face_order(FaceOrder::Type _type)
{
	lazyState.frontFaceOrder = _type;
}

void Video3D::send_lazy_front_face_order_internal()
{
	assert_rendering_thread();
	if (currentState.frontFaceOrder != lazyState.frontFaceOrder)
	{
		DIRECT_GL_CALL_ glFrontFace(lazyState.frontFaceOrder); AN_GL_CHECK_FOR_ERROR
		currentState.frontFaceOrder = lazyState.frontFaceOrder;
	}
}

void Video3D::face_display(FaceDisplay::Type _display)
{
	lazyState.faceDisplay = _display;
}

void Video3D::send_lazy_face_display_internal()
{
	assert_rendering_thread();
	if (currentState.faceDisplay != lazyState.faceDisplay)
	{
		if (currentState.faceDisplay != FaceDisplay::Both && lazyState.faceDisplay == FaceDisplay::Both)
		{
			DIRECT_GL_CALL_ glDisable(GL_CULL_FACE); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			if (currentState.faceDisplay == FaceDisplay::Both)
			{
				DIRECT_GL_CALL_ glEnable(GL_CULL_FACE); AN_GL_CHECK_FOR_ERROR
			}
			DIRECT_GL_CALL_ glCullFace(lazyState.faceDisplay); AN_GL_CHECK_FOR_ERROR
		}
		currentState.faceDisplay = lazyState.faceDisplay;
	}
}

void Video3D::setup_for_2d_display()
{
	assert_rendering_thread();
	depth_test();
	stencil_test();
	depth_mask(false);
	colour_mask(true);
	front_face_order(FaceOrder::CW);
	face_display(FaceDisplay::Both);
	// disabled to allow translucency
	//mark_disable_blend();
	requires_send_state();
}

void Video3D::setup_for_3d_display()
{
	assert_rendering_thread();
	depth_test(Video3DCompareFunc::Less);
	stencil_test();
	depth_mask();
	colour_mask(true);
	front_face_order(FaceOrder::CW);
	face_display(FaceDisplay::Front);
	// disabled to allow translucency
	//mark_disable_blend();
	requires_send_state();
}

void Video3D::ready_matrix_for_video_x_right_y_forward_z_up(REF_ Matrix44 & _mat)
{
	// this works for x right, y forward, z up
	// transform to open gl
	swap(_mat.m01, _mat.m02);
	swap(_mat.m11, _mat.m12);
	swap(_mat.m21, _mat.m22);
	swap(_mat.m31, _mat.m32);
	// reverse z axis -z is forward
	_mat.m02 = -_mat.m02;
	_mat.m12 = -_mat.m12;
	_mat.m22 = -_mat.m22;
	_mat.m32 = -_mat.m32;
}

void Video3D::ready_plane_for_video_x_right_y_forward_z_up(REF_ Plane & _plane)
{
	swap(_plane.normal.y, _plane.normal.z);
	_plane.normal.z = -_plane.normal.z;
	swap(_plane.anchor.y, _plane.anchor.z);
	_plane.anchor.z = -_plane.anchor.z;
}

void Video3D::requires_send_shader_program()
{
	send_lazy_shader_program_internal();
}

void Video3D::send_lazy_shader_program_internal()
{
	if (currentShaderProgramId != lazyShaderProgramId)
	{
		send_lazy_bind_shader_program_internal(lazyShaderProgramId, lazyShaderProgram);
	}
}

void Video3D::bind_shader_program(ShaderProgramID _program, ShaderProgram* _shaderProgram)
{
	an_assert(lazyShaderProgramId == 0, TXT("shader should not be bound"));
	lazyShaderProgramId = _program;
	lazyShaderProgram = _shaderProgram;
}

void Video3D::send_lazy_bind_shader_program_internal(ShaderProgramID _program, ShaderProgram* _shaderProgram)
{
	assert_rendering_thread();
	currentShaderProgramId = _program;
	currentShaderProgram = _shaderProgram;
	DIRECT_GL_CALL_ glUseProgram(_program); AN_GL_CHECK_FOR_ERROR
}

void Video3D::unbind_shader_program(ShaderProgramID _program)
{
	an_assert(lazyShaderProgramId == _program, TXT("same shader should be bound to unbind"));
	lazyShaderProgramId = 0;
	lazyShaderProgram = nullptr;
}

void Video3D::send_lazy_unbind_shader_program_internal(ShaderProgramID _program)
{
	currentShaderProgramId = 0;
	currentShaderProgram = nullptr;
	DIRECT_GL_CALL_ glUseProgram(0); AN_GL_CHECK_FOR_ERROR
}

void Video3D::invalidate_variables()
{
	/*
	not to be invalidated
	variable_invalidate(projectionMatrix);
	variable_invalidate(modelViewMatrixStack);
	variable_invalidate(currentShaderProgramId);
	variable_invalidate(activeTextureSlot);
	variable_invalidate(viewportLeftBottom);
	variable_invalidate(viewportSize);
	variable_invalidate(nearFarPlane);
	*/

	variable_invalidate(currentState);
	variable_invalidate(lazyState);
}

void Video3D::ready_for_rendering()
{
	requires_send_all();
}

void Video3D::requires_send_all()
{
	requires_send_use_textures();
	requires_send_state();
	requires_send_shader_program();
	requires_send_vertex_buffers();
	modelViewMatrixStack.requires_send_all();
	clipPlaneStack.ready_current();
}

void Video3D::requires_send_state()
{
	send_lazy_depth_mask_internal();
	send_lazy_colour_mask_internal();
	send_lazy_stencil_mask_internal();
	send_lazy_depth_test_internal();
	send_lazy_depth_clamp_internal();
	send_lazy_depth_range_internal();
	send_lazy_stencil_test_internal();
	send_lazy_stencil_op_internal();
	send_lazy_front_face_order_internal();
	send_lazy_face_display_internal();
	if (lazyBlendEnabled)
	{
		send_lazy_enable_blend_internal(lazyBlendSrcFactor, lazyBlendDestFactor);
	}
	else
	{
		send_lazy_disable_blend_internal();
	}
	send_lazy_polygon_offset_internal();
}

void Video3D::mark_enable_blend(BlendOp::Type _srcFactor, BlendOp::Type _destFactor)
{
	lazyBlendEnabled = true;
	lazyBlendSrcFactor = _srcFactor;
	lazyBlendDestFactor = _destFactor;
}

void Video3D::mark_disable_blend()
{
	lazyBlendEnabled = false;
}

void Video3D::store_blend_mark(Video3DStateStore & _store) const
{
	_store.blendStored = true;
	_store.blendEnabled = lazyBlendEnabled;
	_store.blendSrcFactor = lazyBlendSrcFactor;
	_store.blendDestFactor = lazyBlendDestFactor;
}

void Video3D::store_polygon_offset_mark(Video3DStateStore & _store) const
{
	_store.polygonOffsetStored = true;
	_store.polygonOffsetEnabled = lazyPolygonOffsetEnabled;
	_store.polygonOffsetFactor = lazyPolygonOffsetFactor;
	_store.polygonOffsetUnits = lazyPolygonOffsetUnits;
}

void Video3D::send_lazy_polygon_offset_internal()
{
	if (polygonOffsetEnabled != lazyPolygonOffsetEnabled)
	{
		polygonOffsetEnabled = lazyPolygonOffsetEnabled;
		if (polygonOffsetEnabled)
		{
			DIRECT_GL_CALL_ glEnable(GL_POLYGON_OFFSET_FILL); AN_GL_CHECK_FOR_ERROR
		}
		else
		{
			DIRECT_GL_CALL_ glDisable(GL_POLYGON_OFFSET_FILL); AN_GL_CHECK_FOR_ERROR
			DIRECT_GL_CALL_ glPolygonOffset(0.0f, 0.0f); AN_GL_CHECK_FOR_ERROR
		}
	}
	if (polygonOffsetEnabled)
	{
		if (polygonOffsetFactor != lazyPolygonOffsetFactor ||
			polygonOffsetUnits != lazyPolygonOffsetUnits)
		{
			polygonOffsetFactor = lazyPolygonOffsetFactor;
			polygonOffsetUnits = lazyPolygonOffsetUnits;
			DIRECT_GL_CALL_ glPolygonOffset(polygonOffsetFactor, polygonOffsetUnits); AN_GL_CHECK_FOR_ERROR
		}
	}
}

void Video3D::mark_enable_polygon_offset(float _factor, float _units)
{
	lazyPolygonOffsetEnabled = true;
	lazyPolygonOffsetFactor = _factor;
	lazyPolygonOffsetUnits = _units;
}

void Video3D::mark_disable_polygon_offset()
{
	lazyPolygonOffsetEnabled = false;
}

void Video3D::restore(Video3DStateStore const & _store)
{
	if (_store.blendStored)
	{
		lazyBlendEnabled = _store.blendEnabled;
		lazyBlendSrcFactor = _store.blendSrcFactor;
		lazyBlendDestFactor = _store.blendDestFactor;
	}
	if (_store.depthDisplayStored)
	{
		depth_mask(_store.depthDisplay);
	}
	if (_store.depthFuncStored)
	{
		depth_test(_store.depthFunc);
	}
	if (_store.depthClampStored)
	{
		depth_clamp(_store.depthClamp);
	}
	if (_store.polygonOffsetStored)
	{
		lazyPolygonOffsetEnabled = _store.polygonOffsetEnabled;
		lazyPolygonOffsetFactor = _store.polygonOffsetFactor;
		lazyPolygonOffsetUnits = _store.polygonOffsetUnits;
	}
	//requires_send_all(); // <- it might be not needed at all
}

void Video3D::store_depth_mask(Video3DStateStore & _store) const
{
	_store.depthDisplayStored = true;
	_store.depthDisplay = lazyState.depthDisplay;
}

void Video3D::store_depth_test(Video3DStateStore & _store) const
{
	_store.depthFuncStored = true;
	_store.depthFunc = lazyState.depthFunc;
}

void Video3D::store_depth_clamp(Video3DStateStore & _store) const
{
	_store.depthClampStored = true;
	_store.depthClamp = lazyState.depthClamp;
}

void Video3D::send_lazy_enable_blend_internal(BlendOp::Type _srcFactor, BlendOp::Type _destFactor)
{
	if (! blendEnabled)
	{
		blendEnabled = true;
		DIRECT_GL_CALL_ glEnable(GL_BLEND); AN_GL_CHECK_FOR_ERROR
	}
	if (_srcFactor != blendSrcFactor ||
		_destFactor != blendDestFactor)
	{
		blendSrcFactor = _srcFactor;
		blendDestFactor = _destFactor;
		DIRECT_GL_CALL_ glBlendFunc(blendSrcFactor, blendDestFactor); AN_GL_CHECK_FOR_ERROR
	}
}

void Video3D::send_lazy_disable_blend_internal()
{
	if (blendEnabled)
	{
		blendEnabled = false;
		DIRECT_GL_CALL_ glDisable(GL_BLEND); AN_GL_CHECK_FOR_ERROR
	}
}

#ifdef AN_GL
void Video3D::load_matrix_for_rendering(VideoMatrixMode::Type _mode, Matrix44 const & _matrix)
{
#ifdef AN_DEVELOPMENT
	if (! loadMatrices)
	{
		return;
	}
	if (loadMatrixMode != _mode)
	{
		loadMatrixMode = _mode;
		DIRECT_GL_CALL_ glMatrixMode(loadMatrixMode); AN_GL_CHECK_FOR_ERROR
	}
	DIRECT_GL_CALL_ glLoadMatrixf(&_matrix.m00); AN_GL_CHECK_FOR_ERROR
#endif
}
#endif

void Video3D::set_multi_sample(bool _enable)
{
	if (multiSampleEnabled.is_set() &&
		multiSampleEnabled.get() == _enable)
	{
		return;
	}
	multiSampleEnabled = _enable;
#ifdef AN_GL
	if (_enable)
	{
		DIRECT_GL_CALL_ glEnable(GL_MULTISAMPLE); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		DIRECT_GL_CALL_ glDisable(GL_MULTISAMPLE); AN_GL_CHECK_FOR_ERROR
	}
#endif
}

void Video3D::set_srgb_conversion(bool _enable)
{
	// always set it, I have no idea why, it might be due to resetting this value when binding framebuffer
	/*
	if (sRGBConversion.is_set() &&
		sRGBConversion.get() == _enable)
	{
		return;
	}
	*/
	sRGBConversion = _enable;
	if (_enable)
	{
		DIRECT_GL_CALL_ glEnable(GL_FRAMEBUFFER_SRGB); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		DIRECT_GL_CALL_ glDisable(GL_FRAMEBUFFER_SRGB); AN_GL_CHECK_FOR_ERROR
	}
}

void Video3D::invalidate_vertex_format(VertexFormat const * _vertexFormat)
{
	if (boundBuffersAndVertexFormat.vertexFormat == _vertexFormat)
	{
		unbind_and_send_buffers_and_vertex_format();
		boundBuffersAndVertexFormat.vertexFormatValid = false;
	}
}

void Video3D::forget_all_buffer_data()
{
#ifdef AN_GLES
	assert_rendering_thread();
	const GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 3, attachments);
#endif
}
void Video3D::forget_render_buffer_data()
{
	assert_rendering_thread();
#ifdef AN_GLES
	const GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 };
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, attachments);
#endif
}

void Video3D::forget_depth_buffer_data()
{
#ifdef AN_GLES
	assert_rendering_thread();
	const GLenum attachments[1] = { GL_DEPTH_ATTACHMENT };
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, attachments);
#endif
}

void Video3D::forget_stencil_buffer_data()
{
#ifdef AN_GLES
	assert_rendering_thread();
	const GLenum attachments[1] = { GL_STENCIL_ATTACHMENT };
	glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 1, attachments);
#endif
}

void Video3D::forget_depth_stencil_buffer_data()
{
#ifdef AN_GLES
	assert_rendering_thread();
	if (multiSampleEnabled.is_set() && multiSampleEnabled.get())
	{
		const GLenum attachments[2] = { GL_DEPTH_ATTACHMENT , GL_STENCIL_ATTACHMENT };
		DIRECT_GL_CALL_ VideoGLExtensions::get().glDiscardFramebufferEXT(GL_DRAW_FRAMEBUFFER, 2, attachments); AN_GL_CHECK_FOR_ERROR
	}
	else
	{
		const GLenum attachments[2] = { GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
		DIRECT_GL_CALL_ glInvalidateFramebuffer(GL_DRAW_FRAMEBUFFER, 2, attachments); AN_GL_CHECK_FOR_ERROR
	}
#endif
}

RangeInt2 const& Video3D::get_scissors() const
{
	return scissors;
}

void Video3D::set_scissors(RangeInt2 const& _region)
{
	if (_region.is_empty())
	{
		clear_scissors();
	}
	else
	{
		glEnable(GL_SCISSOR_TEST);
		scissors = _region;
		glScissor(scissors.x.min, scissors.y.min, scissors.x.length(), scissors.y.length());
	}
}

void Video3D::clear_scissors()
{
	glDisable(GL_SCISSOR_TEST);
	scissors = RangeInt2::empty;
}

