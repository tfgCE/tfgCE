#pragma once

#include "..\debug\detectMemoryLeaks.h"

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\debug\debug.h"

#define ARRAY_PREALLOC_SIZE(Element, Name, count) \
	Array<Element> Name; \
	Name.make_space_for(count);

template <typename Element>
class ArrayStack;

template <typename Element>
class Array
{
	public:
#ifndef AN_CLANG
		typedef Element Element;
#endif
		typedef Element* Iterator;
		typedef Element const* ConstIterator;

		static inline Element* next_of(Iterator const& _iterator) { return _iterator + 1; }
		static inline Element* next_of(ConstIterator const& _iterator) { return _iterator + 1; }

	public:
		inline Array();
		explicit inline Array(uint _reserveBufferBytes);
		inline Array(Array const &);
		inline ~Array();
		inline void operator = (Array const & source) { if (&source != this) copy_from(source); }
		inline void copy_from(Array const & source);
		inline void add_from(Array const & source);

		inline void set_reserve_buffer_bytes(uint _reserveBufferBytes) { reserveBufferBytes = _reserveBufferBytes; }

		// direct access
		inline Element const & operator [] (uint _index) const { assert_slow(is_index_valid(_index), TXT("%i, count %i"), _index, get_size()); return elements[_index]; }
		inline Element & operator [] (uint _index) { assert_slow(is_index_valid(_index), TXT("%i, count %i"), _index, get_size()); return elements[_index]; }

		inline int get_single_data_size() const { return sizeof(Element); }
		inline int get_data_size() const { return numElements * sizeof(Element); }

		inline int get_space_left() const { return (reservedElements - numElements); }
		inline int get_reserved_size() const { return reservedElements; }

		inline int get_size() const { return numElements; }
		inline uint get_usize() const { return (uint)numElements; }
		inline void set_size(uint _size, bool _exactSize = false);
		inline void grow_size(uint _size, bool _exactSize = false);
		inline bool is_empty() const { return numElements == 0; }
		inline bool is_index_valid(int _idx) const { return _idx >= 0 && _idx < (int)numElements; }
		inline bool has_place_left() const { return true; }

		inline void push_back() { push_back(Element()); }
		inline void push_front() { push_front(Element()); }
		inline void push_back(Element const & _element);
		inline bool push_back_unique(Element const & _element); // returns true if added
		inline void pop_back();
		inline void insert_at(uint const _at, Element const & _element);
		inline void push_front(Element const & _element) { insert_at(0, _element); }
		inline void pop_front();

		inline void clear_and_prune();
		inline void clear();
		inline void prune();
		inline void make_space_for(uint const _numElements);
		inline void make_space_for_additional(uint const _byNumElements);
		inline void remove(Element const & _element);
		inline void remove(Iterator const & _iterator);
		inline void remove(ConstIterator const & _iterator);
		inline void remove_at(uint const _index);
		inline void remove_at(uint const _index, uint const _count);
		inline void remove_fast(Element const & _element);
		inline void remove_fast(Iterator const & _iterator);
		inline void remove_fast(ConstIterator const & _iterator);
		inline void remove_fast_at(uint const _index);

		inline Element* get_data() { return elements; }
		inline Element const * get_data() const { return elements; }
		inline Element& get_first() { assert_slow(numElements); return *elements; }
		inline Element const & get_first() const { assert_slow(numElements); return *elements; }
		inline Element& get_last() { assert_slow(numElements); return elements[numElements - 1]; }
		inline Element const & get_last() const { assert_slow(numElements); return elements[numElements - 1]; }
		inline Iterator const begin() { return elements; }
		inline Iterator const end() { return elements ? &unsafe_access(numElements) : nullptr; }

		inline bool does_contain(Element const & _element) const;
		inline int count(Element const & _element) const;

		// finding, iterators
		inline ConstIterator const begin() const { return elements; }
		inline ConstIterator const end() const { return elements ? &unsafe_access(numElements) : nullptr; }
		inline ConstIterator const begin_const() const { return begin(); }
		inline ConstIterator const end_const() const { return end(); }

		inline Iterator find(Element const & _element);
		inline ConstIterator find(Element const & _element) const;
		inline int find_index(Element const & _element) const;

		inline int iterators_index(Iterator const & _i) const { return (int)(_i - begin()); }
		inline int iterators_index(ConstIterator const & _i) const { return (int)(_i - begin()); }

		inline bool operator == (Array const & _other) const;
		inline bool operator != (Array const & _other) const { return !(operator==(_other)); }

	private:
		inline Element *check_space_for(uint const _numElements, bool _exactSize = false);	// allocates memory if needed, pointer to old buffer if new has been allocated (new one is invalid)

	private:
		uint reserveBufferBytes;
		Element* elements = nullptr;
		uint numElements;
		uint reservedElements;

		// if there's need to access reserved space or specific pointer
		inline Element const & unsafe_access(uint _index) const { return elements[_index]; }
		inline Element & unsafe_access(uint _index) { return elements[_index]; }

		friend class ArrayStack<Element>;
};

template <typename Element>
void sort(Array<Element> & _array) { qsort(_array.get_data(), _array.get_size(), sizeof(Element), Element::compare); }

template <typename Element>
void sort(Array<Element> & _array, int(*compare)(void const * _a, void const * _b)) { qsort(_array.get_data(), _array.get_size(), sizeof(Element), compare); }

template <typename Element>
void sort(Array<Element*> & _array) { qsort(_array.get_data(), _array.get_size(), sizeof(Element*), Element::compare); }

template <typename Element>
void sort(Array<Element*> & _array, int(*compare)(void const * _a, void const * _b)) { qsort(_array.get_data(), _array.get_size(), sizeof(Element*), compare); }

// specialisations

#include "sortUtils.h"

template <>
inline void sort(Array<int>& _array) { sort(_array, compare_int); }

#include "array.inl"

