#define USE_OFFSET(_pointer, _offset) (void*)(((int8*)_pointer) + _offset)

void Mesh3DRenderingUtils::render_worker_begin(::System::Video3D * _v3d, ::System::Video3DStateStore & _stateStore, ::System::ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext, void const * _dataPointer, Primitive::Type _primitiveType, uint32 _numberOfVertices, ::System::VertexFormat const & _vertexFormat, ::System::BufferObjectID* _vbo, ::System::BufferObjectID* _ebo)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(renderPart_begin);
#endif

	an_assert(_vertexFormat.is_ok_to_be_used(), TXT("stride and offsets not calculated"));
	assert_rendering_thread();

	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE(renderPart_begin_setupStates);
#endif
		if (!_renderingContext.useExistingFaceDisplay)
		{
			_v3d->front_face_order(::System::FaceOrder::CW);
			::System::FaceDisplay::Type faceDisplay = ::System::FaceDisplay::Front;
			if (_usingMaterial)
			{
				auto const & mfd = _usingMaterial->get_face_display();
				if (mfd.is_set())
				{
					faceDisplay = mfd.get();
				}
			}
			_v3d->face_display(faceDisplay);
		}

		if (!_renderingContext.useExistingBlend)
		{
			::System::BlendOp::Type srcBlendOp = ::System::BlendOp::None;
			::System::BlendOp::Type destBlendOp = ::System::BlendOp::None;
			if (_usingMaterial)
			{
				auto const & msbo = _usingMaterial->get_src_blend_op();
				auto const & mdbo = _usingMaterial->get_dest_blend_op();
				if (msbo.is_set())
				{
					srcBlendOp = msbo.get();
				}
				if (mdbo.is_set())
				{
					destBlendOp = mdbo.get();
				}
			}
			// enable blend only when it makes sense
			if ((srcBlendOp != ::System::BlendOp::None || destBlendOp != ::System::BlendOp::None))
			{
				_v3d->store_blend_mark(_stateStore);
				if (!(srcBlendOp == ::System::BlendOp::One && destBlendOp == ::System::BlendOp::Zero))
				{
					_v3d->mark_enable_blend(srcBlendOp, destBlendOp);
					_v3d->store_polygon_offset_mark(_stateStore);
					_v3d->mark_enable_polygon_offset(-0.1f, -0.1f);
				}
				else
				{
					_v3d->mark_disable_blend();
				}
			}
		}

		if (_usingMaterial &&
			_usingMaterial->get_depth_mask().is_set())
		{
			_v3d->store_depth_mask(_stateStore);
			_v3d->depth_mask(_usingMaterial->get_depth_mask().get());
		}

		if (_usingMaterial &&
			_usingMaterial->get_depth_clamp().is_set())
		{
			_v3d->store_depth_clamp(_stateStore);
			_v3d->depth_clamp(_usingMaterial->get_depth_clamp().get());
		}

		if (_usingMaterial &&
			_usingMaterial->get_depth_func().is_set())
		{
			_v3d->store_depth_test(_stateStore);
			_v3d->depth_test(_usingMaterial->get_depth_func().get());
		}

		if (_usingMaterial &&
			_usingMaterial->get_polygon_offset().is_set())
		{
			_v3d->store_polygon_offset_mark(_stateStore);
			float po = _usingMaterial->get_polygon_offset().get();
			_v3d->mark_enable_polygon_offset(po, po);
		}
	}

	// set uniforms before binding material!
	// skinning
	if (_vertexFormat.has_skinning_data() && _shaderProgramInstance)
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE(renderPart_begin_setupSkinning);
#endif
		::System::ShaderProgramInstance const * shaderProgramInstance = _shaderProgramInstance;
		an_assert(shaderProgramInstance, TXT("no shader program instance associated with material"));
		::System::ShaderProgram const * shaderProgram = shaderProgramInstance->get_shader_program();
		an_assert(shaderProgram, TXT("no shader program associated with material"));
		if (shaderProgram && // yeah, I know but maybe for some reason we don't have it?
			shaderProgram->does_support_skinning())
		{
			if (_renderingContext.meshBonesProvider)
			{
				int bonesCount;
				Matrix44 const * bones = _renderingContext.meshBonesProvider->get_render_bone_matrices(OUT_ bonesCount);
				if (bonesCount)
				{
					shaderProgramInstance->set_uniform(shaderProgram->get_in_skinning_bones_uniform_index(), bones, bonesCount, true); // force it as we expect bones to always change
				}
				else
				{
					warn(TXT("there is render bones provider, but it doesn't provide any bones!"));
				}
			}
			else
			{
				an_assert(false, TXT("no mesh bones provider, from where can I get bones for skinning?"));
			}
		}
	}
	else
	{
		// we should always have shaders?
		/*
		if (_materialInstance)
		{
			an_assert(_materialInstance->get_material_usage() != ::System::MaterialShaderUsage::Skinned &&
				   _materialInstance->get_material_usage() != ::System::MaterialShaderUsage::SkinnedToSingleBone, TXT("material is for skinning but there is no skinning data in mesh"));
		}
		*/
	}

	::System::ShaderProgram * shaderProgram = nullptr;
	if (_shaderProgramInstance)
	{
		{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
			MEASURE_PERFORMANCE(renderPart_begin_bindShader);
#endif
			_shaderProgramInstance->bind(_renderingContext.shaderProgramBindingContext, _renderingContext.setup_shader_program_on_bound);
		}
		::System::ShaderProgramInstance const * shaderProgramInstance = _shaderProgramInstance;
		an_assert(shaderProgramInstance, TXT("no shader program instance associated with material"));
		shaderProgram = shaderProgramInstance->get_shader_program();
		an_assert(shaderProgram, TXT("no shader program associated with material"));
	}
	{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
		MEASURE_PERFORMANCE(renderPart_begin_bindBuffer);
#endif
		_v3d->bind_and_send_buffers_and_vertex_format(_vbo ? *_vbo : 0, _ebo ? *_ebo : 0, & _vertexFormat, shaderProgram, _dataPointer);
	}
}

