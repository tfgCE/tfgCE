
bool ShaderParam::operator==(ShaderParam const & _other) const
{
	if (type != _other.type)
	{
		if (((type == ShaderParamType::uniformMatrices3f || type == ShaderParamType::uniformMatricesPointer3f) &&
			 (_other.type == ShaderParamType::uniformMatrices3f || _other.type == ShaderParamType::uniformMatricesPointer3f)) ||
			((type == ShaderParamType::uniformMatrices4f || type == ShaderParamType::uniformMatricesPointer4f) &&
			 (_other.type == ShaderParamType::uniformMatrices4f || _other.type == ShaderParamType::uniformMatricesPointer4f)) ||
			((type == ShaderParamType::uniformVectors4f || type == ShaderParamType::uniformVectorsPointer4f) &&
			 (_other.type == ShaderParamType::uniformVectors4f || _other.type == ShaderParamType::uniformVectorsPointer4f)))
		{
			// ok
		}
		else
		{
			return false;
		}
	}
	if (type == ShaderParamType::notSet)
	{
		return true;
	}
	if (type == ShaderParamType::texture)
	{
		return textureID == _other.textureID;
	}
	else if (type == ShaderParamType::uniform1i)
	{
		return valueI[0] == _other.valueI[0];
	}
	else if (type == ShaderParamType::uniform1iArray ||
			 type == ShaderParamType::uniform1fArray ||
			 type == ShaderParamType::uniformVectors2f ||
			 type == ShaderParamType::uniformVectors3f)
	{
		return valueDsize == _other.valueDsize &&
			memory_compare(get_uniform_data(), _other.get_uniform_data(), valueDsize);
	}
	else if (type == ShaderParamType::uniformVectors4f ||
			 type == ShaderParamType::uniformVectorsPointer4f)
	{
		void const *pStart = type == ShaderParamType::uniformVectors4f ? _other.get_uniform_data() : _other.valueDptr;
		int32 pDataSize = _other.valueDsize;
		//
		int32 uvDataSize = valueDsize;
		if (uvDataSize != pDataSize)
		{
			return false;
		}
		void const *uv = type == ShaderParamType::uniformVectors4f ? get_uniform_data() : valueDptr;
		return memory_compare(uv, pStart, pDataSize);
	}
	else if (type == ShaderParamType::uniform1f)
	{
		return valueF[0] == _other.valueF[0];
	}
	else if (type == ShaderParamType::uniform2f)
	{
		return valueF[0] == _other.valueF[0] &&
			   valueF[1] == _other.valueF[1];
	}
	else if (type == ShaderParamType::uniform3f)
	{
		return valueF[0] == _other.valueF[0] &&
			   valueF[1] == _other.valueF[1] &&
			   valueF[2] == _other.valueF[2];
	}
	else if (type == ShaderParamType::uniform4f)
	{
		return valueF[0] == _other.valueF[0] &&
			   valueF[1] == _other.valueF[1] &&
			   valueF[2] == _other.valueF[2] &&
			   valueF[3] == _other.valueF[3];
	}
	else if (type == ShaderParamType::uniform4i)
	{
		return valueI[0] == _other.valueI[0] &&
			   valueI[1] == _other.valueI[1] &&
			   valueI[2] == _other.valueI[2] &&
			   valueI[3] == _other.valueI[3];
	}
	else if (type == ShaderParamType::uniformMatrix3f)
	{
		return memory_compare(valueF, _other.valueF, sizeof(Matrix33));
	}
	else if (type == ShaderParamType::uniformMatrices3f ||
			 type == ShaderParamType::uniformMatricesPointer3f)
	{
		void const *pStart = type == ShaderParamType::uniformMatrices3f ? _other.get_uniform_data() : _other.valueDptr;
		int32 pDataSize = _other.valueDsize;
		//
		int32 uvDataSize = valueDsize;
		if (uvDataSize != pDataSize)
		{
			return false;
		}
		void const *uv = type == ShaderParamType::uniformMatrices3f ? get_uniform_data() : valueDptr;
		return memory_compare(uv, pStart, pDataSize);
	}
	else if (type == ShaderParamType::uniformMatrix4f)
	{
		return memory_compare(valueF, _other.valueF, sizeof(Matrix44));
	}
	else if (type == ShaderParamType::uniformMatrices4f ||
			 type == ShaderParamType::uniformMatricesPointer4f)
	{
		void const *pStart = _other.type == ShaderParamType::uniformMatrices4f ? _other.get_uniform_data() : _other.valueDptr;
		int32 pDataSize = _other.valueDsize;
		//
		int32 uvDataSize = valueDsize;
		if (uvDataSize != pDataSize)
		{
			return false;
		}
		void const *uv = type == ShaderParamType::uniformMatrices4f ? get_uniform_data() : valueDptr;
		return memory_compare(uv, pStart, pDataSize);
	}
	else
	{
		an_assert(type == ShaderParamType::notSet, TXT("not implemented?"));
	}
	return false;
}

