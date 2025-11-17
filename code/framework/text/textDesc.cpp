#include "textDesc.h"

#include "textBlockInstance.h"

#include "..\library\library.h"

using namespace Framework;

//

bool TextDescElement::load_from_xml_into(REF_ Array<TextDescElementPtr> & _ptrs, IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _nodeNameAndID, tchar const * _groupNode)
{
	bool result = true;
	for_every(node, _node->children_named(_nodeNameAndID))
	{
		TextDescElementPtr ptr(new Framework::TextDescElement());
		if (ptr->load_from_xml(node, _lc, String::printf(TXT("%S [%i]"), _nodeNameAndID, _ptrs.get_size()).to_char()))
		{
			_ptrs.push_back(ptr);
		}
		else
		{
			error_loading_xml(_node, )
			result = false;
		}
	}
	if (_groupNode)
	{
		for_every(node, _node->children_named(_groupNode))
		{
			result &= load_from_xml_into(_ptrs, _node, _lc, _nodeNameAndID, nullptr);
		}
	}
	return result;
}

bool TextDescElement::load_from_xml(IO::XML::Node const * _node, LibraryLoadingContext & _lc, tchar const * _id)
{
	bool result = true;

	chance = _node->get_float_attribute_or_from_child(TXT("chance"), chance);
	priority = _node->get_float_attribute_or_from_child(TXT("priority"), priority);
	descId = _node->get_name_attribute_or_from_child(TXT("descId"), descId);
	descNo = _node->get_int_attribute_or_from_child(TXT("descNo"), descNo);
	descInternalOffset = _node->get_int_attribute_or_from_child(TXT("descInternalOffset"), descInternalOffset);
	descInternalOffset = _node->get_int_attribute_or_from_child(TXT("internalOffset"), descInternalOffset);
	contendId = _node->get_name_attribute_or_from_child(TXT("contendId"), contendId);
	result &= descTags.load_from_xml_attribute_or_child_node(_node, TXT("descTags"));
	result &= blockDescTags.load_from_xml_attribute_or_child_node(_node, TXT("blockDescTags"));

	paragraphId = _node->get_name_attribute_or_from_child(TXT("paragraphId"), paragraphId);

	result &= text.load_from_xml(_node, _lc, _id);

	return result;
}

bool TextDescElement::prepare_for_game(Library* _library, LibraryPrepareContext& _pfgContext)
{
	bool result = true;

	IF_PREPARE_LEVEL(_pfgContext, Framework::LibraryPrepareLevel::Resolve)
	{
		result &= text.cache(_library);
	}

	return result;
}

//

void TextDescBuilder::add(TextDescElement * _element, ProvideStringFormatterParams _provideFunc)
{
	TextDescElementRef eRef;
	eRef.element = _element;
	eRef.provideParamsFunc = _provideFunc;
	elements.push_back(eRef);
}

