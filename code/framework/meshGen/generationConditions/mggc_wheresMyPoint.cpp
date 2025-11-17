#include "mggc_wheresMyPoint.h"

#include "..\meshGenGenerationContext.h"

#include "..\..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;
using namespace GenerationConditions;

//

bool GenerationConditions::WheresMyPoint::check(ElementInstance & _instance) const
{
	bool result = true;

	::WheresMyPoint::Context context(&_instance);
	context.set_random_generator(_instance.access_context().access_random_generator());
	value.update(result, context);

	return result;
}

bool GenerationConditions::WheresMyPoint::load_from_xml(IO::XML::Node const * _node)
{
	bool result = true;

	result &= value.load_from_xml(_node);

	return result;
}

