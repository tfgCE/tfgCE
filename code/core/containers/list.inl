// definitions
// ListElement

template <class Element>
List<Element>::ListElement::ListElement(List<Element> * const _list, Element const & _element)
:	list( _list )
,	next( nullptr )
,	prev( nullptr )
,	element( _element )
{
	NAME_POOLED_OBJECT_IF_NOT_NAMED(ListElement);
}

template <class Element>
List<Element>::ListElement::~ListElement()
{
}


// definitions
// Iterator

template <class Element>
List<Element>::Iterator::Iterator(List<Element> const * const _list, ListElement * const _ListElement)
:	list( _list )
,	listElement( _ListElement )
{
}

template <class Element>
List<Element>::Iterator::Iterator(Iterator const & _iterator)
:	list( _iterator.list )
,	listElement( _iterator.listElement )
{
}

template <class Element>
List<Element>::Iterator::Iterator(ConstIterator const& _iterator)
:	 list(_iterator.list)
,	 listElement(cast_to_nonconst(_iterator.listElement))
{
}

template <class Element>
typename List<Element>::Iterator & List<Element>::Iterator::operator ++()
{
	assert_slow(listElement);
	assert_slow(listElement->list == list);

	listElement = listElement->next;

	return *this;
}

template <class Element>
typename List<Element>::Iterator & List<Element>::Iterator::operator --()
{
	assert_slow(!listElement || listElement->list == list);

	listElement = listElement? listElement->prev : list->last;

	return *this;
}

template <class Element>
typename List<Element>::Iterator List<Element>::Iterator::operator ++(int)
{
	typename List<Element>::Iterator prev = *this;
	operator ++ ();
	return prev;
}

template <class Element>
typename List<Element>::Iterator List<Element>::Iterator::operator --(int)
{
	typename List<Element>::Iterator prev = *this;
	operator -- ();
	return prev;
}

template <class Element>
bool List<Element>::Iterator::operator ==(Iterator const & _iterator) const
{
	assert_slow(_iterator.listElement != listElement || _iterator.list == list);

	return _iterator.listElement == listElement;
}

template <class Element>
bool List<Element>::Iterator::operator !=(Iterator const & _iterator) const
{
	assert_slow(_iterator.listElement == listElement || _iterator.list == list);

	return _iterator.listElement != listElement;
}


// definitions
// ConstIterator

template <class Element>
List<Element>::ConstIterator::ConstIterator(List<Element> const * const _list, ListElement * const _ListElement)
:	list( _list )
,	listElement( _ListElement )
{
}

template <class Element>
List<Element>::ConstIterator::ConstIterator(const ConstIterator & _iterator)
:	list( _iterator.list )
,	listElement( _iterator.listElement )
{
}

template <class Element>
List<Element>::ConstIterator::ConstIterator(Iterator const & _iterator)
:	list( _iterator.list )
,	listElement( _iterator.listElement )
{
}

template <class Element>
typename List<Element>::ConstIterator & List<Element>::ConstIterator::operator ++()
{
	assert_slow(listElement);
	assert_slow(listElement->list == list);

	listElement = listElement->next;

	return *this;
}

template <class Element>
typename List<Element>::ConstIterator & List<Element>::ConstIterator::operator --()
{
	assert_slow(!listElement || listElement->list == list);

	listElement = listElement? listElement->prev : list->last;

	return *this;
}

template <class Element>
typename List<Element>::ConstIterator List<Element>::ConstIterator::operator ++(int)
{
	typename List<Element>::ConstIterator prev = *this;
	operator ++ ();
	return prev;
}

template <class Element>
typename List<Element>::ConstIterator List<Element>::ConstIterator::operator --(int)
{
	typename List<Element>::ConstIterator prev = *this;
	operator -- ();
	return prev;
}

template <class Element>
bool List<Element>::ConstIterator::operator ==(const ConstIterator & _iterator) const
{
	assert_slow(_iterator.listElement != listElement || _iterator.list == list);

	return _iterator.listElement == listElement;
}

template <class Element>
bool List<Element>::ConstIterator::operator !=(const ConstIterator & _iterator) const
{
	assert_slow(_iterator.listElement == listElement || _iterator.list == list);

	return _iterator.listElement != listElement;
}

template <class Element>
typename List<Element>::Iterator List<Element>::ConstIterator::to_non_const() const
{
	return Iterator(*this);
}


// definitions
// List

template <class Element>
List<Element>::List()
:	numElements( 0 )
,	first( nullptr )
,	last( nullptr )
{
}

template <class Element>
List<Element>::List(List const & _source)
:	numElements( 0 )
,	first( nullptr )
,	last( nullptr )
{
	copy_from(_source);
}

template <class Element>
void List<Element>::copy_from(List const & _source)
{
	clear();
	add_from(_source);
}

template <class Element>
void List<Element>::add_from(List const & _source)
{
	if (_source.numElements)
	{
		auto iElement = _source.begin_const();
		auto iEnd = _source.end_const();
		while (iElement != iEnd)
		{
			// copy element
			push_back(*iElement);
			++iElement;
		}
	}
}

template <class Element>
List<Element>::~List()
{
	clear();
}

template <class Element>
void List<Element>::clear()
{
	while (first)
	{
		remove(first);
	}
}

template <class Element>
void List<Element>::remove(Element const & _element)
{
	Iterator foundElement = find(_element);
	if (foundElement != end())
	{
		remove(foundElement);
	}
}

template <class Element>
void List<Element>::remove(Iterator * io_iterator)
{
	assert_slow(io_iterator->list == this);

	remove(io_iterator->listElement);
#ifdef AN_DEBUG
	// clear iterator
	io_iterator->listElement = nullptr;
#endif
}

