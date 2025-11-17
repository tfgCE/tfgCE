#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\memory\pooledObject.h"
#include "..\debug\detectMemoryLeaks.h"

template <typename Key, typename Element>
class Map
{
	friend class MapElement;

	public:
		class Iterator;
		class ConstIterator;

	private:
		class MapElement
		: public PooledObject<MapElement>
		{
			friend class Map<Key, Element>;
			friend class Iterator;
			friend class ConstIterator;
		private:
			Map<Key, Element>* map;
			MapElement* parent;
			MapElement* lower;
			MapElement* greater;
			Key key;
			Element element;
		private:
			inline MapElement(Map<Key, Element>* _map, Key const & _key);
			inline ~MapElement(); // instead of deleting, call "clear"

			inline void destroy();
		};

	public:
		class Iterator
		{
			friend class Map<Key, Element>;
			friend class ConstIterator;

			public:
				inline Iterator(Iterator const &);

				// pre
				inline Iterator & operator ++ ();
				// post
				inline Iterator operator ++ (int);
				// comparison
				inline bool operator == (Iterator const &) const;
				inline bool operator != (Iterator const &) const;

				inline Element & operator * ()  const { assert_slow(mapElement); return mapElement->element; }
				inline Element & operator -> () const { assert_slow(mapElement); return mapElement->element; }
				inline operator bool () const { return mapElement != nullptr; }

				inline Element & get_element() const { assert_slow(mapElement); return mapElement->element; }
				inline Key & get_key() const { assert_slow(mapElement); return mapElement->key; }

			private:
				Map<Key, Element> const *map;
				MapElement *mapElement;

				inline Iterator(Map<Key, Element> const * const _map, MapElement * const _mapElement);
				static inline Iterator at(Map<Key, Element> const * const _map, MapElement * const _mapElement);
		};
		
		class ConstIterator
		{
			friend class Map<Key, Element>;

			public:
				inline ConstIterator(const ConstIterator &);
				inline ConstIterator(Iterator const &);

				// pre
				inline ConstIterator & operator ++ ();
				// post
				inline ConstIterator operator ++ (int);
				inline bool operator == (const ConstIterator &) const;
				inline bool operator != (const ConstIterator &) const;

				inline Element const & operator * ()  const { assert_slow(mapElement); return mapElement->element; }
				inline Element const & operator -> () const { assert_slow(mapElement); return mapElement->element; }
				inline operator bool () const { return mapElement != nullptr; }

				inline Element const & get_element() const { assert_slow(mapElement); return mapElement->element; }
				inline Key const & get_key() const { assert_slow(mapElement); return mapElement->key; }

			private:
				Map<Key, Element> const *map;
				MapElement const* mapElement;

				inline ConstIterator(Map<Key, Element> const * const _map, MapElement const * const _mapElement);
				static inline ConstIterator at(Map<Key, Element> const * const _map, MapElement const * const _mapElement);
		};
		
	private:
		MapElement* map;

	public:
		inline Map();
		inline Map(Map const &);
		inline ~Map();
		inline void operator = (Map const & source) { if (&source != this) copy_from(source); }
		inline bool operator == (Map const & _other) const;
		inline bool operator != (Map const & _other) const { return !(operator==(_other)); }

	public:
		inline void copy_from(Map const & _source);
		inline void add_from(Map const & _source);

	private:
		inline void add(MapElement const * _mapElement);

	public:
		inline bool is_empty() const { return map == nullptr; }

	public:
		inline Element& operator[](Key const & _key);
		inline void remove(Key const & _key);

		inline Key const * does_contain(Element const & _element) const;

		inline Element* get_existing(Key const & _key);
		inline Element const * get_existing(Key const & _key) const;

		inline bool has_key(Key const & _key) const { return get_existing(_key) != nullptr; }

	public:
		inline Iterator const begin();
		inline Iterator const end();

		inline ConstIterator const begin() const;
		inline ConstIterator const end() const;
		inline ConstIterator const begin_const() const { return begin(); }
		inline ConstIterator const end_const() const { return end(); }

	public:
		inline void clear();

	private:
};

#include "map.inl"
