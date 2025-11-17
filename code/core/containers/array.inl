#include "..\memory\objectHelpers.h"
#include "..\debug\detectMemoryLeaks.h"
#include "..\math\mathUtils.h"

template <typename Element>
Array<Element>::Array()
: reserveBufferBytes(1024)
, numElements(0)
, reservedElements(0)
{
}

template <typename Element>
Array<Element>::Array(uint _reserveBufferBytes)
:	reserveBufferBytes( _reserveBufferBytes )
,	numElements( 0 )
,	reservedElements( 0 )
{
}

template <typename Element>
Array<Element>::Array(Array const & _source)
:	reserveBufferBytes( _source.reserveBufferBytes )
,	numElements( 0 )
,	reservedElements( 0 )
{
	make_space_for(_source.numElements);
	if ((numElements = _source.numElements))
	{
		ObjectHelpers<Element>::call_copy_constructor_on(begin(), _source.begin(), _source.end());
	}
}

template <typename Element>
void Array<Element>::copy_from(Array const & _source)
{
	clear();
	set_size(_source.numElements);
	if ((numElements = _source.numElements))
	{
		ObjectHelpers<Element>::copy(begin(), _source.begin(), _source.end());
	}
}

template <typename Element>
void Array<Element>::add_from(Array const & _source)
{
	uint prevNumElements = numElements;
	set_size(numElements + _source.numElements);
	ObjectHelpers<Element>::copy(&elements[prevNumElements], _source.begin(), _source.end());
}

template <typename Element>
Array<Element>::~Array()
{
	clear_and_prune();
}

template <typename Element>
void Array<Element>::clear_and_prune()
{
	clear();

	if (reservedElements && elements)
	{
		// free memory
		free_memory(elements);
	}

	elements = nullptr;
	numElements = 0;
	reservedElements = 0;
}

template <typename Element>
void Array<Element>::clear()
{
	// destructors
	if (numElements)
	{
		ObjectHelpers<Element>::call_destructor_on(begin(), end());
	}
	numElements = 0;
}

template <typename Element>
void Array<Element>::set_size(uint _size, bool _exactSize)
{
	const uint oldNumElements = numElements;
	numElements = _size;
	if (Element* o_elements = check_space_for(numElements, _exactSize))
	{
		// copy old elements
		if (oldNumElements)
		{
			ObjectHelpers<Element>::call_copy_constructor_on(elements, o_elements, o_elements + oldNumElements);
		}

		// destroy old ones now

		// destructors
		ObjectHelpers<Element>::call_destructor_on(o_elements, o_elements + oldNumElements);

		// delete old array
		free_memory(o_elements);
	}

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
void Array<Element>::grow_size(uint _size, bool _exactSize)
{
	set_size(get_size() + _size, _exactSize);
}

template <typename Element>
void Array<Element>::prune()
{
	if (reservedElements == numElements)
	{
		return;
	}

	if (numElements > 0)
	{
		Element* o_elements = elements;
		elements = (Element*)allocate_memory(sizeof(Element)* numElements);

		// copy old elements
		if (numElements)
		{
			ObjectHelpers<Element>::call_copy_constructor_on(elements, o_elements, o_elements + numElements);
		}

		// destructors
		ObjectHelpers<Element>::call_destructor_on(o_elements, o_elements + numElements);

		// delete old array
		free_memory(o_elements);

		reservedElements = numElements;
	}
	else
	{
		clear_and_prune();
	}
}

template <typename Element>
void Array<Element>::make_space_for(uint const _numElements)
{
	if (Element* o_elements = check_space_for(_numElements, true))
	{
		// copy old elements
		if (numElements)
		{
			ObjectHelpers<Element>::call_copy_constructor_on(begin(), o_elements, o_elements + numElements);
		}

		// destructors
		ObjectHelpers<Element>::call_destructor_on(o_elements, o_elements + numElements);

		// delete old array
		free_memory(o_elements);
	}
}

template <typename Element>
void Array<Element>::make_space_for_additional(uint const _numElements)
{
	make_space_for(numElements + _numElements);
}

template <typename Element>
Element* Array<Element>::check_space_for(uint const _numElements, bool _exactSize)
{
	if (_numElements > reservedElements)
	{
		Element* o_elements = reservedElements? elements : (Element*)nullptr;

		if (_exactSize)
		{
			reservedElements = _numElements;
		}
		else
		{
			reservedElements = reservedElements + max<uint>((uint)1, reserveBufferBytes / sizeof(Element)); // increase not any more than reserve buffer bytes
			reservedElements = min( reservedElements, _numElements * 3 / 2); // try to increase by 50% but not more than above
			reservedElements = max(reservedElements, _numElements); // just make sure it's not less than required
		}

		elements = (Element*) allocate_memory(sizeof(Element) * reservedElements);

		return o_elements;
	}
	else
	{
		return (Element*)nullptr;
	}
}

template <typename Element>
void Array<Element>::push_back(Element const & _element)
{
	const uint oldNumElements = numElements;
	++ numElements;
	if (Element* o_elements = check_space_for(numElements))
	{
		an_assert_immediate(numElements == oldNumElements + 1, TXT("multithreading, huh?"));

		// copy old elements
		if (oldNumElements)
		{
			ObjectHelpers<Element>::call_copy_constructor_on(begin(), o_elements, o_elements + oldNumElements);
		}

		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[numElements - 1]), &_element);

		// destructors
		ObjectHelpers<Element>::call_destructor_on(o_elements, o_elements + oldNumElements);

		// delete old array
		free_memory(o_elements);

		an_assert_immediate(numElements == oldNumElements + 1, TXT("multithreading, huh?"));
	}
	else
	{
		// add new element
		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[numElements - 1]), &_element);
	}
}

