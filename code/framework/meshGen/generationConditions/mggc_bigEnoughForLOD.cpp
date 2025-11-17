#include "mggc_bigEnoughForLOD.h"

#include "..\meshGenConfig.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\debug\previewGame.h"

//

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool BigEnoughForLOD::check(ElementInstance & _instance) const
{
	Optional<float> size;
	if (float const* value = _instance.access_context().get_parameter<float>(parameter))
	{
		size = *value;
	}
	if (Vector2 const* value = _instance.access_context().get_parameter<Vector2>(parameter))
	{
		size = value->length();
	}
	if (Vector3 const* value = _instance.access_context().get_parameter<Vector3>(parameter))
	{
		size = value->length();
	}
	{
		Vector3 const* valueA = _instance.access_context().get_parameter<Vector3>(pointA);
		Vector3 const* valueB = _instance.access_context().get_parameter<Vector3>(pointB);
		if (valueA && valueB)
		{
			size = (*valueA - *valueB).length();
		}
	}
	if (! size.is_set())
	{
		error_generating_mesh(_instance, TXT("don't know how to handle parameters \"%S\" or \"%S\"+\"%S\""), parameter.to_char(), pointA.to_char(), pointB.to_char());
	}

	int lod = _instance.get_context().get_for_lod();
	if (auto* g = Game::get())
	{
		lod = max(lod, g->get_generate_at_lod());
	}

	return !size.is_set() || size.get() >= MeshGeneration::Config::get().get_min_size(lod, _instance.get_context().get_use_lod_fully());
}

bool BigEnoughForLOD::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	parameter = _node->get_name_attribute(TXT("var"), parameter);
	pointA = _node->get_name_attribute(TXT("pointAvar"), pointA);
	pointB = _node->get_name_attribute(TXT("pointBvar"), pointB);
	if (!parameter.is_valid() &&
		(!pointA.is_valid() || !pointB.is_valid()))
	{
		error_loading_xml(_node, TXT("generation condition bigEnoughForLOD has no \"var\" nor \"pointA\"+\"pointB\" provided"));
		result = false;
	}

	return result;
}
