#pragma once

#include "..\..\globalDefinitions.h"
#include "..\..\math\math.h"
#include "..\..\containers\arrayStack.h"
#include "..\..\containers\arrayStatic.h"

#include "video.h"

#include "videoFunctions.h"

class SimpleVariableStorage;

namespace System
{
	class ShaderProgram;
	class Video3D;
	class ShaderProgramBindingContext;

	namespace ShaderParamType
	{
		enum Type
		{
			notSet,
			texture,
			uniform1i,
			uniform1iArray,
			uniform1f,
			uniform1fArray,
			uniform2f,
			uniformVectors2f,
			uniform3f,
			uniformVectors3f,
			uniformVectorsStereo3f,
			uniform4i,
			uniform4f,
			uniformVectors4f,
			uniformVectorsPointer4f,
			uniformVectorsStereo4f,
			uniformMatrix3f,
			uniformMatrices3f,
			uniformMatricesPointer3f,
			uniformMatrix4f,
			uniformMatrices4f,
			uniformMatricesPointer4f,
			uniformMatricesStereo4f,
		};
	};
	struct ShaderParam
	{
		ShaderParamType::Type type;
		bool forced; // will mean that it is always set (but no checks are done)
		union
		{
			int32 valueI[32];
			float valueF[32];
		}; // to hold two matrices44 for stereo matrices
		TextureID textureID;
		Array<int8> valueD; // if valueDsize <= sizeof(float) * 32 then we use valueF
		int8 const * valueDptr;
		int32 valueDsize = 0; // no D

		static ShaderParam not_set() { return ShaderParam(ShaderParamType::notSet); }

