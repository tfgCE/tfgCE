#include "..\memory\objectHelpers.h"

template <typename Element>
ArrayStack<Element>::ArrayStack(Element* _memory, int _maxSize)
: elements(_memory)
, numElements(0)
, reservedElements(_maxSize)
{
}

template <typename Element>
void ArrayStack<Element>::copy_from(Array<Element> const & _source)
{
	an_assert_log_always(reservedElements >= _source.get_usize(), TXT("ArrayStack::copy_from(Array) [%S]"), extra_debug_info()); // assert_slow
	set_size(min(_source.get_usize(), reservedElements));
	ObjectHelpers<Element>::copy(begin(), _source.begin(), &_source.unsafe_access(numElements));
}

template <typename Element>
void ArrayStack<Element>::copy_from(ArrayStack const & _source)
{
	an_assert_log_always(reservedElements >= _source.get_usize(), TXT("ArrayStack::copy_from(ArrayStack) [%S]"), extra_debug_info()); // assert_slow
	set_size(min(_source.get_usize(), reservedElements));
	ObjectHelpers<Element>::copy(begin(), _source.begin(), &_source.unsafe_access(numElements));
}

template <typename Element>
void ArrayStack<Element>::add_from(ArrayStack const& _source)
{
	an_assert_log_always(reservedElements >= numElements + _source.get_usize(), TXT("ArrayStack::add_from [%S]"), extra_debug_info()); // assert_slow
	uint prevNumElements = numElements;
	set_size(numElements + _source.numElements);
	ObjectHelpers<Element>::copy(&elements[prevNumElements], _source.begin(), _source.end());
}

template <typename Element>
ArrayStack<Element>::~ArrayStack()
{
	clear();
}

template <typename Element>
void ArrayStack<Element>::clear()
{
	// destructors
	if (numElements)
	{
		ObjectHelpers<Element>::call_destructor_on(begin(), end());
	}
	numElements = 0;
}

template <typename Element>
void ArrayStack<Element>::set_size(uint _size)
{
	const uint oldNumElements = numElements;
	numElements = _size;
	check_space_for(numElements);

	// if we need to 
	if (numElements > oldNumElements)
	{
		// construct new
		ObjectHelpers<Element>::call_constructor_on(elements + oldNumElements, elements + numElements);
	}
	else if (numElements < oldNumElements)
	{
		// destroy unmatching
		ObjectHelpers<Element>::call_destructor_on(elements + numElements, elements + oldNumElements);
	}	 
}

template <typename Element>
void ArrayStack<Element>::check_space_for(uint const _numElements)
{
	an_assert_log_always(_numElements <= reservedElements, TXT("ArrayStack::check_space_for [%S]"), extra_debug_info()); // assert_slow
}

template <typename Element>
void ArrayStack<Element>::push_back(Element const & _element)
{
	an_assert_log_always(numElements < reservedElements, TXT("ArrayStack::push back [%S]"), extra_debug_info());
	++ numElements;
	check_space_for(numElements);
	// add new element
	ObjectHelpers<Element>::call_copy_constructor_on(&(elements[numElements - 1]), &_element);
}

template <typename Element>
bool ArrayStack<Element>::push_back_unique(Element const & _element)
{
	if (!does_contain(_element))
	{
		push_back(_element);
		return true;
	}
	else
	{
		return false;
	}
}

template <typename Element>
void ArrayStack<Element>::push_back_if_possible(Element const & _element)
{
	if (numElements < reservedElements)
	{
		++numElements;
		check_space_for(numElements);
		// add new element
		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[numElements - 1]), &_element);
	}
}

template <typename Element>
void ArrayStack<Element>::push_back_or_replace(Element const & _element, Random::Generator & _generator)
{
	if (numElements < reservedElements)
	{
		++numElements;
		check_space_for(numElements);
		// add new element
		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[numElements - 1]), &_element);
	}
	else
	{
		int idx = _generator.get_int(numElements);
		ObjectHelpers<Element>::call_destructor_on(&elements[idx], &elements[idx + 1]);
		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[idx]), &_element);
	}
}

