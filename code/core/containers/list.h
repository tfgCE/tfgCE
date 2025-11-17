#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\memory\pooledObject.h"
#include "..\debug\detectMemoryLeaks.h"

template <typename Element>
class List
{
	friend class ListElement;

	public:
		class Iterator;
		class ConstIterator;

	private:
		class ListElement
		: public PooledObject<ListElement>
		{
			friend class List<Element>;
			friend class Iterator;
			friend class ConstIterator;
		private:
			List<Element> *list;
			ListElement *next;
			ListElement *prev;
			Element element;
		private:
			inline ListElement(List<Element> *_list, Element const & _element);
			inline ~ListElement();
		};

	public:
		class Iterator
		{
			friend class List<Element>;
			friend class ConstIterator;

			public:
				inline Iterator(Iterator const &);
				inline explicit Iterator(ConstIterator const &);

				// pre
				inline Iterator & operator ++ ();
				inline Iterator & operator -- ();
				// post
				inline Iterator operator ++ (int);
				inline Iterator operator -- (int);
				// comparison
				inline bool operator == (Iterator const &) const;
				inline bool operator != (Iterator const &) const;

				inline Element & operator * ()  const { assert_slow(listElement); return listElement->element; }
				inline Element & operator -> () const { assert_slow(listElement); return listElement->element; }
				inline operator bool () const { return listElement != nullptr; }

			private:
				List<Element> const *list;
				ListElement *listElement;

				inline Iterator(List<Element> const * const _list, ListElement * const _ListElement);
		};

		class ConstIterator
		{
			friend class List<Element>;

			public:
				inline ConstIterator(ConstIterator const &);
				inline ConstIterator(Iterator const &);

				// pre
				inline ConstIterator & operator ++ ();
				inline ConstIterator & operator -- ();
				// post
				inline ConstIterator operator ++ (int);
				inline ConstIterator operator -- (int);
				inline bool operator == (ConstIterator const &) const;
				inline bool operator != (ConstIterator const &) const;

				inline Element const & operator * ()  const { assert_slow(listElement); return listElement->element; }
				inline Element const & operator -> () const { assert_slow(listElement); return listElement->element; }
				inline operator bool () const { return listElement != nullptr; }

				inline Iterator to_non_const() const;

			private:
				List<Element> const *list;
				ListElement const* listElement;

				inline ConstIterator(List<Element> const * const _list, ListElement * const _ListElement);
		};

	private:
		unsigned int numElements;
		ListElement *first;
		ListElement *last;

		static inline Element* next_of(Iterator const& _iterator) { todo_note(TXT("implement!")); return nullptr; }
		static inline Element* next_of(ConstIterator const& _iterator) { todo_note(TXT("implement!")); return nullptr; }

	public:
		inline List();
		inline List(List const &);
		inline ~List();
		inline void operator = (List const & source) { if (&source != this) copy_from(source); }

		// slow and should be used rarely
		inline Element const & operator [] (uint _index) const;
		inline Element & operator [] (uint _index);

		inline Iterator const iterator_for(uint _index) const;

	public:
		inline void copy_from(List const & source);
		inline void add_from(List const & source);

	public:
		inline int get_size() const { return numElements; }
		inline bool is_empty() const { return numElements == 0; }
		inline bool has_place_left() const { return true; }

	public:
		inline void clear();

		inline void remove(Element const & _element);
		inline void remove(Iterator * io_iterator);
		inline void remove(Iterator const & _iterator);

		inline void push_back() { push_back(Element()); }

		inline void push_back(Element const & _element);
		inline bool push_back_unique(Element const& _element); // returns true if added
		inline void push_front(Element const & _element);

		inline void pop_back() { remove(last); }
		inline void pop_front() { remove(first); }

		inline void insert(Iterator const & _before, Element const & _element);
		inline void insert_after(Iterator const & _after, Element const & _element);

		inline Element const & get_first(void) const { assert_slow(first); return first->element; }
		inline Element const & get_last(void) const { assert_slow(last); return last->element; }
		inline Element & get_first(void) { assert_slow(first); return first->element; }
		inline Element & get_last(void) { assert_slow(last); return last->element; }

	public:
		inline bool does_contain(Element const & _element) const;

		inline Iterator const find(Element const & _element);
		inline Iterator const begin();
		inline Iterator const end();

		inline ConstIterator const find(Element const & _element) const;
		inline ConstIterator const begin() const;
		inline ConstIterator const end() const;
		inline ConstIterator const begin_const() const { return begin(); }
		inline ConstIterator const end_const() const { return end(); }

		inline int iterators_index(Iterator const & _i) const;
		inline int iterators_index(ConstIterator const & _i) const;

	private:
		inline void remove(ListElement * const _listElement);
};

#include "list.inl"
