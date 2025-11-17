#include "meshGenMeshNodeContext.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(MeshNodeContext);

MeshNodeContext::MeshNodeContext(GenerationContext & _generationContext, ElementInstance & _elementInstance, MeshNode * _meshNode)
: generationContext(_generationContext)
, elementInstance(_elementInstance)
, meshNode(_meshNode)
{

}

MeshNodeContext::~MeshNodeContext()
{
}

bool MeshNodeContext::get_point_for_wheres_my_point(Name const & _byName, OUT_ Vector3 & _point) const
{
	return elementInstance.get_point_for_wheres_my_point(_byName, _point);
}

bool MeshNodeContext::store_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return elementInstance.access_context().store_value_for_wheres_my_point(_byName, _value);
}

bool MeshNodeContext::restore_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return elementInstance.access_context().restore_value_for_wheres_my_point(_byName, OUT_ _value);
}

bool MeshNodeContext::store_convert_value_for_wheres_my_point(Name const& _byName, TypeId _to)
{
	return elementInstance.access_context().store_convert_value_for_wheres_my_point(_byName, _to);
}

bool MeshNodeContext::rename_value_forwheres_my_point(Name const& _from, Name const& _to)
{
	return elementInstance.access_context().rename_value_forwheres_my_point(_from, _to);
}

bool MeshNodeContext::store_global_value_for_wheres_my_point(Name const & _byName, WheresMyPoint::Value const & _value)
{
	return elementInstance.access_context().store_global_value_for_wheres_my_point(_byName, _value);
}

bool MeshNodeContext::restore_global_value_for_wheres_my_point(Name const & _byName, OUT_ WheresMyPoint::Value & _value) const
{
	return elementInstance.access_context().restore_global_value_for_wheres_my_point(_byName, OUT_ _value);
}

