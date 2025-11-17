#include "aiParam.h"

#include "..\..\core\other\simpleVariableStorage.h"

//

using namespace Framework;
using namespace AI;

//

Param::Param()
{
}

Param::Param(Name const& _name)
{
	name = _name;
}

Param::Param(Param const & _other)
{
	name = _other.name;
	type = _other.type;
	if (type != type_id_none())
	{
		RegisteredType::construct(type, data);
		RegisteredType::copy(type, data, _other.data);
	}
}

Param& Param::operator = (Param const & _other)
{
	if (type != _other.type)
	{
		if (type != type_id_none())
		{
			RegisteredType::destruct(type, data);
		}
		if (_other.type != type_id_none())
		{
			RegisteredType::construct(_other.type, data);
		}
	}
	name = _other.name;
	type = _other.type;
	if (type != type_id_none())
	{
		RegisteredType::copy(type, data, _other.data);
	}
	return *this;
}

Param::~Param()
{
	if (type != type_id_none())
	{
		RegisteredType::destruct(type, data);
		//type = type_id_none();
	}
}

void Param::set(SimpleVariableInfo const& _svi)
{
	name = _svi.get_name();
	set(_svi.type_id(), _svi.get_raw());
}

void Param::set(TypeId _type, void const* _raw)
{
	if (_type != type_id_none() && RegisteredType::get_size_of(_type) > DATASIZE)
	{
		error(TXT("no enough space for ai param"));
		return;
	}
	if (type != _type)
	{
		if (type != type_id_none())
		{
			RegisteredType::destruct(type, data);
		}
		if (_type != type_id_none())
		{
			RegisteredType::construct(_type, data);
		}
	}
	type = _type;
	if (type != type_id_none())
	{
		RegisteredType::copy(type, data, _raw);
	}
}

void Param::clear()
{
	if (type != type_id_none())
	{
		RegisteredType::destruct(type, data);
	}
	name = Name::invalid();
	type = type_id_none();
}

//

Params::Params()
{
	SET_EXTRA_DEBUG_INFO(params, TXT("AI::Params.params"));
}

Param & Params::access(Name const & _name)
{
	for_every(param, params)
	{
		if (param->get_name() == _name)
		{
			return *param;
		}
	}
	params.push_back(Param(_name));
	return params.get_last();
}

Param const * Params::get(Name const & _name) const
{
	for_every(param, params)
	{
		if (param->get_name() == _name)
		{
			return param;
		}
	}

	return nullptr;
}

void Params::clear()
{
	for_every(param, params)
	{
		param->clear();
	}
	params.clear();
}
