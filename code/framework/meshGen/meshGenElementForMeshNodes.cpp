#include "meshGenElementForMeshNodes.h"

#include "meshGenGenerationContext.h"
#include "meshGenMeshNodeContext.h"
#include "meshGenUtils.h"

#include "..\appearance\meshNode.h"

#include "..\..\core\containers\arrayStack.h"
#include "..\..\core\io\xml.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace Framework;
using namespace MeshGeneration;

//

REGISTER_FOR_FAST_CAST(ElementForMeshNodes);

ElementForMeshNodes::~ElementForMeshNodes()
{
}

bool ElementForMeshNodes::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);

	againstCheckpointsUp = _node->get_int_attribute_or_from_child(TXT("againstCheckpointsUp"), againstCheckpointsUp);

	for_every(node, _node->children_named(TXT("forEach")))
	{
		ForEachPtr fe(new ForEach());
		if (fe->load_from_xml(node, _lc))
		{
			forEach.push_back(fe);
		}
		else
		{
			result = false;
		}
	}
	
	if (_node->first_child_named(TXT("element")))
	{
		result &= MeshGeneration::Element::load_single_element_from_xml(_node, _lc, TXT("element"), element);
	}

	return result;
}

bool ElementForMeshNodes::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = base::prepare_for_game(_library, _pfgContext);

	return result;
}

#ifdef AN_DEVELOPMENT
void ElementForMeshNodes::for_included_mesh_generators(std::function<void(MeshGenerator*)> _do) const
{
	if (element.is_set())
	{
		element->for_included_mesh_generators(_do);
	}
}
#endif

struct ElementForMeshNodes_Step
{
	MeshNode* meshNode = nullptr;
	int meshNodeIdx = NONE;
	int idxToCheck = 0;
};

