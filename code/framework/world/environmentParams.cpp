#include "environmentParams.h"

#include "..\library\libraryPrepareContext.h"
#include "..\video\texture.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\math\math.h"
#include "..\..\core\casts.h"
#include "..\..\core\types\colour.h"
#include "..\..\core\system\core.h"
#include "..\..\core\system\video\shaderProgramBindingContext.h"
#include "..\..\core\utils\utils_loadingForSystemShader.h"

#ifdef AN_CLANG
#include "..\library\usedLibraryStored.inl"
#endif

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;

//

EnvironmentParam::EnvironmentParam()
: type(EnvironmentParamType::notSet)
{
}

EnvironmentParam::EnvironmentParam(EnvironmentParamType::Type _type)
: type(_type)
{
}

void EnvironmentParam::setup_shader_binding_context(::System::ShaderProgramBindingContext * _bindingContext, Matrix44 const & _modelViewMatrixReadiedForRendering) const
{
	::System::ShaderParam & shaderParam = _bindingContext->access_shader_param(name).param;
	if (type == EnvironmentParamType::floatNumber ||
		type == EnvironmentParamType::floatNumberInverted)
	{
		shaderParam.set_uniform(valueF[0]);
	}
	else if (type == EnvironmentParamType::cycle)
	{
		shaderParam.set_uniform(valueF[0]);
	}
	else if (type == EnvironmentParamType::vector2)
	{
		shaderParam.set_uniform(*(plain_cast<Vector2>(valueF)));
	}
	else if (type == EnvironmentParamType::colour)
	{
		shaderParam.set_uniform(*(plain_cast<Vector4>(valueF)));
	}
	else if (type == EnvironmentParamType::colour3)
	{
		shaderParam.set_uniform(*(plain_cast<Vector3>(valueF)));
	}
	else if (type == EnvironmentParamType::segment01)
	{
		shaderParam.set_uniform(*(plain_cast<Vector2>(valueF)));
	}
	else if (type == EnvironmentParamType::direction)
	{
		shaderParam.set_uniform(_modelViewMatrixReadiedForRendering.vector_to_world(*(plain_cast<Vector3>(valueF))));
	}
	else if (type == EnvironmentParamType::location)
	{
		shaderParam.set_uniform(_modelViewMatrixReadiedForRendering.location_to_world(*(plain_cast<Vector3>(valueF))));
	}
	else if (type == EnvironmentParamType::matrix33)
	{
		shaderParam.set_uniform(_modelViewMatrixReadiedForRendering.to_matrix33().to_world(*(plain_cast<Matrix33>(valueF))));
	}
	else if (type == EnvironmentParamType::texture)
	{
		if (auto const * t = texture.get())
		{
			shaderParam.set_uniform(t->get_texture()->get_texture_id());
		}
		else
		{
			shaderParam.set_uniform(::System::texture_id_none());
		}
	}
	else
	{
		an_assert(false, TXT("not implemented"));
	}
}

