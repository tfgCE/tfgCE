#include "..\memory\objectHelpers.h"
#include "..\debug\debug.h"

template <typename Element, int Size>
ArrayStatic<Element, Size>::ArrayStatic()
:	numElements( 0 )
{
}

template <typename Element, int Size>
ArrayStatic<Element, Size>::ArrayStatic(ArrayStatic const & _source)
:	numElements( 0 )
{
	copy_from(_source);
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::copy_from(ArrayStatic const & _source)
{
	clear();
	if ((numElements = _source.numElements))
	{
		ObjectHelpers<Element>::copy(begin(), _source.begin(), _source.end());
	}
}

template <typename Element, int Size>
ArrayStatic<Element, Size>::~ArrayStatic()
{
	clear();
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::clear()
{
	// destructors
	numElements = 0;
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::set_size(uint _size)
{
	numElements = _size;
	an_assert_log_always(numElements <= Size, TXT("ArrayStatic::set_size [%S]"), extra_debug_info());
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::grow_size(uint _size)
{
	numElements += _size;
	an_assert_log_always(numElements <= Size, TXT("ArrayStatic::grow_size [%S] [+%i -> %i > %i"), extra_debug_info(), _size, numElements, Size);
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::pop_front()
{
	an_assert_log_always(numElements > 0, TXT("ArrayStatic::pop_front [%S]"), extra_debug_info());
	if (numElements >= 2)
	{
		Element * prev = &elements[0];
		Element * next = &elements[1];
		for (uint i = 1; i < numElements; ++i, ++prev, ++next)
		{
			*prev = *next;
		}
	}
	--numElements;
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::pop_back()
{
	an_assert_log_always(numElements > 0, TXT("ArrayStatic::pop_back [%S]"), extra_debug_info());
	--numElements;
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::push_front(Element const & _element)
{
	an_assert_log_always(numElements < Size, TXT("ArrayStatic::push_front [%S]"), extra_debug_info());
	++numElements;
	if (numElements >= 2)
	{
		Element * prev = &elements[numElements - 2];
		Element * next = &elements[numElements - 1];
		for (uint i = numElements - 1; i >= 1; --i, --prev, --next)
		{
			*next = *prev;
		}
	}
	elements[0] = _element;
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::push_back(Element const & _element)
{
	an_assert_log_always(numElements < Size, TXT("ArrayStatic::push_back [%S]"), extra_debug_info());
	++ numElements;
	elements[numElements - 1] = _element;
}

template <typename Element, int Size>
bool ArrayStatic<Element, Size>::push_back_unique(Element const & _element)
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

template <typename Element, int Size>
void ArrayStatic<Element, Size>::push_back_or_replace(Element const& _element, Random::Generator& _generator)
{
	if (numElements < Size)
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

template <typename Element, int Size>
void ArrayStatic<Element, Size>::insert_at(uint const _at, Element const & _element)
{
	if (_at < numElements)
	{
		// >> abcdef
		//    012345
		const uint oldNumElements = numElements;
		bool haveOneMore = numElements < Size;
		if (haveOneMore)
		{
			++numElements;
		}
		// add new element in existing
		Element* copyFrom = &elements[oldNumElements - (haveOneMore ? 1 : 2)];
		Element* endCopyingAt = &elements[_at];
		Element* copyTo = &elements[numElements - 1];
		if (haveOneMore)
		{
			// create last one and copy there
			ObjectHelpers<Element>::copy(copyTo, copyFrom, copyFrom + 1);
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
		if (numElements >= Size)
		{
			return;
		}
		++numElements;
		// place new one
		ObjectHelpers<Element>::copy(&(elements[_at]), &_element);
	}
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::remove(Element const & _element)
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

template <typename Element, int Size>
void ArrayStatic<Element, Size>::remove_at(uint const _index, uint _howMany)
{
	if (_index + _howMany <= numElements)
	{
		numElements -= _howMany;

		// setup variables
		uint i = _index;
		Element *me = &elements[_index];
		Element *oe = &elements[_index + _howMany];

		// copy old elements (has to be done one after another... as memory may overlap)
		for (; i < numElements; ++ i, ++ me, ++ oe)
		{
			ObjectHelpers<Element>::copy(me, oe);
		}
	}
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::remove_fast(Element const & _element)
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

template <typename Element, int Size>
void ArrayStatic<Element, Size>::remove_fast_at(uint const _index)
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
	}
}

template <typename Element, int Size>
bool ArrayStatic<Element, Size>::does_contain(Element const & _element) const
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

template <typename Element, int Size>
bool ArrayStatic<Element, Size>::operator == (ArrayStatic const& _other) const
{
	if (get_size() == _other.get_size())
	{
		Element const* a = begin();
		Element const* o = _other.begin();
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

template <typename Element, int Size>
void ArrayStatic<Element, Size>::add_from(ArrayStatic<Element, Size> const& _source)
{
	an_assert_log_always(ArraySize >= numElements + _source.get_usize(), TXT("ArrayStatic::add_from(ArrayStatic) [%S]"), extra_debug_info());
	uint prevNumElements = numElements;
	set_size(numElements + _source.get_size());
	ObjectHelpers<Element>::copy(&elements[prevNumElements], _source.begin(), _source.end());
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::add_from(Array<Element> const& _source)
{
	an_assert_log_always(ArraySize >= numElements + _source.get_usize(), TXT("ArrayStatic::add_from(Array) [%S]"), extra_debug_info());
	uint prevNumElements = numElements;
	set_size(numElements + _source.get_size());
	ObjectHelpers<Element>::copy(&elements[prevNumElements], _source.begin(), _source.end());
}

template <typename Element, int Size>
void ArrayStatic<Element, Size>::check_space_for(uint const _numElements)
{
	an_assert_log_always(_numElements <= Size, TXT("ArrayStatic::check_space_for [%S]"), extra_debug_info());
}

template <typename Element, int Size>
int ArrayStatic<Element, Size>::find_index(Element const& _element) const
{
	// setup variables
	uint i;
	Element const* me = elements;

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

