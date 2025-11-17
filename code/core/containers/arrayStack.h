#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\debug\detectMemoryLeaks.h"
#include "array.h"
#include "..\random\random.h"

#define ARRAY_STACK(Element, Name, count) \
	int32 size##Name = count; \
	allocate_stack_var(Element, stack##Name, size##Name); \
	ArrayStack< Element > Name((Element*)stack##Name, size##Name); \
	SET_EXTRA_DEBUG_INFO(Name, TXT(#Name));

template <typename Element>
class ArrayStack
{
#ifdef WITH_EXTRA_DEBUG_INFO
	private:
		tchar const* debugInfo = nullptr;
	public:
		void set_extra_debug_info(tchar const* _debugInfo) { debugInfo = _debugInfo; }
		tchar const * extra_debug_info() { return debugInfo? debugInfo : TXT("[not provided]"); }
#else
		tchar const* extra_debug_info() { return TXT("[not supported]"); }
#endif

	public:
#ifndef AN_CLANG
		typedef Element Element;
#endif
		typedef Element* Iterator;
		typedef Element const* ConstIterator;

		static inline Element* next_of(Iterator const& _iterator) { return _iterator + 1; }
		static inline Element* next_of(ConstIterator const& _iterator) { return _iterator + 1; }

	public:
		inline ArrayStack(Element* _memory, int _maxSize);
		inline ArrayStack(ArrayStack const& _array) { copy_from(_array); }
		inline ~ArrayStack();
		inline void operator = (Array<Element> const & source) { copy_from(source); }
		inline void operator = (ArrayStack const & source) { if (&source != this) copy_from(source); }
		inline void copy_from(Array<Element> const & source);
		inline void copy_from(ArrayStack const & source);
		inline void add_from(ArrayStack const & source);

		// direct access
		inline Element const & operator [] (uint _index) const { assert_slow(is_index_valid(_index), TXT("%i, count %i"), _index, get_size()); return elements[_index]; }
		inline Element & operator [] (uint _index) { assert_slow(is_index_valid(_index), TXT("%i, count %i"), _index, get_size()); return elements[_index]; }

		inline int get_max_size() const { return reservedElements; }

		inline int get_single_data_size() const { return sizeof(Element); }
		inline int get_data_size() const { return numElements * sizeof(Element); }

		inline int get_size() const { return numElements; }
		inline uint get_usize() const { return (uint)numElements; }
		inline void set_size(uint _size);
		inline void grow_size(uint _addSize) { set_size(get_size() + _addSize); }
		inline bool is_empty() const { return numElements == 0; }
		inline bool is_index_valid(int _idx) const { return _idx >= 0 && _idx < (int)numElements; }
		inline bool has_place_left() const { return numElements < reservedElements; }

		inline void push_back() { push_back(Element()); }
		inline void push_back(Element const & _element);
		inline bool push_back_unique(Element const & _element); // returns true if added
		inline void push_back_if_possible(Element const & _element);
		inline void push_back_or_replace(Element const & _element, Random::Generator & _generator = Random::Generator().temp_ref());
		inline void pop_back();
		inline void pop_front() { remove_at(0); }
		inline void insert_at(uint const _at, Element const & _element); // if no place, will push last one out of bounds

		inline void clear();
		inline void remove(Element const & _element);
		inline void remove(Iterator const & _iterator);
		inline void remove(ConstIterator const & _iterator);
		inline void remove_at(uint const _index);
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

		// finding, iterators
		inline ConstIterator const begin() const { return elements; }
		inline ConstIterator const end() const { return elements ? &unsafe_access(numElements) : nullptr; }
		inline ConstIterator const begin_const() const { return begin(); }
		inline ConstIterator const end_const() const { return end(); }

		inline int iterators_index(Iterator const & _i) const { return (int)(_i - begin()); }
		inline int iterators_index(ConstIterator const & _i) const { return (int)(_i - begin()); }

	private:
		inline void check_space_for(uint const _numElements);	// just makes sure there is enough memory

	private: 
		Element* elements;
		uint numElements;
		uint reservedElements;

		// if there's need to access reserved space or specific pointer
		inline Element const & unsafe_access(uint _index) const { return elements[_index]; }
		inline Element & unsafe_access(uint _index) { return elements[_index]; }
};

template <typename Element>
void sort(ArrayStack<Element> & _array) { qsort(_array.get_data(), _array.get_size(), sizeof(Element), Element::compare); }

template <typename Element>
void sort(ArrayStack<Element> & _array, int(*compare)(void const * _a, void const * _b)) { qsort(_array.get_data(), _array.get_size(), sizeof(Element), compare); }

template <typename Element>
void sort(ArrayStack<Element*> & _array) { qsort(_array.get_data(), _array.get_size(), sizeof(Element*), Element::compare); }

template <typename Element>
void sort(ArrayStack<Element*> & _array, int(*compare)(void const * _a, void const * _b)) { qsort(_array.get_data(), _array.get_size(), sizeof(Element*), compare); }

// specialisations

#include "sortUtils.h"

template <>
inline void sort(ArrayStack<int>& _array) { sort(_array, compare_int); }

#include "arrayStack.inl"

