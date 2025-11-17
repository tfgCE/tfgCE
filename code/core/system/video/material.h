#pragma once

#include "video3d.h"
#include "shaderProgram.h"
#include "videoEnums.h"

#include "..\..\tags\tag.h"

namespace System
{
	/**
	 *	Material is pairs of shader program and shader program instance. Instances are default/basic material setup.
	 *
	 *	Mutliple instances are for different material usage.
	 *
	 *	If uniform is set to this, it modifies default material's instances.
	 */
	class Material
	{
	public:
		Material();
		~Material();

		void set_shader(MaterialShaderUsage::Type _usage, ShaderProgram* _shaderProgram);
		ShaderProgram * get_shader_program(MaterialShaderUsage::Type _usage);
		ShaderProgramInstance & access_shader_program_instance(MaterialShaderUsage::Type _usage);
		ShaderProgramInstance const & get_shader_program_instance(MaterialShaderUsage::Type _usage) const;

		void set_parent(Material* material);
		Material const * get_parent() const { return parent; }

		void allow_individual_instances(bool _allowIndividualInstances) { allowIndividualInstances = _allowIndividualInstances; }
		bool are_individual_instances_allowed() const { return allowIndividualInstances; }

		void clear();
		void clear_uniform(Name const & _name);
		void set_uniform(Name const & _name, ShaderParam const & _v, bool _force = false);
		void set_uniform(Name const & _name, TextureID _v, bool _force = false);
		void set_uniform(Name const & _name, int32 _v, bool _force = false);
		void set_uniform(Name const & _name, float _v, bool _force = false);
		void set_uniform(Name const & _name, Vector2 const & _v, bool _force = false);
		void set_uniform(Name const & _name, Vector4 const & _v, bool _force = false);
		void set_uniform(Name const & _name, Matrix44 const & _v, bool _force = false);
		void set_uniform(Name const & _name, Array<Matrix44> const & _v, bool _force = false);
		void set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, bool _force = false);

		void set_tags(Tags const & _tags) { tags = _tags; }
		Tags & access_tags() { return tags; }
		Tags const & get_tags() const { return tags; }

		bool load_from_xml(IO::XML::Node const * _node);

		Optional<System::FaceDisplay::Type> const & get_face_display() const { return faceDisplay.is_set() || ! parent? faceDisplay : parent->get_face_display(); }
		void set_src_blend_op(Optional<System::BlendOp::Type> const& _op) { srcBlendOp = _op; update_any_blending(); }
		void set_dest_blend_op(Optional<System::BlendOp::Type> const & _op) { destBlendOp = _op; update_any_blending(); }
		Optional<System::BlendOp::Type> const & get_src_blend_op() const { return srcBlendOp.is_set() || !parent ? srcBlendOp : parent->get_src_blend_op(); }
		Optional<System::BlendOp::Type> const & get_dest_blend_op() const { return destBlendOp.is_set() || !parent ? destBlendOp : parent->get_dest_blend_op(); }
		bool does_any_blending() const { return anyBlending || (parent && parent->does_any_blending()); }
		Optional<bool> const & get_depth_mask() const { return depthMask.is_set() || !parent ? depthMask : parent->get_depth_mask(); }
		Optional<bool> const & get_depth_clamp() const { return depthClamp.is_set() || !parent ? depthClamp : parent->get_depth_clamp(); }
		Optional<Video3DCompareFunc::Type> const & get_depth_func() const { return depthFunc.is_set() || !parent ? depthFunc : parent->get_depth_func(); }
		Optional<float> const & get_polygon_offset() const { return polygonOffset.is_set() || !parent ? polygonOffset : parent->get_polygon_offset(); }

		Optional<int> const & get_rendering_order_priority() const { return renderingOrderPriority.is_set() || !parent ? renderingOrderPriority : parent->get_rendering_order_priority(); }
	private:
		Tags tags; // to allow to find per tag and other options (Framework uses this
		Material* parent = nullptr;
		bool allowIndividualInstances = false; // allows uniforms to be set by anything else than material itself
		RefCountObjectPtr<ShaderProgram> shaderPrograms[MaterialShaderUsage::Num];
		ShaderProgramInstance shaderProgramInstances[MaterialShaderUsage::Num];

		Optional<System::FaceDisplay::Type> faceDisplay;
		Optional<System::BlendOp::Type> srcBlendOp;
		Optional<System::BlendOp::Type> destBlendOp;
		CACHED_ bool anyBlending = false;
		Optional<bool> depthMask;
		Optional<bool> depthClamp;
		Optional<Video3DCompareFunc::Type> depthFunc;
		Optional<float> polygonOffset;
		Optional<int> renderingOrderPriority; // will override_ mesh's rendering order priority

		void set_shader_internal(MaterialShaderUsage::Type _usage, ShaderProgram* _shaderProgram);

		void update_any_blending();

		bool internal_load_from_xml(IO::XML::Node const* _node, bool _nested);
	};

};