bool ElementForMeshNodes::process(GenerationContext & _context, ElementInstance & _instance) const
{
	bool result = true;
	for_every_ref(fe, forEach)
	{
		int const startWithMeshNodeIdx = againstCheckpointsUp >= 0? _context.get_checkpoint(1 + againstCheckpointsUp).meshNodesSoFarCount : 0;

		int forEachMeshNodesCount = fe->meshNodes.get_size();
		ARRAY_PREALLOC_SIZE(ElementForMeshNodes_Step, foundForEachMeshNodes, forEachMeshNodesCount);
		foundForEachMeshNodes.set_size(forEachMeshNodesCount);
		for_every(fmn, foundForEachMeshNodes)
		{
			fmn->idxToCheck = startWithMeshNodeIdx;
		}
		int atLevel = 0;
		auto const& contextMeshNodes = _context.get_mesh_nodes();
		Array<MeshNodePtr> contextMeshNodesRandomOrder;
		contextMeshNodesRandomOrder.make_space_for(contextMeshNodes.get_size());
		{
			bool requiresRandomOrder = false;
			for_every_ref(femn, fe->meshNodes)
			{
				if (femn->randomOrder)
				{
					requiresRandomOrder = true;
					break;
				}
			}
			if (requiresRandomOrder)
			{
				Random::Generator rg = _instance.access_context().access_random_generator().spawn();
				Array<MeshNode*> contextMeshNodesRandomOrderSource;
				contextMeshNodesRandomOrderSource.make_space_for(contextMeshNodes.get_size());
				for_every_ref(mn, contextMeshNodes)
				{
					contextMeshNodesRandomOrderSource.push_back(mn);
				}
				while (!contextMeshNodesRandomOrderSource.is_empty())
				{
					int idx = rg.get_int(contextMeshNodesRandomOrderSource.get_size());
					contextMeshNodesRandomOrder.push_back(MeshNodePtr(contextMeshNodesRandomOrderSource[idx]));
					contextMeshNodesRandomOrderSource.remove_fast_at(idx);
				}
			}
		}
		int processIdx = 0;
		while (atLevel >= 0)
		{
			// check if we have one element dropped so far and go back at lower level
			bool goBack = false;
			for_count(int, i, atLevel + 1)
			{
				if (foundForEachMeshNodes[i].meshNode && foundForEachMeshNodes[i].meshNode->is_dropped())
				{
					goBack = true;
				}
			}

			{
				auto & atLevelMeshNode = foundForEachMeshNodes[atLevel];
				if (atLevelMeshNode.idxToCheck >= contextMeshNodes.get_size()) // size is the same for random order
				{
					if (fe->meshNodes[atLevel].get()->processAll ||
						! atLevelMeshNode.meshNode) // for "one" nothing found or already processed, go back
					{
						// we processed all at this level, go back
						goBack = true;
					}
					else
					{
						// start next one (or do element if we reached the end)
						++atLevel;
						if (foundForEachMeshNodes.is_index_valid(atLevel))
						{
							foundForEachMeshNodes[atLevel].meshNode = nullptr;
							foundForEachMeshNodes[atLevel].meshNodeIdx = NONE;
							foundForEachMeshNodes[atLevel].idxToCheck = startWithMeshNodeIdx;
						}
					}
				}

				if (goBack)
				{
					atLevelMeshNode.meshNode = nullptr;
					atLevelMeshNode.meshNodeIdx = NONE;
					--atLevel;
					if (atLevel >= 0)
					{
						foundForEachMeshNodes[atLevel].meshNode = nullptr;
						foundForEachMeshNodes[atLevel].meshNodeIdx = NONE;
						if (fe->meshNodes[atLevel].get()->processAll)
						{
							// continue scanning
							++foundForEachMeshNodes[atLevel].idxToCheck;
						}
						else
						{
							// exit
							foundForEachMeshNodes[atLevel].idxToCheck = contextMeshNodes.get_size(); // size is the same for random order
						}
					}
					continue;
				}
			}

			// check candidate
			if (atLevel < foundForEachMeshNodes.get_size())
			{
				auto & atLevelMeshNode = foundForEachMeshNodes[atLevel];
				auto* checkFEMeshNode = fe->meshNodes[atLevel].get();
				auto const& contextMeshNodesSource = checkFEMeshNode->randomOrder ? contextMeshNodesRandomOrder : contextMeshNodes;
				auto* currentMeshNode = contextMeshNodesSource[atLevelMeshNode.idxToCheck].get();
				bool unique = true;
				for_count(int, i, atLevel)
				{
					if (foundForEachMeshNodes[i].idxToCheck == atLevelMeshNode.idxToCheck)
					{
						unique = false;
						break;
					}
				}
				bool matches = unique;
				if (matches)
				{
					if (checkFEMeshNode->name.is_set() &&
						checkFEMeshNode->name.get(_context) != currentMeshNode->name)
					{
						matches = false;
					}
					if (!checkFEMeshNode->tagged.is_empty() &&
						!checkFEMeshNode->tagged.check(currentMeshNode->tags))
					{
						matches = false;
					}
				}
				if (!currentMeshNode->is_dropped() && unique && matches)
				{
					bool ok = false;
					if (checkFEMeshNode->toolSet.is_empty())
					{
						ok = true;
					}
					else
					{
						MeshNodeContext mnContext(_context, _instance, currentMeshNode);
						WheresMyPoint::Context wmpContext(&mnContext);
						wmpContext.set_random_generator(_instance.access_context().access_random_generator());

						WheresMyPoint::Value value;
						checkFEMeshNode->toolSet.update(value, wmpContext);
						if (value.get_type() != type_id<bool>())
						{
							error_generating_mesh(_instance, TXT("expecting bool, got %S"), RegisteredType::get_name_of(value.get_type()));
							result = false;
						}
						else
						{
							ok = value.get_as<bool>();
						}
					}
					if (ok)
					{
						// store candidate
						atLevelMeshNode.meshNode = currentMeshNode;
						atLevelMeshNode.meshNodeIdx = atLevelMeshNode.idxToCheck;
						// if we're looking for the best, stay here, if we process all, go deeper
						if (checkFEMeshNode->processAll)
						{
							++atLevel;
							if (foundForEachMeshNodes.is_index_valid(atLevel))
							{
								foundForEachMeshNodes[atLevel].meshNode = nullptr;
								foundForEachMeshNodes[atLevel].meshNodeIdx = NONE;
								foundForEachMeshNodes[atLevel].idxToCheck = startWithMeshNodeIdx;
							}
						}
					}
					if (! checkFEMeshNode->processAll)
					{
						// continue scanning
						++foundForEachMeshNodes[atLevel].idxToCheck;
					}
				}
				else
				{
					++atLevelMeshNode.idxToCheck; // skip dropped
					continue;
				}
			}

			// we have found enough!
			if (atLevel >= foundForEachMeshNodes.get_size())
			{
				bool allOk = true;
				for_every(fme, foundForEachMeshNodes)
				{
					if (!fme->meshNode)
					{
						allOk = false;
						break;
					}
				}
				if (allOk)
				{
#ifdef AN_DEVELOPMENT
					String processForFound;
					for_every(ffemn, foundForEachMeshNodes)
					{
						processForFound += String::printf(TXT("%i, "), ffemn->meshNodeIdx);
					}
#endif
					if (element.get())
					{
						if (!_context.process(element.get(), &_instance, processIdx))
						{
							error(TXT("error processing element"));
							result = false;
						}
						++processIdx;
					}
					for_every_ref(femn, fe->meshNodes)
					{
						for_every_ref(drop, femn->drop)
						{
							if (drop->toolSet.is_empty() &&
								!drop->name.is_set() &&
								drop->tagged.is_empty())
							{
								// drop (this)
								foundForEachMeshNodes[for_everys_index(femn)].meshNode->drop();
							}
							else
							{
								auto const& contextMeshNodesSource = femn->randomOrder ? contextMeshNodesRandomOrder : contextMeshNodes;
								for_range(int, mnIdx, startWithMeshNodeIdx, contextMeshNodesSource.get_size() - 1)
								{
									if (!contextMeshNodesSource.is_index_valid(mnIdx))
									{
										error(TXT("trying to reach not existing mesh node"));
										continue;
									}
									auto* meshNode = contextMeshNodesSource[mnIdx].get();
									if (!meshNode || meshNode->is_dropped())
									{
										continue;
									}
									bool matches = true;
									if (matches)
									{
										if (drop->name.is_set() &&
											drop->name.get(_context) != meshNode->name)
										{
											matches = false;
										}
										if (!drop->tagged.is_empty() &&
											!drop->tagged.check(meshNode->tags))
										{
											matches = false;
										}
									}
									if (!matches)
									{
										continue;
									}
									bool dropThisNode = false;
									if (drop->toolSet.is_empty())
									{
										dropThisNode = true;
									}
									else
									{
										MeshNodeContext mnContext(_context, _instance, meshNode);
										WheresMyPoint::Context wmpContext(&mnContext);
										wmpContext.set_random_generator(_instance.access_context().access_random_generator());

										WheresMyPoint::Value value;
										drop->toolSet.update(value, wmpContext);

										if (value.get_type() != type_id<bool>())
										{
											error_generating_mesh(_instance, TXT("expecting bool, got %S"), RegisteredType::get_name_of(value.get_type()));
											result = false;
										}
										else
										{
											dropThisNode = value.get_as<bool>();
										}
									}

									if (dropThisNode)
									{
										meshNode->drop();
									}
								}
							}
						}
					}
				}
				--atLevel;
				foundForEachMeshNodes[atLevel].meshNode = nullptr;
				foundForEachMeshNodes[atLevel].meshNodeIdx = NONE;
				if (fe->meshNodes[atLevel].get()->processAll)
				{
					// continue scanning
					++foundForEachMeshNodes[atLevel].idxToCheck;
				}
				else
				{
					// exit
					foundForEachMeshNodes[atLevel].idxToCheck = contextMeshNodes.get_size(); // size is the same for random order
				}
			}
		}
	}
	return result;
}

