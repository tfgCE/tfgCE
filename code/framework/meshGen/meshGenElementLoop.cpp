#include "meshGenElementLoop.h"

#include "meshGenerator.h"
#include "meshGenGenerationContext.h"

#include "meshGenUtils.h"

#include "..\..\core\io\xml.h"

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementLoop);

ElementLoop::~ElementLoop()
{
}

bool ElementLoop::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	result &= startWMP.load_from_xml(_node->first_child_named(TXT("startWMP")));
	result &= whileWMP.load_from_xml(_node->first_child_named(TXT("whileWMP")));
	result &= nextWMP.load_from_xml(_node->first_child_named(TXT("nextWMP")));

	bool anyElementsPresent = false;
	for_every(elementsNode, _node->children_named(TXT("loopElements")))
	{
		for_every(node, elementsNode->all_children())
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					loopElements.push_back(RefCountObjectPtr<Element>(element));
				}
				else
				{
					error_loading_xml(node, TXT("problem loading one element of set"));
					result = false;
				}
			}
		}
		anyElementsPresent = true;
	}

	if (!anyElementsPresent)
	{
		error_loading_xml(_node, TXT("no elements provided for loop"));
		result = false;
	}

	if (whileWMP.is_empty() && nextWMP.is_empty())
	{
		error_loading_xml(_node, TXT("both whileWMP and nextWMP not provided"));
		result = false;
	}

	return result;
}

bool ElementLoop::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(element, loopElements)
	{
		result &= element->get()->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementLoop::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;

	WheresMyPoint::Value tempValue;
	WheresMyPoint::Context context(&_instance);
	context.set_random_generator(_instance.access_context().access_random_generator());
	int advRG = context.access_random_generator().get_int();
	if (!startWMP.update(tempValue, context))
	{
		error_generating_mesh(_instance, TXT("error processing startWMP"));
		return false;
	}
	Checkpoint checkpoint(_context);
	while (true)
	{
		if (!whileWMP.is_empty())
		{
			WheresMyPoint::Value tempValue;
			tempValue.set(true);
			if (!whileWMP.update(tempValue, context))
			{
				error_generating_mesh(_instance, TXT("error processing whileWMP"));
				return false;
			}
			if (tempValue.get_type() != type_id<bool>())
			{
				error_generating_mesh(_instance, TXT("whileWMP, expected bool result"));
				return false;
			}
			if (!tempValue.get_as<bool>())
			{
				break;
			}
		}
		{
			if (useCheckpoint)
			{
				_context.push_checkpoint(checkpoint);
			}
			for_every(element, loopElements)
			{
				_context.access_random_generator().advance_seed(2374, 823794);
				_context.access_random_generator().advance_seed(advRG, advRG+5);

				result &= _context.process(element->get(), &_instance, for_everys_index(element));
			}
			if (useCheckpoint)
			{
				_context.pop_checkpoint();
			}
			checkpoint = Checkpoint(_context);
		}
		if (!nextWMP.is_empty())
		{
			WheresMyPoint::Value tempValue;
			tempValue.set(true);
			if (!nextWMP.update(tempValue, context))
			{
				error_generating_mesh(_instance, TXT("error processing nextWMP"));
				return false;
			}
			if (tempValue.get_type() != type_id<bool>())
			{
				error_generating_mesh(_instance, TXT("nextWMP, expected bool result"));
				return false;
			}
			if (!tempValue.get_as<bool>())
			{
				break;
			}
		}
	}

	return result;
}

#ifdef AN_DEVELOPMENT
void ElementLoop::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(element, loopElements)
	{
		element->get()->for_included_mesh_generators(_do);
	}
}
#endif