template <typename Element>
bool Array<Element>::push_back_unique(Element const & _element)
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
void Array<Element>::pop_back()
{
	if (numElements > 0)
	{
		numElements--;
		ObjectHelpers<Element>::call_destructor_on(&unsafe_access(numElements), &unsafe_access(numElements + 1));
	}
}

template <typename Element>
void Array<Element>::pop_front()
{
	if (!is_empty())
	{
		remove_at(0);
	}
}

template <typename Element>
void Array<Element>::insert_at(uint const _at, Element const & _element)
{
	const uint oldNumElements = numElements;
	++ numElements;
	if (Element* o_elements = check_space_for(numElements))
	{
		// copy old elements with hole inside
		if (oldNumElements)
		{
			ObjectHelpers<Element>::call_copy_constructor_on(begin(), o_elements, o_elements + _at); // 0 to _at-1
			ObjectHelpers<Element>::call_copy_constructor_on(begin() + _at+1, o_elements + _at, o_elements + oldNumElements); // _at to end
		}

		// create new one in the middle
		ObjectHelpers<Element>::call_copy_constructor_on(&(elements[_at]), &_element);

		// destructors
		ObjectHelpers<Element>::call_destructor_on(o_elements, o_elements + oldNumElements);

		// delete old array
		free_memory(o_elements);
	}
	else
	{
		// add new element in existing
		Element* copyFrom = &elements[oldNumElements-1];
		Element* endCopyingAt = &elements[_at];
		Element* copyTo = &elements[numElements-1];
		if (oldNumElements > 0)
		{
			// >> abcdef
			//    012345
			// create last one and copy there
			ObjectHelpers<Element>::call_copy_constructor_on(copyTo, copyFrom, copyFrom + 1);
			--copyTo;
			--copyFrom;
			// >> abcde.f
			//    0123456
			if (oldNumElements > _at + 1)
			{
				// copy rest
				ObjectHelpers<Element>::copy_reverse(copyTo, copyFrom, endCopyingAt);
				// >> abCcdef
				//    0123456
			}
			// place new one (copy into existing)
			ObjectHelpers<Element>::copy(&(elements[_at]), &_element);
			// >> abgcdef
			//    0123456
		}
		else
		{
			// place new one
			ObjectHelpers<Element>::call_copy_constructor_on(&(elements[_at]), &_element);
		}
	}
}

