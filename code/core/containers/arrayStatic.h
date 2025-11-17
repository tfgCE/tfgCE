#pragma once

#include "..\globalDefinitions.h"
#include "..\memory\memory.h"
#include "..\debug\debug.h"
#include "..\debug\detectMemoryLeaks.h"
#include "..\random\random.h"

#define ARRAY_STATIC(Element, Name, count) \
	ArrayStatic< Element, count > Name;

template <typename Element>
class Array;

template <typename Element, int Size>
class ArrayStatic
{
#ifdef WITH_EXTRA_DEBUG_INFO
	private:
		tchar const* debugInfo = nullptr;
	public:
		void set_extra_debug_info(tchar const* _debugInfo) { debugInfo = _debugInfo; }
		tchar const* extra_debug_info() { return debugInfo ? debugInfo : TXT("[not provided]"); }
#else
		tchar const* extra_debug_info() { return TXT("[not supported]"); }
#endif

private:
		enum
		{
			ArraySize = Size
		};
	public:
#ifndef AN_CLANG
		typedef Element Element;
#endif
		typedef Element* Iterator;
		typedef Element const* ConstIterator;

		static inline Element* next_of(Iterator const& _iterator) { return _iterator + 1; }
		static inline Element* next_of(ConstIterator const& _iterator) { return _iterator + 1; }

	public:
		inline ArrayStatic();
		inline ArrayStatic(ArrayStatic const &);
		inline ~ArrayStatic();
		inline void operator = (ArrayStatic const & source) { if (&source != this) copy_from(source); }
		inline void add_from(ArrayStatic<Element, Size> const& source);
		inline void add_from(Array<Element> const& source);

		inline void set_size_to_have(uint _index) { assert_slow(_index < ArraySize); numElements = max(numElements, _index + 1); }

		// direct access
		inline Element const & operator [] (uint _index) const { assert_slow(is_index_valid(_index), TXT("%i, count %i"), _index, get_size()); return elements[_index]; }
		inline Element & operator [] (uint _index) { assert_slow(is_index_valid(_index), TXT("%i, count %i"), _index, get_size()); return elements[_index]; }

		inline int get_space_left() const { return ArraySize - get_size(); }
		inline int get_max_size() const { return ArraySize; }

		inline int get_single_data_size() const { return sizeof(Element); }
		inline int get_data_size() const { return numElements * sizeof(Element); }

		inline int get_size() const { return numElements; }
		inline uint get_usize() const { return (uint)numElements; }
		inline void set_size(uint _size);
		inline void grow_size(uint _size);
		inline bool is_empty() const { return numElements == 0; }
		inline bool is_index_valid(int _idx) const { return _idx >= 0 && _idx < (int)numElements; }
		inline bool has_place_left() const { return get_size() < ArraySize; }

		inline int find_index(Element const& _element) const;

		inline void pop_front();
		inline void pop_back();
		inline void push_front(Element const & _element);
		inline void push_back() { push_back(Element()); }
		inline void push_front() { push_front(Element()); }
		inline void push_back(Element const& _element);
		inline bool push_back_unique(Element const & _element); // returns true if added
		inline void push_back_or_replace(Element const& _element, Random::Generator& _generator = Random::Generator().temp_ref());
		inline void insert_at(uint const _at, Element const & _element); // if no place, will push last one out of bounds

		inline void clear();
		inline void remove(Element const & _element);
		inline void remove_at(uint const _index, uint _howMany = 1);
		inline void remove_fast(Element const & _element);
		inline void remove_fast_at(uint const _index);

		inline Element* get_data() { return elements; }
		inline Element const * get_data() const { return elements; }
		inline Element& get_first() { assert_slow(numElements); return elements[0]; }
		inline Element const & get_first() const { assert_slow(numElements); return elements[0]; }
		inline Element& get_last() { assert_slow(numElements); return elements[numElements - 1]; }
		inline Element const & get_last() const { assert_slow(numElements); return elements[numElements - 1]; }
		inline Iterator const begin() { return elements; }
		inline Iterator const end() { return
#ifdef AN_CLANG
			& unsafe_access(numElements);
#else
			elements ? &unsafe_access(numElements) : nullptr;
#endif
		}

		inline bool does_contain(Element const & _element) const;

		// finding, iterators
		inline ConstIterator const begin() const { return elements; }
		inline ConstIterator const end() const { return
#ifdef AN_CLANG
			& unsafe_access(numElements);
#else
			elements ? &unsafe_access(numElements) : nullptr;
#endif
		}
		inline ConstIterator begin_const() const { return begin(); }
		inline ConstIterator end_const() const { return end(); }

		inline int iterators_index(Iterator const & _i) const { return (int)(_i - begin()); }
		inline int iterators_index(ConstIterator const & _i) const { return (int)(_i - begin()); }

		inline bool operator == (ArrayStatic const& _other) const;
		inline bool operator != (ArrayStatic const& _other) const { return !(operator==(_other)); }

	private:
		inline void copy_from(ArrayStatic const & source);

	private:
		inline void check_space_for(uint const _numElements);	// just makes sure there is enough memory

	private:
		Element elements[ArraySize];
		uint numElements;

		// if there's need to access reserved space or specific pointer
		inline Element const & unsafe_access(uint _index) const { return elements[_index]; }
		inline Element & unsafe_access(uint _index) { return elements[_index]; }
};

template <typename Element, int Size>
void sort(ArrayStatic<Element, Size>& _array) { qsort(_array.get_data(), _array.get_size(), sizeof(Element), Element::compare); }

template <typename Element, int Size>
void sort(ArrayStatic<Element, Size>& _array, int(*compare)(void const* _a, void const* _b)) { qsort(_array.get_data(), _array.get_size(), sizeof(Element), compare); }

template <typename Element, int Size>
void sort(ArrayStatic<Element*, Size>& _array) { qsort(_array.get_data(), _array.get_size(), sizeof(Element*), Element::compare); }

template <typename Element, int Size>
void sort(ArrayStatic<Element*, Size>& _array, int(*compare)(void const* _a, void const* _b)) { qsort(_array.get_data(), _array.get_size(), sizeof(Element*), compare); }

#include "arrayStatic.inl"