void EnvironmentParam::lerp_with(float _amount, EnvironmentParam const & _param)
{
	float invAmount = 1.0f - _amount;
	if (type == EnvironmentParamType::floatNumber)
	{
		valueF[0] = invAmount * valueF[0] + _amount * _param.valueF[0];
	}
	else if (type == EnvironmentParamType::floatNumberInverted)
	{
		float f = valueF[0] != 0.0f ? 1.0f / valueF[0] : 0.0f;
		float p = _param.valueF[0] != 0.0f ? 1.0f / _param.valueF[0] : 0.0f;
		float rInv = invAmount * f + _amount * p;
		valueF[0] = rInv != 0.0f? 1.0f / rInv : 0.0f;
	}
	else if (type == EnvironmentParamType::cycle)
	{
		float tNorm = (valueF[0] - valueF[1]) / (valueF[2] - valueF[1]);
		float oNorm = (_param.valueF[0] - _param.valueF[1]) / (_param.valueF[2] - _param.valueF[1]);
		float add = mod(oNorm - tNorm + 0.5f, 1.0f) - 0.5f;
		float added = tNorm + _amount * add;
		valueF[0] = valueF[1] + mod(added, 1.0f) * (valueF[2] - valueF[1]);
	}
	else if (type == EnvironmentParamType::vector2)
	{
		*plain_cast<Vector2>(valueF) = invAmount * *plain_cast<Vector2>(valueF) + _amount * *plain_cast<Vector2>(_param.valueF);
	}
	else if (type == EnvironmentParamType::colour)
	{
		*plain_cast<Vector4>(valueF) = invAmount * *plain_cast<Vector4>(valueF) + _amount * *plain_cast<Vector4>(_param.valueF);
	}
	else if (type == EnvironmentParamType::colour3)
	{
		*plain_cast<Vector3>(valueF) = invAmount * *plain_cast<Vector3>(valueF) + _amount * *plain_cast<Vector3>(_param.valueF);
	}
	else if (type == EnvironmentParamType::segment01)
	{
		auto& seg = *plain_cast<Vector2>(valueF);
		auto& addSeg = *plain_cast<Vector2>(_param.valueF);
		seg.x = invAmount * seg.x + _amount * addSeg.x;
		seg.y = safe_inv(invAmount * safe_inv(seg.y) + _amount * safe_inv(addSeg.y));
	}
	else if (type == EnvironmentParamType::direction)
	{
		*plain_cast<Vector3>(valueF) = (invAmount * *plain_cast<Vector3>(valueF) + _amount * *plain_cast<Vector3>(_param.valueF)).normal();
	}
	else if (type == EnvironmentParamType::matrix33)
	{
		// do not lerp?
	}
	else if (type == EnvironmentParamType::location)
	{
		*plain_cast<Vector3>(valueF) = invAmount * *plain_cast<Vector3>(valueF) + _amount * *plain_cast<Vector3>(_param.valueF);
	}
}

void EnvironmentParam::add(float _amount, EnvironmentParam const & _param)
{
	if (type == EnvironmentParamType::floatNumber)
	{
		valueF[0] += _amount * _param.valueF[0];
	}
	else if (type == EnvironmentParamType::floatNumberInverted)
	{
		float f = valueF[0] != 0.0f ? 1.0f / valueF[0] : 0.0f;
		float p = _param.valueF[0] != 0.0f ? 1.0f / _param.valueF[0] : 0.0f;
		float rInv = f + _amount * p;
		valueF[0] = rInv != 0.0f? 1.0f / rInv : 0.0f;
	}
	else if (type == EnvironmentParamType::cycle)
	{
		float tNorm = (valueF[0] - valueF[1]) / (valueF[2] - valueF[1]);
		float oNorm = (_param.valueF[0] - _param.valueF[1]) / (_param.valueF[2] - _param.valueF[1]);
		float add = mod(oNorm - tNorm + 0.5f, 1.0f) - 0.5f;
		float added = tNorm + _amount * add;
		valueF[0] = valueF[1] + mod(added, 1.0f) * (valueF[2] - valueF[1]);
	}
	else if (type == EnvironmentParamType::vector2)
	{
		*plain_cast<Vector2>(valueF) += _amount * *plain_cast<Vector2>(_param.valueF);
	}
	else if (type == EnvironmentParamType::colour)
	{
		*plain_cast<Vector4>(valueF) += _amount * *plain_cast<Vector4>(_param.valueF);
	}
	else if (type == EnvironmentParamType::colour3)
	{
		*plain_cast<Vector3>(valueF) += _amount * *plain_cast<Vector3>(_param.valueF);
	}
	else if (type == EnvironmentParamType::segment01)
	{
		auto& seg = *plain_cast<Vector2>(valueF);
		auto& addSeg = *plain_cast<Vector2>(_param.valueF);
		seg.x += _amount * addSeg.x;
		seg.y = safe_inv(safe_inv(seg.y) + _amount * safe_inv(addSeg.y));
	}
	else if (type == EnvironmentParamType::direction)
	{
		*plain_cast<Vector3>(valueF) = (*plain_cast<Vector3>(valueF) + _amount * *plain_cast<Vector3>(_param.valueF)).normal();
	}
	else if (type == EnvironmentParamType::matrix33)
	{
		// do not lerp?
	}
	else if (type == EnvironmentParamType::location)
	{
		*plain_cast<Vector3>(valueF) += _amount * *plain_cast<Vector3>(_param.valueF);
	}
}