void Mesh3DRenderingUtils::render_worker_render_non_indexed(::System::Video3D * _v3d, Primitive::Type _primitiveType, uint32 _numberOfVertices)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(renderPart_nonIndexed);
#endif
	_v3d->ready_for_rendering();
#ifdef INSPECT_MESH_RENDERING
	output(TXT("   draw %i triangles (non indexed)"), vertex_count_to_primitive_count(_primitiveType, _numberOfVertices));
#endif
#ifdef ALLOW_SELECTIVE_RENDERING
	if (SelectiveRendering::renderNow)
#endif
	{
		DRAW_CALL_ DIRECT_GL_CALL_ glDrawArrays(_primitiveType, 0, _numberOfVertices); AN_GL_CHECK_FOR_ERROR
		DRAW_TRIANGLES(vertex_count_to_primitive_count(_primitiveType, _numberOfVertices));
	}
}

void Mesh3DRenderingUtils::render_worker_render_indexed(::System::Video3D * _v3d, Primitive::Type _primitiveType, uint32 _numberOfElements, ::System::IndexFormat const & _indexFormat)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(renderPart_indexed);
#endif
	_v3d->ready_for_rendering();
#ifdef INSPECT_MESH_RENDERING
	output(TXT("   draw %i triangles (indexed)"), vertex_count_to_primitive_count(_primitiveType, _numberOfElements));
#endif
#ifdef ALLOW_SELECTIVE_RENDERING
	if (SelectiveRendering::renderNow)
#endif
	{
		DRAW_CALL_ DIRECT_GL_CALL_ glDrawElements(_primitiveType, _numberOfElements, _indexFormat.get_element_size(), 0); AN_GL_CHECK_FOR_ERROR
		DRAW_TRIANGLES(vertex_count_to_primitive_count(_primitiveType, _numberOfElements));
	}
}

void Mesh3DRenderingUtils::render_worker_end(::System::Video3D * _v3d, ::System::Video3DStateStore const & _stateStore, ::System::ShaderProgramInstance const * _shaderProgramInstance, System::Material const * _usingMaterial, Mesh3DRenderingContext const & _renderingContext, void const * _dataPointer, Primitive::Type _primitiveType, uint32 _numberOfVertices, ::System::VertexFormat const & _vertexFormat, ::System::BufferObjectID* _vbo, ::System::BufferObjectID* _ebo)
{
#ifdef MEASURE_PERFORMANCE_RENDERING_DETAILED
	MEASURE_PERFORMANCE(renderPart_end);
#endif
	_v3d->restore(_stateStore);
	// disable
	_v3d->soft_unbind_buffers_and_vertex_format();
	if (_shaderProgramInstance)
	{
		_shaderProgramInstance->unbind();
	}
}

#undef USE_OFFSET
