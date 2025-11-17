#include "wmp_mirror.h"

#include "..\io\xml.h"

using namespace WheresMyPoint;

bool Mirror::load_from_xml(IO::XML::Node const * _node)
{
	bool result = base::load_from_xml(_node);

	mirrorDir.load_from_xml(_node);
	mirrorDir.normalise();

	result &= mirrorDirToolSet.load_from_xml(_node);

	return result;
}

bool Mirror::update(REF_ Value & _value, Context & _context) const
{
	if (_value.get_type() != type_id<Vector3>())
	{
		error_processing_wmp(TXT("can't mirror something else than vector3"));
		return false;
	}

	bool result = true;

	Vector3 useMirrorDir = mirrorDir;

	Value mirrorDirValue;
	mirrorDirValue.set(useMirrorDir);

	if (mirrorDirToolSet.update(mirrorDirValue, _context))
	{
		if (mirrorDirValue.get_type() == type_id<Vector3>())
		{
			useMirrorDir = mirrorDirValue.get_as<Vector3>().normal();
		}
		else
		{
			error_processing_wmp(TXT("\"mirror\" expected mirror dir being Vector3"));
			result = false;
		}
	}
	else
	{
		error_processing_wmp(TXT("error processing mirror dir"));
		result = false;
	}

	Vector3 value = _value.get_as<Vector3>();
	value = value - 2.0f * useMirrorDir * value;
	_value.set<Vector3>(value);

	return result;
}