template <typename Element>
void Array<Element>::remove(Element const & _element)
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
void Array<Element>::remove(Iterator const & _iterator)
{
	remove_at(_iterator - begin());
}

template <typename Element>
void Array<Element>::remove(ConstIterator const & _iterator)
{
	remove_at(_iterator - begin());
}

template <typename Element>
void Array<Element>::remove_at(uint const _index)
{
	if (_index < numElements)
	{
		numElements --;

		// setup variables
		uint i = _index;
		Element *me = &elements[_index];
		Element *oe = &elements[_index + 1];
		
		if (_index == numElements)
		{
			// call destructor on last one
			ObjectHelpers<Element>::call_destructor_on(me, me + 1);
		}
		else
		{
			// copy old elements (has to be done one after another... as memory may overlap)
			for (; i < numElements; ++i, ++me, ++oe)
			{
				ObjectHelpers<Element>::copy(me, oe);
			}
			// call destructor on last one
			ObjectHelpers<Element>::call_destructor_on(me, me + 1);
		}
	}
}

template <typename Element>
void Array<Element>::remove_at(uint const _index, uint const _count)
{
	if (_index < numElements)
	{
		uint count = min(_count, numElements - _index);
		numElements -= count;

		// setup variables
		uint i = _index;
		Element *me = &elements[_index];
		Element *oe = &elements[_index + count];
		
		if (_index == numElements)
		{
			// call destructor on last one
			ObjectHelpers<Element>::call_destructor_on(me, me + count);
		}
		else
		{
			// copy old elements (has to be done one after another... as memory may overlap)
			for (; i < numElements; ++i, ++me, ++oe)
			{
				ObjectHelpers<Element>::copy(me, oe);
			}
			// call destructor on last one
			ObjectHelpers<Element>::call_destructor_on(me, me + count);
		}
	}
}

template <typename Element>
void Array<Element>::remove_fast(Element const & _element)
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
void Array<Element>::remove_fast(Iterator const & _iterator)
{
	remove_fast_at((int)(_iterator - begin()));
}

template <typename Element>
void Array<Element>::remove_fast(ConstIterator const & _iterator)
{
	remove_fast_at((int)(_iterator - begin()));
}

template <typename Element>
void Array<Element>::remove_fast_at(uint const _index)
{
	if (_index < numElements)
	{
		numElements --;

		// replace the one we remove with the last one
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
bool Array<Element>::does_contain(Element const & _element) const
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

template <typename Element>
int Array<Element>::count(Element const & _element) const
{
	// setup variables
	uint i;
	Element const * me = elements;

	int counted = 0;
	// find element
	for (i = 0; i < numElements; ++ i, ++ me)
	{
		if (*me == _element)
		{
			++counted;
		}
	}
	return counted;
}

template <typename Element>
typename Array<Element>::Iterator Array<Element>::find(Element const & _element)
{
	// setup variables
	uint i;
	Element * me = elements;

	// find element
	for (i = 0; i < numElements; ++i, ++me)
	{
		if (*me == _element)
		{
			return me;
		}
	}
	return nullptr;
}

template <typename Element>
typename Array<Element>::ConstIterator Array<Element>::find(Element const & _element) const
{
	// setup variables
	uint i;
	Element const * me = elements;

	// find element
	for (i = 0; i < numElements; ++i, ++me)
	{
		if (*me == _element)
		{
			return me;
		}
	}
	return nullptr;
}

template <typename Element>
AN_NOT_CLANG_TYPENAME int Array<Element>::find_index(Element const & _element) const
{
	// setup variables
	uint i;
	Element const * me = elements;

	// find element
	for (i = 0; i < numElements; ++i, ++me)
	{
		if (*me == _element)
		{
			return i;
		}
	}
	return NONE;
}

template <typename Element>
bool Array<Element>::operator == (Array const & _other) const
{
	if (get_size() == _other.get_size())
	{
		Element const * a = begin();
		Element const * o = _other.begin();
		for (int idx = get_size(); idx > 0; --idx, ++a, ++o)
		{
			if (*a != *o)
			{
				return false;
			}
		}
		return true;
	}

	return false;
}
