#include "meshGenElementRandomSet.h"

#include "meshGenerator.h"
#include "meshGenGenerationContext.h"
#include "meshGenValueDefImpl.inl"
#include "meshGenUtils.h"

#include "..\library\library.h"

#include "..\..\core\io\xml.h"
#include "..\..\core\debug\debugVisualiser.h"

//

using namespace Framework;
using namespace MeshGeneration;

//

#ifdef AN_DEVELOPMENT
#define AN_DEBUG_ITERATIONS
#endif

//

DEFINE_STATIC_NAME(randomSetValue);

//

REGISTER_FOR_FAST_CAST(ElementRandomSet);

ElementRandomSet::~ElementRandomSet()
{
}

bool ElementRandomSet::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

#ifdef AN_DEVELOPMENT
	debugIterations = _node->get_bool_attribute_or_from_child_presence(TXT("debugIterations"), debugIterations);
#endif

	range.load_from_xml(_node, TXT("range"));
	count.load_from_xml(_node, TXT("count"));
	iterationCount.load_from_xml(_node, TXT("iterationCount"));
	pushAwayStrength.load_from_xml(_node, TXT("pushAwayStrength"));
	pushAwayDistance.load_from_xml(_node, TXT("pushAwayDistance"));
	keepTogetherStrength.load_from_xml(_node, TXT("keepTogetherStrength"));
	pullCloserStrength.load_from_xml(_node, TXT("pullCloserStrength"));
	pullCloserDistance.load_from_xml(_node, TXT("pullCloserDistance"));

	randomSetValueParam = _node->get_name_attribute_or_from_child(TXT("randomSetValueParam"), randomSetValueParam);
	if (!randomSetValueParam.is_valid())
	{
		randomSetValueParam = NAME(randomSetValue);
	}
	randomSetIndexParam = _node->get_name_attribute_or_from_child(TXT("randomSetIndexParam"), randomSetIndexParam);

	bool anyElementsPresent = false;
	for_every(elementsNode, _node->children_named(TXT("elements")))
	{
		for_every(node, elementsNode->all_children())
		{
			if (Element* element = Element::create_from_xml(node, _lc))
			{
				if (element->load_from_xml(node, _lc))
				{
					elements.push_back(RefCountObjectPtr<Element>(element));
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
		error_loading_xml(_node, TXT("no elements provided for random set"));
		result = false;
	}

	return result;
}

bool ElementRandomSet::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	for_every(element, elements)
	{
		result &= element->get()->prepare_for_game(_library, _pfgContext);
	}

	return result;
}

bool ElementRandomSet::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;

	WheresMyPoint::Value tempValue;
	WheresMyPoint::Context context(&_instance);
	context.set_random_generator(_instance.access_context().access_random_generator());

	int useCount = count.get(_context);
	int useIterationCount = iterationCount.get_with_default(_context, 10);
	Range2 useRange = range.get(_context);
	float useKeepTogetherStrength = keepTogetherStrength.get_with_default(_context, 0.001f);
	float usePullCloserStrength = pullCloserStrength.get_with_default(_context, 0.001f);
	float usePullCloserDistance = pullCloserDistance.get_with_default(_context, 0.2f);
	float usePushAwayStrength = pushAwayStrength.get_with_default(_context, 0.005f);
	float usePushAwayDistance = pushAwayDistance.get_with_default(_context, 0.1f);

	if (useCount > 0)
	{
#ifdef AN_DEBUG_ITERATIONS
		DebugVisualiserPtr dv;
		if (debugIterations)
		{
			dv = new DebugVisualiser(String::printf(TXT("random set")));
		}
		if (dv.is_set())
		{
			dv->activate();
		}
#endif

		ARRAY_PREALLOC_SIZE(Vector2, randomSet, useCount);
		ARRAY_PREALLOC_SIZE(Vector2, randomSetMoveBy, useCount);
		
		for_count(int, i, useCount)
		{
			randomSet.push_back(Vector2(context.access_random_generator().get(useRange.x), context.access_random_generator().get(useRange.y)));
			randomSetMoveBy.push_back(Vector2::zero);
		}

		for_count(int, it, useIterationCount)
		{
#ifdef AN_DEBUG_ITERATIONS
			if (dv.is_set())
			{
				dv->start_gathering_data();
				dv->clear();
				dv->add_text(useRange.centre(), String::printf(TXT("#%i"), it), Colour::blue);
				for_every(v, randomSet)
				{
					dv->add_circle(*v, 0.001f, Colour::blue);
					dv->add_circle(*v, usePullCloserDistance, Colour::lerp(0.8f, Colour::blue, Colour::white));
					dv->add_circle(*v, usePushAwayDistance, Colour::lerp(0.8f, Colour::red, Colour::white));
				}
				dv->add_range2(useRange, Colour::purple);
			}
#endif

			// calculate how much do we have to move
			for_every(mb, randomSetMoveBy)
			{
				*mb = Vector2::zero;
				int idx = for_everys_index(mb);
				Vector2 at = randomSet[idx];
				Vector2 centre = Vector2::zero;
				for_every(v, randomSet)
				{
					centre += *v;
					if (idx == for_everys_index(v))
					{
						continue;
					}
					Vector2 toV = *v - at;
					Vector2 toVDir = toV.normal();
					float dist = toV.length();
					if (dist < usePushAwayDistance)
					{
						if (toVDir.is_almost_zero())
						{
							toVDir.x = _context.access_random_generator().get_float(-1.0f, 1.0f);
							toVDir.y = _context.access_random_generator().get_float(-1.0f, 1.0f);
							toVDir.normalise();
						}
						*mb -= toVDir * usePushAwayStrength * clamp(3.0f * (1.0f - dist / usePushAwayDistance), 0.0f, 1.0f);
					}
					if (dist > usePullCloserDistance)
					{
						*mb += toVDir * usePullCloserStrength * clamp(((dist - usePullCloserDistance) / usePullCloserDistance - 1.0f), 0.0f, 1.0f);
					}
				}
				centre /= (float)randomSet.get_size();
				if (useKeepTogetherStrength != 0.0f)
				{
					Vector2 toV = centre - at;
					Vector2 toVDir = toV.normal();
					*mb += toVDir * useKeepTogetherStrength;
				}
			}

#ifdef AN_DEBUG_ITERATIONS
			if (dv.is_set())
			{
				for_every(v, randomSet)
				{
					int idx = for_everys_index(v);
					Vector2 mb = randomSetMoveBy[idx];
					dv->add_circle(*v, 0.001f, Colour::blue);
					dv->add_arrow(*v, *v + mb, Colour::green);
				}
				dv->end_gathering_data();
				dv->show_and_wait_for_key();
			}
#endif

			// move
			for_every(v, randomSet)
			{
				int idx = for_everys_index(v);
				Vector2 mb = randomSetMoveBy[idx];
				*v += mb;
				v->x = useRange.x.clamp(v->x);
				v->y = useRange.y.clamp(v->y);
			}
		}

#ifdef AN_DEBUG_ITERATIONS
		if (dv.is_set())
		{
			dv->deactivate();
		}
#endif

		for_every(v, randomSet)
		{
			_context.access_random_generator().advance_seed(268547, 92579);
			_context.set_parameter<Vector2>(randomSetValueParam, *v);
			if (randomSetIndexParam.is_valid())
			{
				_context.set_parameter<int>(randomSetIndexParam, for_everys_index(v));
			}

			for_every(element, elements)
			{
				_context.access_random_generator().advance_seed(8234, 85735);

				result &= _context.process(element->get(), &_instance, for_everys_index(element));
			}
		}
	}
	else
	{
		error_generating_mesh_element(this, TXT("no elements to create"));
	}

	return result;
}

#ifdef AN_DEVELOPMENT
void ElementRandomSet::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	for_every(element, elements)
	{
		element->get()->for_included_mesh_generators(_do);
	}
}
#endif