template <class Element>
void List<Element>::remove(Iterator const & _iterator)
{
	assert_slow(_iterator.list == this);

	remove(_iterator.listElement);
}

template <class Element>
void List<Element>::remove(ListElement * const _listElement)
{
	assert_slow(_listElement->list == this);

	// work with previous ListElement
	if (_listElement->prev)
	{
		_listElement->prev->next = _listElement->next;
	}
	else
	{
		first = _listElement->next;
	}

	// work with next ListElement
	if (_listElement->next)
	{
		_listElement->next->prev = _listElement->prev;
	}
	else
	{
		last = _listElement->prev;
	}

	// remove list ListElement
	delete _listElement;

	// decrease number of elements
	numElements --;
}

template <class Element>
void List<Element>::push_back(Element const & _element)
{
	// create list element
	ListElement * _listElement = new ListElement(this, _element);

	// add it at back
	_listElement->prev = last;
	if (last)
	{
		last->next = _listElement;
	}
	else
	{
		first = _listElement;
	}
	last = _listElement;

	// set list
	_listElement->list = this;

	// increase number of elements
	++ numElements;
}

template <typename Element>
bool List<Element>::push_back_unique(Element const& _element)
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


template <class Element>
void List<Element>::push_front(Element const & _element)
{
	// create list element
	ListElement * _listElement = new ListElement(this, _element);

	// add it at front
	_listElement->next = first;
	if (first)
	{
		first->prev = _listElement;
	}
	else
	{
		last = _listElement;
	}
	first = _listElement;

	// set list
	_listElement->list = this;

	// increase number of elements
	++ numElements;
}

template <class Element>
void List<Element>::insert(Iterator const & _before, Element const & _element)
{
	// create list element
	ListElement * _listElement = new ListElement(this, _element);

	// add it before "before"
	ListElement * before = _before.listElement;

	if (before)
	{
		_listElement->next = before;
		_listElement->prev = before->prev;
		before->prev = _listElement;
		if (_listElement->prev)
		{
			_listElement->prev->next = _listElement;
		}
		else
		{
			first = _listElement;
		}
	}
	else
	{
		_listElement->prev = last;
		if (last)
		{
			last->next = _listElement;
		}
		last = _listElement;
	}

	if (! first)
	{
		first = last;
	}

	// set list
	_listElement->list = this;

	// increase number of elements
	++ numElements;
}

template <class Element>
void List<Element>::insert_after(Iterator const & _after, Element const & _element)
{
	// create list element
	ListElement * _listElement = new ListElement(this, _element);

	// add it after "after"
	ListElement * after = _after.listElement;

	if (after)
	{
		_listElement->prev = after;
		_listElement->next = after->next;
		after->next= _listElement;
		if (_listElement->next)
		{
			_listElement->next->prev = _listElement;
		}
		else
		{
			last = _listElement;
		}
	}
	else
	{
		_listElement->next = first;
		if (first)
		{
			first->prev = _listElement;
		}
		first = _listElement;
	}

	if (!first && last)
	{
		first = last;
	}
	if (!last && first)
	{
		last = first;
	}

	// set list
	_listElement->list = this;

	// increase number of elements
	++numElements;
}

template <class Element>
typename List<Element>::Iterator const List<Element>::begin()
{
	return Iterator(this, first);
}

template <class Element>
typename List<Element>::Iterator const List<Element>::end()
{
	return Iterator(this, nullptr);
}

template <class Element>
bool List<Element>::does_contain(Element const & _element) const
{
	ListElement *element = first;
	while (element)
	{
		if (element->element == _element)
		{
			return true;
		}
		element = element->next;
	}
	return false;
}

template <class Element>
typename List<Element>::Iterator const List<Element>::find(Element const & _element)
{
	ListElement *element = first;
	while (element)
	{
		if (element->element == _element)
		{
			return Iterator(this, element);
		}
		element = element->next;
	}
	return Iterator(this, nullptr);
}

template <class Element>
typename List<Element>::ConstIterator const List<Element>::begin() const
{
	return ConstIterator(this, first);
}

template <class Element>
typename List<Element>::ConstIterator const List<Element>::end() const
{
	return ConstIterator(this, nullptr);
}

template <class Element>
typename List<Element>::ConstIterator const List<Element>::find(Element const & _element) const
{
	ListElement *element = first;
	while (element)
	{
		if (element->element == _element)
		{
			return Iterator(this, element);
		}
		element = element->next;
	}
	return Iterator(this, nullptr);
}

template <class Element>
Element const & List<Element>::operator [] (uint _index) const
{
	ListElement const *element = first;
	while (element && _index > 0)
	{
		element = element->next;
		-- _index;
	}
	assert_slow(element != nullptr);
	return element->element;
}

template <class Element>
Element & List<Element>::operator [] (uint _index)
{
	ListElement *element = first;
	while (element && _index > 0)
	{
		element = element->next;
		-- _index;
	}
	assert_slow(element != nullptr);
	return element->element;
}

template <class Element>
typename List<Element>::Iterator const List<Element>::iterator_for(uint _index) const
{
	ListElement* element = first;
	while (element && _index > 0)
	{
		--_index;
		element = element->next;
	}
	return Iterator(this, element);
}

template <class Element>
int List<Element>::iterators_index(Iterator const & _i) const
{
	ConstIterator a = begin();
	int idx = 0;
	while (a != end())
	{
		if (a == _i)
		{
			return idx;
		}
		++idx;
		++a;
	}
	return idx;
}

template <class Element>
int List<Element>::iterators_index(ConstIterator const & _i) const
{
	ConstIterator a = begin();
	int idx = 0;
	while (a != end())
	{
		if (a == _i)
		{
			return idx;
		}
		++idx;
		++a;
	}
	return idx;
}