void EnvironmentParam::log(LogInfoContext & _log) const
{
	if (type == EnvironmentParamType::floatNumber)
	{
		_log.log(TXT("%S : f : %.3f"), name.to_char(), valueF[0]);
	}
	else if (type == EnvironmentParamType::floatNumberInverted)
	{
		_log.log(TXT("%S : if : %.3f"), name.to_char(), valueF[0] != 0.0f? 1.0f / valueF[0] : 0.0f);
	}
	else if (type == EnvironmentParamType::cycle)
	{
		_log.log(TXT("%S : cycle : %.3f (%.3f -> %.3f)"), name.to_char(), valueF[0], valueF[1], valueF[2]);
	}
	else if (type == EnvironmentParamType::vector2)
	{
		_log.log(TXT("%S : v2 : %.3f %.3f"), name.to_char(), valueF[0], valueF[1]);
	}
	else if (type == EnvironmentParamType::colour)
	{
		_log.log(TXT("%S : clr4 : r%.3f g%.3f b%.3f a%.3f"), name.to_char(), valueF[0], valueF[1], valueF[2], valueF[3]);
	}
	else if (type == EnvironmentParamType::colour3)
	{
		_log.log(TXT("%S : clr3 : r%.3f g%.3f b%.3f"), name.to_char(), valueF[0], valueF[1], valueF[2]);
	}
	else if (type == EnvironmentParamType::segment01)
	{
		_log.log(TXT("%S : seg01 : %.3f -> %.3f (len: %.3f)"), name.to_char(), (*plain_cast<Vector2>(valueF)).x, (*plain_cast<Vector2>(valueF)).x + safe_inv((*plain_cast<Vector2>(valueF)).y), safe_inv((*plain_cast<Vector2>(valueF)).y));
	}
	else if (type == EnvironmentParamType::direction)
	{
		_log.log(TXT("%S : dir : %.3f %.3f %.3f"), name.to_char(), valueF[0], valueF[1], valueF[2]);
	}
	else if (type == EnvironmentParamType::matrix33)
	{
		_log.log(TXT("%S : mat33 :"), name.to_char());
		LOG_INDENT(_log);
		_log.log(TXT("%7.3f %7.3f %7.3f"), valueF[0], valueF[1], valueF[2]);
		_log.log(TXT("%7.3f %7.3f %7.3f"), valueF[3], valueF[4], valueF[5]);
		_log.log(TXT("%7.3f %7.3f %7.3f"), valueF[6], valueF[7], valueF[8]);
	}
	else if (type == EnvironmentParamType::location)
	{
		_log.log(TXT("%S : loc : %.3f %.3f %.3f"), name.to_char(), valueF[0], valueF[1], valueF[2]);
	}
}