void ShaderParam::store(ShaderParam const & _source, bool _forced)
{
	type = _source.type;
	forced = _forced || _source.forced;
	if (type == ShaderParamType::texture)
	{
		memory_copy(&textureID, &_source.textureID, sizeof(TextureID));
	}
	else if (type == ShaderParamType::uniform1i)
	{
		memory_copy(&valueI, &_source.valueI, sizeof(int32));
	}
	else if (type == ShaderParamType::uniform1f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(float));
	}
	else if (type == ShaderParamType::uniform2f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Vector2));
	}
	else if (type == ShaderParamType::uniform3f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Vector3));
	}
	else if (type == ShaderParamType::uniform4f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Vector4));
	}
	else if (type == ShaderParamType::uniform4i)
	{
		memory_copy(&valueI, &_source.valueI, sizeof(Vector4));
	}
	else if (type == ShaderParamType::uniformMatrix3f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Matrix33));
	}
	else if (type == ShaderParamType::uniformMatrix4f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Matrix44));
	}
	else if (type == ShaderParamType::uniformVectorsStereo3f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Vector3) * 2);
	}
	else if (type == ShaderParamType::uniformVectorsStereo4f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Vector4) * 2);
	}
	else if (type == ShaderParamType::uniformMatricesStereo4f)
	{
		memory_copy(&valueF, &_source.valueF, sizeof(Matrix44) * 2);
	}
	else if (type == ShaderParamType::uniform1iArray ||
			 type == ShaderParamType::uniform1fArray ||
			 type == ShaderParamType::uniformVectors2f ||
			 type == ShaderParamType::uniformVectors3f ||
			 type == ShaderParamType::uniformVectors4f ||
			 type == ShaderParamType::uniformMatrices3f ||
			 type == ShaderParamType::uniformMatrices4f)
	{
		set_uniform_data(_source.valueDsize, _source.get_uniform_data());
	}
	else if (type == ShaderParamType::uniformVectorsPointer4f)
	{
		type = ShaderParamType::uniformVectors4f;
		set_uniform_data(_source.valueDsize, _source.valueDptr);
	}
	else if (type == ShaderParamType::uniformMatricesPointer3f)
	{
		type = ShaderParamType::uniformMatrices3f;
		set_uniform_data(_source.valueDsize, _source.valueDptr);
	}
	else if (type == ShaderParamType::uniformMatricesPointer4f)
	{
		type = ShaderParamType::uniformMatrices4f;
		set_uniform_data(_source.valueDsize, _source.valueDptr);
	}
	else
	{
		an_assert(type == ShaderParamType::notSet, TXT("not implemented?"));
	}
}

void ShaderParam::set_uniform_data(int32 _dSize, void const * _src)
{
	valueDsize = _dSize;
	if (_dSize <= sizeof(float) * 32)
	{
		valueD.set_size(0);
		memory_copy(valueF, _src, _dSize);
	}
	else
	{
		valueD.set_size(_dSize);
		memory_copy(valueD.get_data(), _src, _dSize);
	}
}

void const * ShaderParam::get_uniform_data() const
{
	return valueDsize <= sizeof(float) * 32 ? plain_cast<void>(valueF) : plain_cast<void>(valueD.get_data());
}
