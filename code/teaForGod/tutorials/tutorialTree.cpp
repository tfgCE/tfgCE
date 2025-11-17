#include "tutorialTree.h"

#include "..\library\library.h"

//

#ifdef AN_ALLOW_OPTIMIZE_OFF
#pragma optimize("", off)
#endif

//

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(TutorialTreeElement);

bool TutorialTreeElement::load_as_tutorial_tree_element_from_xml_child_node(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc)
{
	bool result = true;

	tutorialTreeElementID.load_from_xml(_node, TXT("name"), _lc);
	
	for_every(node, _node->children_named(TXT("asTutorialTreeElement")))
	{
		tutorialTreeElementName.load_from_xml(node, _lc, TXT("name"), false);
		tutorialTreeElementParentID.load_from_xml(node, TXT("childOf"), _lc);
		tutorialTreeElementIndex = node->get_int_attribute(TXT("index"), tutorialTreeElementIndex);
	}

	for_every(node, _node->children_named(TXT("asSuggestedTutorial")))
	{
		suggestedTutorialName.load_from_xml(node, _lc, TXT("suggestedTutorialName"), false);
	}

	return result;
}

void TutorialTreeElement::add_if_child(TutorialTreeElement* tte)
{
	if (tte)
	{
		if (tte->tutorialTreeElementParentID == tutorialTreeElementID)
		{
			children.push_back(tte);
		}
	}
}

int TutorialTreeElement::compare_ptrs(void const* _a, void const* _b)
{
	TutorialTreeElement* const& a = *plain_cast<TutorialTreeElement*>(_a);
	TutorialTreeElement* const& b = *plain_cast<TutorialTreeElement*>(_b);
	if (a->tutorialTreeElementIndex < b->tutorialTreeElementIndex) return A_BEFORE_B;
	if (b->tutorialTreeElementIndex < a->tutorialTreeElementIndex) return B_BEFORE_A;
	String aName = a->tutorialTreeElementID.to_string();
	String bName = b->tutorialTreeElementID.to_string();
	return String::compare_icase_sort(&aName, &bName);
}

TutorialTreeElement* TutorialTreeElement::build(TeaForGodEmperor::Library* library)
{
	TutorialTreeElement* tree = nullptr;
	library->get_tutorial_structures().do_for_every([&tree](Framework::LibraryStored* _libraryStored)
	{
		if (auto* tte = fast_cast<TutorialTreeElement>(_libraryStored))
		{
			if (!tte->tutorialTreeElementParentID.is_valid())
			{
				tree = tte;
			}
		}
	});
	if (tree)
	{
		tree->build_children(library, 0);
	}
	return tree;
}

void TutorialTreeElement::build_children(TeaForGodEmperor::Library* library, int _indent)
{
	if (_indent >= 99)
	{
		error_stop(TXT("check loops for tutorials"));
	}
	indent = _indent;
	children.clear();
	library->get_tutorials().do_for_every([this](Framework::LibraryStored* _libraryStored)
	{
		add_if_child(fast_cast<TutorialTreeElement>(_libraryStored));
	});
	library->get_tutorial_structures().do_for_every([this](Framework::LibraryStored* _libraryStored)
	{
		add_if_child(fast_cast<TutorialTreeElement>(_libraryStored));
	});
	sort(children, compare_ptrs);
	for_every_ptr(child, children)
	{
		child->build_children(library, _indent + 1);
	}
}

void TutorialTreeElement::store_flat_tree(OUT_ Array<TutorialTreeElement*>& _flatTree)
{
#ifdef AN_ALLOW_EXTENSIVE_LOGS
#ifdef AN_DEVELOPMENT_OR_PROFILER
	String line;
	for_count(int, i, indent)
	{
		line += TXT("  ");
	}
	if (! children.is_empty())
	{
		line += TXT("+=");
	}
	if (tutorialTreeElementName.is_valid())
	{
		line += tutorialTreeElementName.get();
	}
	else
	{
		line += TXT("[tutorials]");
	}
	output(TXT("[%04i]  %S"), tutorialTreeElementIndex, line.to_char());
#endif
#endif
	if (tutorialTreeElementName.is_valid())
	{
		_flatTree.push_back(this);
	}
	for_every_ptr(child, children)
	{
		child->store_flat_tree(OUT_ _flatTree);
	}
}