bool EnvironmentParam::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	name = _node->get_name_attribute(TXT("name"), name);
	if (!name.is_valid())
	{
		error_loading_xml(_node, TXT("no name given for environment param"));
		result = false;
	}

	if (_node->get_name() == TXT("float"))
	{
		type = EnvironmentParamType::floatNumber;
		valueF[0] = _node->get_float(0.0f);
	}
	else if (_node->get_name() == TXT("cycle"))
	{
		type = EnvironmentParamType::cycle;
		valueF[0] = _node->get_float(0.0f);
		Range range(0.0f, 1.0f);
		range.load_from_xml(_node, TXT("range"));
		valueF[1] = range.min;
		valueF[2] = range.max;
	}
	else if (_node->get_name() == TXT("invertedFloat"))
	{
		type = EnvironmentParamType::floatNumberInverted;
		valueF[0] = _node->get_float(0.0f);
		valueF[0] = valueF[0] != 0.0f ? 1.0f / valueF[0] : 0.0f;
	}
	else if (_node->get_name() == TXT("vector2"))
	{
		Vector2 & param = *(plain_cast<Vector2>(valueF));
		param = Vector2::zero;
		if (param.load_from_xml(_node))
		{
			type = EnvironmentParamType::vector2;
		}
		else
		{
			error_loading_xml(_node, TXT("environment param loading error - vector2"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("colour"))
	{
		Colour & param = *(plain_cast<Colour>(valueF));
		param = Colour::black;
		if (param.load_from_xml(_node))
		{
			type = EnvironmentParamType::colour;
		}
		else
		{
			error_loading_xml(_node, TXT("environment param loading error - colour"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("colour3"))
	{
		Colour & param = *(plain_cast<Colour>(valueF));
		param = Colour::black;
		if (param.load_from_xml(_node))
		{
			float modulate = _node->get_float_attribute_or_from_child(TXT("modulate"), 1.0);
			param = param.mul_rgb(modulate);
			type = EnvironmentParamType::colour3;
		}
		else
		{
			error_loading_xml(_node, TXT("environment param loading error - colour3"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("segment01"))
	{
		type = EnvironmentParamType::segment01;
		float start = _node->get_float_attribute(TXT("start"), 0.0f);
		if (auto * attr = _node->get_attribute(TXT("length")))
		{
			float length = attr->get_as_float();
			float lengthInv = length != 0.0f ? 1.0f / length : 0.0f;
			Vector2& param = *(plain_cast<Vector2>(valueF));
			param.x = start;
			param.y = lengthInv;
		}
		else if (auto * attr = _node->get_attribute(TXT("end")))
		{
			float length = attr->get_as_float() - start;
			float lengthInv = length != 0.0f ? 1.0f / length : 0.0f;
			Vector2& param = *(plain_cast<Vector2>(valueF));
			param.x = start;
			param.y = lengthInv;
		}
		else
		{
			error_loading_xml(_node, TXT("no length nor end for segment01"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("direction") ||
			 _node->get_name() == TXT("directionNormalised") ||
			 _node->get_name() == TXT("location"))
	{
		Vector3 & param = *(plain_cast<Vector3>(valueF));
		param = Vector3::zero;
		if (param.load_from_xml(_node))
		{
			if (_node->get_name() == TXT("direction") ||
				_node->get_name() == TXT("directionNormalised"))
			{
				type = EnvironmentParamType::direction;
				if (_node->get_name() == TXT("directionNormalised"))
				{
					param.normalise();
				}
			}
			else
			{
				type = EnvironmentParamType::location;
			}
		}
		else
		{
			error_loading_xml(_node, TXT("environment param loading error - vector"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("matrix33"))
	{
		Matrix33 & param = *(plain_cast<Matrix33>(valueF));
		param = Matrix33::identity;
		if (param.load_from_xml(_node))
		{
			type = EnvironmentParamType::matrix33;
		}
		else
		{
			error_loading_xml(_node, TXT("environment param loading error - matrix33"));
			result = false;
		}
	}
	else if (_node->get_name() == TXT("texture"))
	{
		if (texture.load_from_xml(_node, TXT("texture"), _lc))
		{
			type = EnvironmentParamType::texture;
		}
		else
		{
			error_loading_xml(_node, TXT("environment param loading error - texture"));
			result = false;
		}
	}
	else
	{
		error_loading_xml(_node, TXT("environment param type not recognised"));
		result = false;
	}

	return result;
}

bool EnvironmentParam::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	if (type == EnvironmentParamType::texture)
	{
		result &= texture.prepare_for_game_find(_library, _pfgContext, LibraryPrepareLevel::Resolve);
	}

	return result;
}

//

void EnvironmentParams::clear()
{
	params.clear();
}

void EnvironmentParams::fill_with(EnvironmentParams const & _source, Array<Name> const * _skipParams)
{
	for_every(sourceParam, _source.params)
	{
		if (sourceParam->type == EnvironmentParamType::notSet)
		{
			continue;
		}
		if (_skipParams)
		{
			bool skipThis = false;
			for_every(sp, *_skipParams)
			{
				if (sourceParam->name == *sp)
				{
					skipThis = true;
					break;
				}
			}
			if (skipThis)
			{
				continue;
			}
		}
		bool found = false;
		for_every(param, params)
		{
			if (sourceParam->name == param->name)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			params.push_back(*sourceParam);
		}
	}
}

void EnvironmentParams::set_from(EnvironmentParams const & _source, Array<Name> const * _skipParams)
{
	for_every(sourceParam, _source.params)
	{
		if (sourceParam->type == EnvironmentParamType::notSet)
		{
			continue;
		}
		if (_skipParams)
		{
			bool skipThis = false;
			for_every(sp, *_skipParams)
			{
				if (sourceParam->name == *sp)
				{
					skipThis = true;
					break;
				}
			}
			if (skipThis)
			{
				continue;
			}
		}
		bool found = false;
		for_every(param, params)
		{
			if (sourceParam->name == param->name)
			{
				*param = *sourceParam;
				found = true;
				break;
			}
		}
		if (!found)
		{
			params.push_back(*sourceParam);
		}
	}
}

void EnvironmentParams::set_missing_from(EnvironmentParams const & _source, Array<Name> const * _skipParams)
{
	for_every(sourceParam, _source.params)
	{
		if (sourceParam->type == EnvironmentParamType::notSet)
		{
			continue;
		}
		if (_skipParams)
		{
			bool skipThis = false;
			for_every(sp, *_skipParams)
			{
				if (sourceParam->name == *sp)
				{
					skipThis = true;
					break;
				}
			}
			if (skipThis)
			{
				continue;
			}
		}
		bool found = false;
		for_every(param, params)
		{
			if (sourceParam->name == param->name)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			params.push_back(*sourceParam);
		}
	}
}

void EnvironmentParams::copy_from(EnvironmentParams const & _source, Array<Name> const * _skipParams)
{
	clear();
	fill_with(_source, _skipParams);
}

void EnvironmentParams::lerp_with(float _amount, EnvironmentParams const & _source, Array<Name> const * _skipParams)
{
	for_every(sourceParam, _source.params)
	{		
		if (_skipParams)
		{
			bool skipThis = false;
			for_every(sp, *_skipParams)
			{
				if (sourceParam->name == *sp)
				{
					skipThis = true;
					break;
				}
			}
			if (skipThis)
			{
				continue;
			}
		}

		if (sourceParam->type == EnvironmentParamType::notSet ||
			sourceParam->type == EnvironmentParamType::texture)
		{
			continue;
		}

#ifdef AN_DEVELOPMENT
		bool parameterExists = false;
#endif
		for_every(param, params)
		{
			if (param->name == sourceParam->name &&
				param->type == sourceParam->type)
			{
				param->lerp_with(_amount, *sourceParam);
#ifdef AN_DEVELOPMENT
				parameterExists = true;
#endif
			}
		}
#ifdef AN_DEVELOPMENT
		if (!parameterExists)
		{
			error(TXT("parameter \"%S\" does not exist in the source environment, make sure it is there, use default value"));
		}
#endif
	}
}

void EnvironmentParams::setup_shader_binding_context(::System::ShaderProgramBindingContext * _bindingContext, Matrix44 const & _modelViewMatrixReadiedForRendering) const
{
	for_every(param, params)
	{
		param->setup_shader_binding_context(_bindingContext, _modelViewMatrixReadiedForRendering);
	}
}

bool EnvironmentParams::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(childNode, _node->all_children())
	{
		if (childNode->get_name() == TXT("for"))
		{
			if (CoreUtils::Loading::should_load_for_system_or_shader_option(childNode))
			{
				result &= load_from_xml(childNode, _lc);
			}
		}
		else
		{
			EnvironmentParam param;
			if (param.load_from_xml(childNode, _lc))
			{
				bool exists = false;
				for_every(p, params)
				{
					if (p->get_name() == param.get_name())
					{
						*p = param;
						exists = true;
					}
				}
				if (!exists)
				{
					params.push_back(param);
				}
			}
			else
			{
				error_loading_xml(_node, TXT("error loading environment param"));
				result = false;
			}
		}
	}

	return result;
}

bool EnvironmentParams::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	for_every(param, params)
	{
		result &= param->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

#ifdef AN_TWEAKS
void EnvironmentParams::tweak(EnvironmentParam const & _param)
{
	for_every(param, params)
	{
		if (param->name == _param.name)
		{
			*param = _param;
		}
	}
}
#endif

EnvironmentParam const * EnvironmentParams::get_param(Name const & _name) const
{
	for_every(param, params)
	{
		if (param->name == _name)
		{
			return param;
		}
	}
	return nullptr;
}

EnvironmentParam* EnvironmentParams::access_param(Name const & _name, Optional<EnvironmentParamType::Type> const& _addOfTypeIfNotThere)
{
	for_every(param, params)
	{
		if (param->name == _name)
		{
			return param;
		}
	}
	if (_addOfTypeIfNotThere.is_set())
	{
		EnvironmentParam ep(_addOfTypeIfNotThere.get());
		ep.name = _name;
		params.push_back(ep);
		return &params.get_last();
	}
	return nullptr;
}

void EnvironmentParams::log(LogInfoContext & _log) const
{
	_log.log(TXT("environment params"));
	LOG_INDENT(_log);
	for_every(param, params)
	{
		param->log(_log);
	}
}