template <typename Element>
void ArrayStack<Element>::pop_back()
{
	if (numElements > 0)
	{
		--numElements;
		ObjectHelpers<Element>::call_destructor_on(&(elements[numElements]), &(elements[numElements + 1]));
	}
}

template <typename Element>
void ArrayStack<Element>::insert_at(uint const _at, Element const & _element)
{
	if (_at < numElements)
	{
		// >> abcdef
		//    012345
		const uint oldNumElements = numElements;
		bool haveOneMore = numElements < reservedElements;
		if (haveOneMore)
		{
			++numElements;
			check_space_for(numElements);
		}
		// add new element in existing
		Element* copyFrom = &elements[oldNumElements - (haveOneMore ? 1 : 2)];
		Element* endCopyingAt = &elements[_at];
		Element* copyTo = &elements[numElements - 1];
		if (haveOneMore)
		{
			// create last one and copy there
			ObjectHelpers<Element>::call_copy_constructor_on(copyTo, copyFrom, copyFrom + 1);
			// >> abcde.f
			//    0123456
			--copyTo;
			--copyFrom;
		}
		if (oldNumElements > _at + 1)
		{
			// copy rest
			ObjectHelpers<Element>::copy_reverse(copyTo, copyFrom, endCopyingAt);
			// >> abCcdef
			//    0123456
		}
		// copy into existing
		ObjectHelpers<Element>::copy(&(elements[_at]), &_element);
		// >> abgcdef
		//    0123456
	}
	else
	{
		if (numElements >= reservedElements)
		{
			return;
		}
		++numElements;
		// place new one
		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[_at]), &_element);
	}
}

template <typename Element>
void ArrayStack<Element>::remove(Element const & _element)
{
	// setup variables
	uint i;
	Element const * me = elements;

	// find element
	for (i = 0; i < numElements; ++ i, ++ me)
	{
		if (*me == _element)
		{
			remove_at(i);
			return;
		}
	}
}

template <typename Element>
void ArrayStack<Element>::remove(Iterator const & _iterator)
{
	remove_at(_iterator - begin());
}

template <typename Element>
void ArrayStack<Element>::remove(ConstIterator const & _iterator)
{
	remove_at(_iterator - begin());
}

template <typename Element>
void ArrayStack<Element>::remove_at(uint const _index)
{
	if (_index < numElements)
	{
		numElements --;

		// setup variables
		uint i = _index;
		Element *me = &elements[_index];
		Element *oe = &elements[_index + 1];

		// copy old elements (has to be done one after another... as memory may overlap)
		for (; i < numElements; ++ i, ++ me, ++ oe)
		{
			ObjectHelpers<Element>::copy(me, oe);
		}
		// call destructor on last one
		ObjectHelpers<Element>::call_destructor_on(oe, oe + 1);
	}
}

template <typename Element>
void ArrayStack<Element>::remove_fast(Element const & _element)
{
	// setup variables
	uint i;
	Element const * me = elements;

	// find element
	for (i = 0; i < numElements; ++ i, ++ me)
	{
		if (*me == _element)
		{
			remove_fast_at(i);
			return;
		}
	}
}

template <typename Element>
void ArrayStack<Element>::remove_fast(Iterator const & _iterator)
{
	remove_fast_at(_iterator - begin());
}

template <typename Element>
void ArrayStack<Element>::remove_fast(ConstIterator const & _iterator)
{
	remove_fast_at(_iterator - begin());
}

template <typename Element>
void ArrayStack<Element>::remove_fast_at(uint const _index)
{
	if (_index < numElements)
	{
		numElements --;

		// setup variables
		if (_index != numElements)
		{
			Element *me = &elements[_index];
			Element *oe = &unsafe_access(numElements);
			ObjectHelpers<Element>::copy(me, oe);
		}
		// call destructor on last one
		Element *le = &unsafe_access(numElements);
		ObjectHelpers<Element>::call_destructor_on(le, le + 1);
	}
}

template <typename Element>
bool ArrayStack<Element>::does_contain(Element const & _element) const
{
	// setup variables
	uint i;
	Element const * me = elements;

	// find element
	for (i = 0; i < numElements; ++ i, ++ me)
	{
		if (*me == _element)
		{
			return true;
		}
	}
	return false;
}