void TextDescBuilder::build(REF_ TextBlockInstances & _textBlockInstances, tchar const * _format, StringFormatterParams & _params, Random::Generator _usingGenerator) const
{
	Name currentDescId;

	struct ElementRef
	{
		TextDescElementRef const * ref = nullptr;
		Name descId;
		int descNo;
		int descInternalOffset;
		float priority;

		static int compare(void const * _a, void const * _b)
		{
			ElementRef const * a = plain_cast<ElementRef>(_a);
			ElementRef const * b = plain_cast<ElementRef>(_b);
			if (a->descNo > b->descNo)
			{
				return 1;
			}
			if (a->descNo < b->descNo)
			{
				return -1;
			}
			if (a->descInternalOffset > b->descInternalOffset)
			{
				return 1;
			}
			if (a->descInternalOffset < b->descInternalOffset)
			{
				return -1;
			}
			return 0;
		}
	};
	ARRAY_STACK(ElementRef, elementRefs, this->elements.get_size());
	ARRAY_STACK(ElementRef, mainElementRefs, this->elements.get_size());

	// gather all and fill desc nos
	for_every(element, this->elements)
	{
		if (element->element->text.get_all().is_empty())
		{
			continue;
		}
		ElementRef eRef;
		eRef.ref = element;
		int descNo = eRef.ref->element->descNo;
		if (eRef.ref->element->descId.is_valid())
		{
			if (auto const * exDescNo = this->descIdNo.get_existing(eRef.ref->element->descId))
			{
				descNo = *exDescNo;
			}
		}
		eRef.descId = eRef.ref->element->descId;
		eRef.descNo = descNo;
		eRef.descInternalOffset = eRef.ref->element->descInternalOffset;
		eRef.priority = eRef.ref->element->priority;

		elementRefs.push_back(eRef);
		if (eRef.ref->element->descInternalOffset == 0 &&
			_usingGenerator.get_chance(eRef.ref->element->chance))
		{
			mainElementRefs.push_back(eRef);
		}
	}

	// remove doubled main elements using priority
	// remove if they share id or there is no id for one to remove and they share desc number
	for (int i = 0; i < mainElementRefs.get_size(); ++ i) // size may change
	{
		bool removeIt = false;
		ElementRef const& iRef = mainElementRefs[i];
		for_count(int, j, mainElementRefs.get_size())
		{
			if (i != j)
			{
				ElementRef const& jRef = mainElementRefs[j];
				if (iRef.descNo == jRef.descNo &&
					(iRef.descId == jRef.descId || !iRef.descId.is_valid()) &&
					(iRef.priority < jRef.priority || (iRef.priority == jRef.priority && _usingGenerator.get_chance(0.5f))))
				{
					removeIt = true;
					break;
				}
			}
		}
		if (removeIt)
		{
			// remove it from both
			for_every(element, elementRefs)
			{
				if (element->ref == iRef.ref)
				{
					elementRefs.remove_fast_at(for_everys_index(element));
					break;
				}
			}
			mainElementRefs.remove_fast_at(i); // use fast, we will be sorting any way
			--i;
		}
	}

	// sort main elements now
	sort(mainElementRefs);

	// do blocking and removing after sorting
	for (int i = mainElementRefs.get_size() - 1; i >= 0; -- i) // reverse to allow first blockers block following
	{
		ElementRef const& iRef = mainElementRefs[i];
		if (!iRef.ref->element->descTags.is_empty())
		{
			bool removeIt = false;
			for_count(int, j, mainElementRefs.get_size())
			{
				if (i != j)
				{
					ElementRef const& jRef = mainElementRefs[j];
					if (! jRef.ref->element->blockDescTags.is_empty() &&
						jRef.ref->element->blockDescTags.does_match_any_from(iRef.ref->element->descTags))
					{
						removeIt = true;
						break;
					}
				}
			}
			if (removeIt)
			{
				mainElementRefs.remove_fast_at(i); // use fast, we will be sorting any way again
			}
		}
	}

	// sort main ref again and all elements
	sort(mainElementRefs);
	sort(elementRefs);

	// go through both arrays looking for refs to place
	Name paragraphId;
	bool paragraphAdded = false;
	for_every(mainRef, mainElementRefs)
	{
		for_every(eRef, elementRefs)
		{
			if (eRef->descId == mainRef->descId &&
				eRef->descNo == mainRef->descNo &&
				eRef->ref->element->contendId == mainRef->ref->element->contendId)
			{
				if (paragraphAdded && paragraphId != eRef->ref->element->paragraphId)
				{
					_textBlockInstances.end_paragraph();
				}
				paragraphId = eRef->ref->element->paragraphId;
				paragraphAdded = true;

				int idx = _usingGenerator.get_int(eRef->ref->element->text.get_all().get_size());
				TextBlock const * tb = eRef->ref->element->text.get_all()[idx].get();
				ForStringFormatterParams fsfp(_params);
				if (eRef->ref->provideParamsFunc)
				{
					// allow provided for this specific element to override_ global one
					fsfp.add_params_provider(eRef->ref->provideParamsFunc);
				}
				_textBlockInstances.add(tb->create_instance(_format, _params, _usingGenerator));
			}
		}
	}
}
