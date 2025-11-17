// definitions
// MapElement

template <typename Key, typename Element>
Map<Key, Element>::MapElement::MapElement(Map<Key, Element> * const _map, Key const & _key)
: map(_map)
, parent(nullptr)
, lower(nullptr)
, greater(nullptr)
, key(_key)
{
	NAME_POOLED_OBJECT_IF_NOT_NAMED(MapElement);
}

template <typename Key, typename Element>
Map<Key, Element>::MapElement::~MapElement()
{
	assert_slow(lower == nullptr);
	assert_slow(greater == nullptr);
}

template <typename Key, typename Element>
void Map<Key, Element>::MapElement::destroy()
{
	if (lower)
	{
		lower->destroy();
		lower = nullptr;
	}
	if (greater)
	{
		greater->destroy();
		greater = nullptr;
	}
	delete this;
}

// definitions
// Iterator

template <typename Key, typename Element>
Map<Key, Element>::Iterator::Iterator(Map<Key, Element> const * const _map, MapElement * const _mapElement)
: map(_map)
, mapElement(_mapElement)
{
	if (mapElement)
	{
		while (mapElement->lower)
		{
			mapElement = mapElement->lower;
		}
	}
}

template <typename Key, typename Element>
typename Map<Key, Element>::Iterator Map<Key, Element>::Iterator::at(Map<Key, Element> const * const _map, MapElement * const _mapElement)
{
	Iterator i = Iterator(_map, nullptr);
	i.mapElement = _mapElement;
	return i;
}

template <typename Key, typename Element>
Map<Key, Element>::Iterator::Iterator(Iterator const & _iterator)
: map(_iterator.map)
, mapElement(_iterator.mapElement)
{
	if (mapElement)
	{
		while (mapElement->lower)
		{
			mapElement = mapElement->lower;
		}
	}
}

template <typename Key, typename Element>
typename Map<Key, Element>::Iterator & Map<Key, Element>::Iterator::operator ++()
{
	assert_slow(mapElement);
	assert_slow(mapElement->map == map);

	if (mapElement->greater)
	{
		// go to lowest in greater, as we are on top of current
		mapElement = mapElement->greater;
		while (mapElement->lower)
		{
			mapElement = mapElement->lower;
		}
	}
	else if (mapElement->parent)
	{
		// go higher
		// if we're lower, stop at parent
		if (mapElement->parent->lower == mapElement)
		{
			mapElement = mapElement->parent;
		}
		else
		{
			// exit from greater
			while (mapElement->parent && mapElement->parent->greater == mapElement)
			{
				mapElement = mapElement->parent;
			}
			// we're either end or go from lower here:
			mapElement = mapElement->parent;
		}
	}
	else
	{
		// we're at top that has no greater
		mapElement = nullptr;
	}

	return *this;
}

template <typename Key, typename Element>
typename Map<Key, Element>::Iterator Map<Key, Element>::Iterator::operator ++(int)
{
	typename Map<Key, Element>::Iterator prev = *this;
	operator ++ ();
	return prev;
}

template <typename Key, typename Element>
bool Map<Key, Element>::Iterator::operator ==(Iterator const & _iterator) const
{
	assert_slow(_iterator.mapElement != mapElement || _iterator.map == map);

	return _iterator.mapElement == mapElement;
}

template <typename Key, typename Element>
bool Map<Key, Element>::Iterator::operator !=(Iterator const & _iterator) const
{
	assert_slow(_iterator.mapElement == mapElement || _iterator.map == map);

	return _iterator.mapElement != mapElement;
}

// definitions
// ConstIterator

template <typename Key, typename Element>
Map<Key, Element>::ConstIterator::ConstIterator(Map<Key, Element> const * const _map, MapElement const * const _mapElement)
: map(_map)
, mapElement(_mapElement)
{
	if (mapElement)
	{
		while (mapElement->lower)
		{
			mapElement = mapElement->lower;
		}
	}
}

template <typename Key, typename Element>
typename Map<Key, Element>::ConstIterator Map<Key, Element>::ConstIterator::at(Map<Key, Element> const * const _map, MapElement const * const _mapElement)
{
	ConstIterator i = ConstIterator(_map, nullptr);
	i.mapElement = _mapElement;
	return i;
}

template <typename Key, typename Element>
Map<Key, Element>::ConstIterator::ConstIterator(ConstIterator const & _ConstIterator)
: map(_ConstIterator.map)
, mapElement(_ConstIterator.mapElement)
{
	if (mapElement)
	{
		while (mapElement->lower)
		{
			mapElement = mapElement->lower;
		}
	}
}

template <typename Key, typename Element>
typename Map<Key, Element>::ConstIterator & Map<Key, Element>::ConstIterator::operator ++()
{
	assert_slow(mapElement);
	assert_slow(mapElement->map == map);

	if (mapElement->greater)
	{
		// go to lowest in greater, as we are on top of current
		mapElement = mapElement->greater;
		while (mapElement->lower)
		{
			mapElement = mapElement->lower;
		}
	}
	else if (mapElement->parent)
	{
		// go higher
		// if we're lower, stop at parent
		if (mapElement->parent->lower == mapElement)
		{
			mapElement = mapElement->parent;
		}
		else
		{
			// exit from greater
			while (mapElement->parent && mapElement->parent->greater == mapElement)
			{
				mapElement = mapElement->parent;
			}
			// we're either end or go from lower here:
			mapElement = mapElement->parent;
		}
	}
	else
	{
		// we're at top that has no greater
		mapElement = nullptr;
	}

	return *this;
}

