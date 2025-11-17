#pragma once

#include "..\..\framework\text\localisedString.h"

namespace TeaForGodEmperor
{
	class Library;

	class TutorialTreeElement
	{
		FAST_CAST_DECLARE(TutorialTreeElement);
		FAST_CAST_END();

	public:
		virtual ~TutorialTreeElement() {}

		int get_tutorial_tree_element_indent() const { return indent; }
		Framework::LocalisedString const& get_tutorial_tree_element_name() const { return tutorialTreeElementName; }
		Framework::LocalisedString const& get_suggested_tutorial_name() const { return suggestedTutorialName.is_valid()? suggestedTutorialName : tutorialTreeElementName; }

		static TutorialTreeElement* build(TeaForGodEmperor::Library* library);
		void store_flat_tree(OUT_ Array<TutorialTreeElement*> & _flatTree);

		Array<TutorialTreeElement*> const& get_children() const { return children; }

		bool load_as_tutorial_tree_element_from_xml_child_node(IO::XML::Node const* _node, Framework::LibraryLoadingContext& _lc);

	private:
		Framework::LocalisedString tutorialTreeElementName;
		Framework::LocalisedString suggestedTutorialName; // if not provided, general name is used
		Framework::LibraryName tutorialTreeElementID; // same as library name (read from parent node!)
		Framework::LibraryName tutorialTreeElementParentID;
		int tutorialTreeElementIndex = 0;

		CACHED_ int indent = 0;

		Array<TutorialTreeElement*> children;

		void add_if_child(TutorialTreeElement* tte);

		static int compare_ptrs(void const* _a, void const* _b);

		void build_children(TeaForGodEmperor::Library* library, int _indent);
	};
};