		ShaderParam() : type(ShaderParamType::notSet), forced(false) {}
		explicit ShaderParam(ShaderParamType::Type _type) : type(_type), forced(false) {}
		explicit ShaderParam(TextureID _id) : type(ShaderParamType::texture), forced(false) { memory_copy(&textureID, &_id, sizeof(TextureID)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(int32 _v) : type(ShaderParamType::uniform1i), forced(false) { memory_copy(valueI, &_v, sizeof(int32)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Array<int32> const & _v) : type(ShaderParamType::uniform1iArray), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(float _v) : type(ShaderParamType::uniform1f), forced(false) { memory_copy(valueF, &_v, sizeof(float)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Array<float> const & _v) : type(ShaderParamType::uniform1fArray), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Vector2 const & _v) : type(ShaderParamType::uniform2f), forced(false) { memory_copy(valueF, &_v, sizeof(Vector2)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Array<Vector2> const & _v) : type(ShaderParamType::uniformVectors2f), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Vector3 const & _v) : type(ShaderParamType::uniform3f), forced(false) { memory_copy(valueF, &_v, sizeof(Vector3)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Vector3 const & _v0, Vector3 const & _v1, bool _forced = false) : type(ShaderParamType::uniformVectorsStereo3f), forced(false) { memory_copy(valueF, &_v0.x, sizeof(Vector3)); memory_copy(&valueF[3], &_v1.x, sizeof(Vector3)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Array<Vector3> const & _v) : type(ShaderParamType::uniformVectors3f), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Vector4 const & _v) : type(ShaderParamType::uniform4f), forced(false) { memory_copy(valueF, &_v, sizeof(Vector4)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Vector4 const * _v, int32 _count) : type(ShaderParamType::uniformVectorsPointer4f), forced(false) { valueDptr = (int8*)_v; valueDsize = _count * sizeof(Vector4); }
		explicit ShaderParam(Array<Vector4> const & _v) : type(ShaderParamType::uniformVectors4f), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Array<Colour> const & _v) : type(ShaderParamType::uniformVectors4f), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Vector4 const & _v0, Vector4 const & _v1, bool _forced = false) : type(ShaderParamType::uniformVectorsStereo4f), forced(false) { memory_copy(valueF, &_v0.x, sizeof(Vector4)); memory_copy(&valueF[4], &_v1.x, sizeof(Vector4)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(VectorInt4 const & _v) : type(ShaderParamType::uniform4i), forced(false) { memory_copy(valueI, &_v, sizeof(VectorInt4)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Matrix33 const & _v) : type(ShaderParamType::uniformMatrix3f), forced(false) { memory_copy(valueF, &_v, sizeof(Matrix33)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Matrix33 const * _v, int32 _count) : type(ShaderParamType::uniformMatricesPointer3f), forced(false) { valueDptr = (int8*)_v; valueDsize = _count * sizeof(Matrix33); }
		explicit ShaderParam(Array<Matrix33> const & _v) : type(ShaderParamType::uniformMatrices3f), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Matrix44 const & _v) : type(ShaderParamType::uniformMatrix4f), forced(false) { memory_copy(valueF, &_v, sizeof(Matrix44)); valueDsize = 0; /* no D */ }
		explicit ShaderParam(Array<Matrix44> const & _v) : type(ShaderParamType::uniformMatrices4f), forced(false) { set_uniform_data(_v.get_data_size(), _v.get_data()); }
		explicit ShaderParam(Matrix44 const * _v, int32 _count) : type(ShaderParamType::uniformMatricesPointer4f), forced(false) { valueDptr = (int8*)_v; valueDsize = _count * sizeof(Matrix44); }
		explicit ShaderParam(Matrix44 const & _v0, Matrix44 const & _v1, bool _forced = false) : type(ShaderParamType::uniformMatricesStereo4f), forced(false) { memory_copy(valueF, &_v0.m00, sizeof(Matrix44)); memory_copy(&valueF[16], &_v1.m00, sizeof(Matrix44)); valueDsize = 0; /* no D */ }

		// special case for textures as we want to know if param is texture or not - TODO - is that so actually?
		inline bool is_set() const { return type != ShaderParamType::notSet && (type != ShaderParamType::texture || textureID != texture_id_none()); }

		inline void clear() { type = ShaderParamType::notSet; }
		inline void set_uniform(TextureID _id, bool _forced = false) { type = ShaderParamType::texture; forced = _forced; memory_copy(&textureID, &_id, sizeof(TextureID)); valueDsize = 0; /* no D */ }
		inline void set_uniform(int32 _v, bool _forced = false) { type = ShaderParamType::uniform1i; forced = _forced; memory_copy(valueI, &_v, sizeof(int32)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<int32> const & _v, bool _forced = false) { type = ShaderParamType::uniform1iArray; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(ArrayStack<int32> const & _v, bool _forced = false) { type = ShaderParamType::uniform1iArray; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(float _v, bool _forced = false) { type = ShaderParamType::uniform1f; forced = _forced; memory_copy(valueF, &_v, sizeof(float)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<float> const & _v, bool _forced = false) { type = ShaderParamType::uniform1fArray; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(ArrayStack<float> const & _v, bool _forced = false) { type = ShaderParamType::uniform1fArray; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(Vector2 const & _v, bool _forced = false) { type = ShaderParamType::uniform2f; forced = _forced; memory_copy(valueF, &_v, sizeof(Vector2)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<Vector2> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors2f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(ArrayStack<Vector2> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors2f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(Vector2 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformVectors2f; forced = _forced; set_uniform_data(_count * sizeof(Vector2), _v); }
		inline void set_uniform(Vector3 const & _v, bool _forced = false) { type = ShaderParamType::uniform3f; forced = _forced; memory_copy(valueF, &_v, sizeof(Vector3)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Vector3 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformVectors3f; forced = _forced; set_uniform_data(_count * sizeof(Vector3), _v); }
		inline void set_uniform(Vector3 const & _v0, Vector3 const & _v1, bool _forced = false) { type = ShaderParamType::uniformVectorsStereo3f; forced = _forced; memory_copy(valueF, &_v0.x, sizeof(Vector3)); memory_copy(&valueF[3], &_v1.x, sizeof(Vector3)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<Vector3> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors3f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(ArrayStack<Vector3> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors3f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(Vector4 const & _v, bool _forced = false) { type = ShaderParamType::uniform4f; forced = _forced; memory_copy(valueF, &_v, sizeof(Vector4)); valueDsize = 0; /* no D */ }
		inline void set_uniform_ptr(Vector4 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformVectorsPointer4f; forced = _forced; valueDptr = (int8*)_v; valueDsize = _count * sizeof(Vector4); }
		inline void set_uniform(Vector4 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformVectors4f; forced = _forced; set_uniform_data(_count * sizeof(Vector4), _v); }
		inline void set_uniform(Vector4 const & _v0, Vector4 const & _v1, bool _forced = false) { type = ShaderParamType::uniformVectorsStereo4f; forced = _forced; memory_copy(valueF, &_v0.x, sizeof(Vector4)); memory_copy(&valueF[4], &_v1.x, sizeof(Vector4)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<Vector4> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors4f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(ArrayStack<Vector4> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors4f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(Array<Colour> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors4f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(ArrayStack<Colour> const & _v, bool _forced = false) { type = ShaderParamType::uniformVectors4f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); }
		inline void set_uniform(VectorInt4 const & _v, bool _forced = false) { type = ShaderParamType::uniform4i; forced = _forced; memory_copy(valueI, &_v, sizeof(VectorInt4)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Matrix33 const & _v, bool _forced = false) { type = ShaderParamType::uniformMatrix3f; forced = _forced; memory_copy(valueF, &_v, sizeof(Matrix33)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<Matrix33> const & _v, bool _forced = false) { type = ShaderParamType::uniformMatrices3f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); } // should be avoided as may hit performance
		inline void set_uniform(ArrayStack<Matrix33> const & _v, bool _forced = false) { type = ShaderParamType::uniformMatrices3f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); } // should be avoided as may hit performance
		inline void set_uniform_ptr(Matrix33 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformMatricesPointer3f; forced = _forced; valueDptr = (int8*)_v; valueDsize = _count * sizeof(Matrix33); }
		inline void set_uniform(Matrix33 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformMatrices3f; forced = _forced; set_uniform_data(_count * sizeof(Matrix33), _v); }
		inline void set_uniform(Matrix44 const & _v, bool _forced = false) { type = ShaderParamType::uniformMatrix4f; forced = _forced; memory_copy(valueF, &_v, sizeof(Matrix44)); valueDsize = 0; /* no D */ }
		inline void set_uniform(Array<Matrix44> const & _v, bool _forced = false) { type = ShaderParamType::uniformMatrices4f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); } // should be avoided as may hit performance
		inline void set_uniform(ArrayStack<Matrix44> const & _v, bool _forced = false) { type = ShaderParamType::uniformMatrices4f; forced = _forced; set_uniform_data(_v.get_data_size(), _v.get_data()); } // should be avoided as may hit performance
		inline void set_uniform_ptr(Matrix44 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformMatricesPointer4f; forced = _forced; valueDptr = (int8*)_v; valueDsize = _count * sizeof(Matrix44); }
		inline void set_uniform(Matrix44 const * _v, int32 _count, bool _forced = false) { type = ShaderParamType::uniformMatrices4f; forced = _forced; set_uniform_data(_count * sizeof(Matrix44), _v); }
		inline void set_uniform(Matrix44 const & _v0, Matrix44 const & _v1, bool _forced = false) { type = ShaderParamType::uniformMatricesStereo4f; forced = _forced; memory_copy(valueF, &_v0.m00, sizeof(Matrix44)); memory_copy(&valueF[16], &_v1.m00, sizeof(Matrix44)); valueDsize = 0; /* no D */ }
		
		inline void set_uniform_data(int32 _dSize, void const * _src);
		inline void const * get_uniform_data() const;

		inline void store(ShaderParam const & _source, bool _forced = false);

		inline bool operator==(ShaderParam const & _other) const;
		inline bool operator!=(ShaderParam const & _other) const { return !operator==(_other); }

		inline float get_as_float() const { an_assert(type == ShaderParamType::uniform1f); return *(float*)get_uniform_data(); }
		inline Vector2 get_as_vector2() const { an_assert(type == ShaderParamType::uniform2f); return *(Vector2*)get_uniform_data(); }
		inline Vector3 get_as_vector3() const { an_assert(type == ShaderParamType::uniform3f); return *(Vector3*)get_uniform_data(); }
		inline Vector4 get_as_vector4() const { an_assert(type == ShaderParamType::uniform4f); return *(Vector4*)get_uniform_data(); }
		inline Colour get_as_colour() const { an_assert(type == ShaderParamType::uniform4f || type == ShaderParamType::uniform3f); if (type == ShaderParamType::uniform4f) { return *(Colour*)get_uniform_data(); } else { Colour c = *(Colour*)get_uniform_data(); c.a = 1.0f; return c; } }
		inline void get_as_array_float(OUT_ Array<float>& _into, bool _readSize = true) const { an_assert(type == ShaderParamType::uniform1fArray); if (_readSize) { _into.set_size(valueDsize / sizeof(float)); } memory_copy(_into.begin(), get_uniform_data(), min(valueDsize, _into.get_data_size())); }
		inline void get_as_array_vector4(OUT_ Array<Vector4>& _into, bool _readSize = true) const { an_assert(type == ShaderParamType::uniformVectors4f); if (_readSize) { _into.set_size(valueDsize / sizeof(Vector4)); } memory_copy(_into.begin(), get_uniform_data(), min(valueDsize, _into.get_data_size())); }
		// add more as needed

		String value_to_string() const;
	};

	struct NamedShaderParam
	{
		Name name;
		ShaderParam param;
	};

	struct ShaderProgramInstance
	{
		static bool do_match(ShaderProgramInstance const & _a, ShaderProgramInstance const & _b);

		ShaderProgramInstance();
		~ShaderProgramInstance();

		void bind(ShaderProgramBindingContext const * _bindingContext, SetupShaderProgram _setup_shader_program) const;
		void unbind() const;

		void set_shader_program(ShaderProgram * _shaderProgram);
		ShaderProgram * get_shader_program() const { return shaderProgram; }

		ShaderParam const * get_shader_param(Name const & _name) const { return get_shader_param(find_uniform_index(_name)); }
		ShaderParam const * get_shader_param(int _idx) const { return uniforms.is_index_valid(_idx) ? &uniforms[_idx] : nullptr; }

		void set_uniforms_from(ShaderProgramInstance const & _source); // will set uniforms as in source
		void set_uniforms_from(SimpleVariableStorage const & _source);
		void hard_copy_from(ShaderProgramInstance const & _source); // will copy everything
		bool hard_copy_fill_missing_with(ShaderProgramInstance const & _source);

		void clear();

		// via name
		void clear_uniform(Name const & _name) const { clear_uniform(find_uniform_index(_name)); }
		void set_uniform(Name const & _name, ShaderParam const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, TextureID _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, int32 _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, float _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Array<float> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Array<Vector3> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Array<Vector4> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Array<Colour> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, ArrayStack<float> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, ArrayStack<Vector3> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, ArrayStack<Vector4> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, ArrayStack<Colour> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Vector2 const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Vector3 const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Vector4 const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform_ptr(Name const & _name, Vector4 const * _v, int32 _count, bool _force = false) const { set_uniform_ptr(find_uniform_index(_name), _v, _count, _force); }
		void set_uniform(Name const & _name, Vector4 const * _v, int32 _count, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _count, _force); }
		void set_uniform(Name const & _name, Matrix33 const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Matrix44 const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, Array<Matrix44> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform(Name const & _name, ArrayStack<Matrix44> const & _v, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _force); }
		void set_uniform_ptr(Name const & _name, Matrix44 const * _v, int32 _count, bool _force = false) const { set_uniform_ptr(find_uniform_index(_name), _v, _count, _force); }
		void set_uniform(Name const & _name, Matrix44 const * _v, int32 _count, bool _force = false) const { set_uniform(find_uniform_index(_name), _v, _count, _force); }

		// via index
		void clear_uniform(int _idx) const;
		void set_uniform(int _idx, ShaderParam const & _v, bool _force = false) const;
		void set_uniform(int _idx, TextureID _v, bool _force = false) const;
		void set_uniform(int _idx, int32 _v, bool _force = false) const;
		void set_uniform(int _idx, float _v, bool _force = false) const;
		void set_uniform(int _idx, Array<float> const & _v, bool _force = false) const;
		void set_uniform(int _idx, Array<Vector3> const & _v, bool _force = false) const;
		void set_uniform(int _idx, Array<Vector4> const & _v, bool _force = false) const;
		void set_uniform(int _idx, Array<Colour> const & _v, bool _force = false) const;
		void set_uniform(int _idx, ArrayStack<float> const & _v, bool _force = false) const;
		void set_uniform(int _idx, ArrayStack<Vector3> const & _v, bool _force = false) const;
		void set_uniform(int _idx, ArrayStack<Vector4> const & _v, bool _force = false) const;
		void set_uniform(int _idx, ArrayStack<Colour> const & _v, bool _force = false) const;
		void set_uniform(int _idx, Vector2 const & _v, bool _force = false) const;
		void set_uniform(int _idx, Vector3 const & _v, bool _force = false) const;
		void set_uniform(int _idx, Vector4 const & _v, bool _force = false) const;
		void set_uniform_ptr(int _idx, Vector4 const * _v, int32 _count, bool _force = false) const;
		void set_uniform(int _idx, Vector4 const * _v, int32 _count, bool _force = false) const;
		void set_uniform(int _idx, VectorInt4 const & _v, bool _force = false) const;
		void set_uniform(int _idx, Matrix33 const & _v, bool _force = false) const;
		void set_uniform(int _idx, Matrix44 const & _v, bool _force = false) const;
		void set_uniform(int _idx, Array<Matrix44> const & _v, bool _force = false) const;
		void set_uniform(int _idx, ArrayStack<Matrix44> const & _v, bool _force = false) const;
		void set_uniform_ptr(int _idx, Matrix44 const * _v, int32 _count, bool _force = false) const;
		void set_uniform(int _idx, Matrix44 const * _v, int32 _count, bool _force = false) const;

		void log(LogInfoContext & _log) const;

	private:
		ShaderProgram * shaderProgram; // doesn't do ref counting
		mutable ArrayStatic<ShaderParam, 100> uniforms; // indices same as in shaderProgram - mutable as this is just cache

		void operator=(ShaderProgramInstance const &); // DO NOT IMPLEMENT

		int find_uniform_index(Name const & _name) const;

		void set_shader_program_being_part_of_shader_program(ShaderProgram * _shaderProgram);

		friend class ShaderProgram;
	};

	#include "shaderProgramInstance.inl"
};

 