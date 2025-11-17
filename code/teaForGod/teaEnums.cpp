#include "teaEnums.h"

template <>
bool Optional<TeaForGodEmperor::DamageType::Type>::load_from_xml(IO::XML::Node const* _node)
{
	return load_optional_enum_from_xml<TeaForGodEmperor::DamageType::Type>(*this, _node, TeaForGodEmperor::DamageType::parse, TeaForGodEmperor::DamageType::Unknown);
}

template <>
bool Optional<TeaForGodEmperor::DamageType::Type>::load_from_xml(IO::XML::Attribute const* _attr)
{
	return load_optional_enum_from_xml<TeaForGodEmperor::DamageType::Type>(*this, _attr, TeaForGodEmperor::DamageType::parse, TeaForGodEmperor::DamageType::Unknown);
}
