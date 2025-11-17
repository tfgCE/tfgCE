#pragma once

#include "video3d.h"
#include "shaderProgramInstance.h"
#include "material.h"

namespace System
{
	/**
	 *	It's basically a wrapper around shader program instance with dependence on base material
	 *
	 *	This is instance of material with separate instance of shader program.
	 *
	 *	If uniform is set to this, it modifies only shader program instance.
	 */
	struct MaterialInstance
	{
		static bool do_match(MaterialInstance const & _a, MaterialInstance const & _b);

		MaterialInstance();

		void set_material(Material* _material, MaterialShaderUsage::Type _materialUsage);
		Material* get_material() const { return material; }
		MaterialShaderUsage::Type get_material_usage() const { return materialUsage; }
		void fill_missing();

		Optional<int> const & get_rendering_order_priority_offset() const { return renderingOrderPriorityOffset; }
		void set_rendering_order_priority_offset(Optional<int> const& _renderingOrderPriorityOffset) { renderingOrderPriorityOffset = _renderingOrderPriorityOffset; }

		ShaderProgramInstance const * get_shader_program_instance() const { return &shaderProgramInstance; }
		
		void hard_copy_from(MaterialInstance const & _instance, Optional<MaterialShaderUsage::Type> const & _forceMaterialUsage = NP); // without filling
		void hard_copy_from_with_filled_missing(MaterialInstance const & _instance, Optional<MaterialShaderUsage::Type> const& _forceMaterialUsage = NP);

		void clear(MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void clear_uniform(Name const & _name, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, ShaderParam const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, TextureID _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, int32 _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, float _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Array<float> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Array<Vector3> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Array<Vector4> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Array<Colour> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, ArrayStack<float> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, ArrayStack<Vector3> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, ArrayStack<Vector4> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, ArrayStack<Colour> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Vector2 const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Vector3 const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Vector4 const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Matrix33 const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Matrix44 const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Array<Matrix44> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, ArrayStack<Matrix44> const & _v, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);
		void set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, MaterialSetting::Type _settingMaterial = MaterialSetting::Default);

		void log(LogInfoContext & _log) const;

	private:
		Material* material;
		MaterialShaderUsage::Type materialUsage;
		ShaderProgramInstance shaderProgramInstance;
		Optional<int> renderingOrderPriorityOffset;

		MaterialInstance const & operator = (MaterialInstance const & _other); // do not implement_

		void hard_copy_from_internal(MaterialInstance const& _instance, Optional<MaterialShaderUsage::Type> const& _forceMaterialUsage, bool _fillMissing);
	};

};