//

bool ElementForMeshNodes::ForEach::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc)
{
	bool result = true;

	for_every(node, _node->children_named(TXT("meshNode")))
	{
		ForEachMeshNodePtr femn(new ForEachMeshNode());
		if (femn->load_from_xml(node))
		{
			meshNodes.push_back(femn);
		}
		else
		{
			result = false;
		}
	}

	return result;
}

//

bool ElementForMeshNodes::ForEachMeshNode::load_from_xml(IO::XML::Node const * _node, bool _asToDrop)
{
	bool result = true;

	name.load_from_xml(_node, TXT("name"));
	tagged.load_from_xml_attribute(_node, TXT("tagged"));
	toolSet.load_from_xml(_node->first_child_named(TXT("wheresMyPoint")));
	
	if (!_asToDrop)
	{
		if (auto * attr = _node->get_attribute(TXT("process")))
		{
			if (attr->get_as_string() == TXT("all"))
			{
				processAll = true;
			}
			else if (attr->get_as_string() == TXT("one") ||
				attr->get_as_string() == TXT("best"))
			{
				processAll = false;
			}
			else
			{
				error_loading_xml(_node, TXT("\"process\" value is not \"all\" nor \"one/best\"!"));
			}
		}
		else
		{
			error_loading_xml(_node, TXT("\"process\" attribute missing!"));
		}
	}

	if (auto* attr = _node->get_attribute(TXT("order")))
	{
		if (attr->get_as_string() == TXT("random"))
		{
			randomOrder = true;
		}
		else
		{
			error_loading_xml(_node, TXT("\"order\" value is not \"random\"!"));
		}
	}

	if (toolSet.is_empty() && !name.is_set() && tagged.is_empty())
	{
		if (!_asToDrop)
		{
			error_loading_xml(_node, TXT("add anything, name, tagged, wheresMyPoint"));
			result = false;
		}
	}

	if (!_asToDrop)
	{
		for_every(node, _node->children_named(TXT("drop")))
		{
			ForEachMeshNodePtr femn(new ForEachMeshNode());
			if (femn->load_from_xml(node, true))
			{
				drop.push_back(femn);
			}
			else
			{
				result = false;
			}
		}
	}

	return result;
}