template <typename Key, typename Element>
typename Map<Key, Element>::ConstIterator Map<Key, Element>::ConstIterator::operator ++(int)
{
	typename Map<Key, Element>::ConstIterator prev = *this;
	operator ++ ();
	return prev;
}

template <typename Key, typename Element>
bool Map<Key, Element>::ConstIterator::operator ==(ConstIterator const & _ConstIterator) const
{
	assert_slow(_ConstIterator.mapElement != mapElement || _ConstIterator.map == map);

	return _ConstIterator.mapElement == mapElement;
}

template <typename Key, typename Element>
bool Map<Key, Element>::ConstIterator::operator !=(ConstIterator const & _ConstIterator) const
{
	assert_slow(_ConstIterator.mapElement == mapElement || _ConstIterator.map == map);

	return _ConstIterator.mapElement != mapElement;
}

// Map

template <typename Key, typename Element>
Map<Key, Element>::Map()
: map(nullptr)
{
}

template <typename Key, typename Element>
Map<Key, Element>::Map(Map const & _source)
: map(nullptr)
{
	copy_from(_source);
}

template <typename Key, typename Element>
Map<Key, Element>::~Map()
{
	clear();
}

template <typename Key, typename Element>
void Map<Key, Element>::copy_from(Map const & _source)
{
	clear();
	add_from(_source);
}

template <typename Key, typename Element>
void Map<Key, Element>::add_from(Map const & _source)
{
	add(_source.map);
}

template <typename Key, typename Element>
void Map<Key, Element>::add(MapElement const * _mapElement)
{
	if (_mapElement)
	{
		operator[](_mapElement->key) = _mapElement->element;
		add(_mapElement->lower);
		add(_mapElement->greater);
	}
}

template <typename Key, typename Element>
void Map<Key, Element>::remove(Key const & _key)
{
	if (MapElement * cur = map)
	{
		while (cur)
		{
			if (_key == cur->key)
			{
				// store lower and greater
				MapElement* lower = cur->lower;
				cur->lower = nullptr;
				MapElement* greater = cur->greater;
				cur->greater = nullptr;
				// remove from parent
				if (cur->parent)
				{
					if (cur->parent->lower == cur)
					{
						cur->parent->lower = nullptr;
					}
					if (cur->parent->greater == cur)
					{
						cur->parent->greater = nullptr;
					}
				}
				else
				{
					map = nullptr;
				}
				// delete cur
				cur->destroy();
				// add lower and greater pairs
				add(lower);
				add(greater);
				// delete those pairs
				if (lower)
				{
					lower->destroy();
				}
				if (greater)
				{
					greater->destroy();
				}
				return;
			}
			MapElement *& next = _key < cur->key ? cur->lower : cur->greater;
			if (!next)
			{
				break;
			}
			cur = next;
		}
	}
}

template <typename Key, typename Element>
Element& Map<Key, Element>::operator[](Key const & _key)
{
	if (MapElement * cur = map)
	{
		while (cur)
		{
			if (_key == cur->key)
			{
				return cur->element;
			}
			MapElement *& next = _key < cur->key? cur->lower : cur->greater;
			if (! next)
			{
				MapElement * newNext = new MapElement(this, _key);
				newNext->parent = cur;
				next = newNext;
				return newNext->element;
			}
			cur = next;
		}
		assert_slow(false, TXT("should not get here"));
		return map->element;
	}
	else
	{
		map = new MapElement(this, _key);
		return map->element;
	}
}

template <typename Key, typename Element>
Key const * Map<Key, Element>::does_contain(Element const & _element) const
{
	ConstIterator i = begin();
	ConstIterator ie = end();
	while (i != ie)
	{
		if (i.get_element() == _element)
		{
			return &i.get_key();
		}
		++i;
	}
	return nullptr;
}

template <typename Key, typename Element>
Element* Map<Key, Element>::get_existing(Key const & _key)
{
	MapElement * cur = map;
	while (cur)
	{
		if (_key == cur->key)
		{
			return &cur->element;
		}
		cur = _key < cur->key? cur->lower : cur->greater;
	}
	return nullptr;
}

template <typename Key, typename Element>
Element const * Map<Key, Element>::get_existing(Key const & _key) const
{
	MapElement const * cur = map;
	while (cur)
	{
		if (_key == cur->key)
		{
			return &cur->element;
		}
		cur = _key < cur->key? cur->lower : cur->greater;
	}
	return nullptr;
}

template <typename Key, typename Element>
void Map<Key, Element>::clear()
{
	if (map)
	{
		map->destroy();
	}
	map = nullptr;
}

template <typename Key, typename Element>
typename Map<Key, Element>::Iterator const Map<Key, Element>::begin()
{
	return Iterator(this, map);
}

template <typename Key, typename Element>
typename Map<Key, Element>::Iterator const Map<Key, Element>::end()
{
	return Iterator(this, nullptr);
}

template <typename Key, typename Element>
typename Map<Key, Element>::ConstIterator const Map<Key, Element>::begin() const
{
	return ConstIterator(this, map);
}

template <typename Key, typename Element>
typename Map<Key, Element>::ConstIterator const Map<Key, Element>::end() const
{
	return ConstIterator(this, nullptr);
}

template <typename Key, typename Element>
bool Map<Key, Element>::operator == (Map const & _other) const
{
	ConstIterator t = begin();
	ConstIterator o = _other.begin();
	ConstIterator et = end();
	ConstIterator eo = _other.end();
	while (t != et && o != eo)
	{
		if (t.get_key() != o.get_key() ||
			t.get_element() != o.get_element())
		{
			return false;
		}
		++t;
		++o;
	}
	return t == et && o == eo; // both reached the end
}

