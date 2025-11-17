#pragma once

#include "..\containers\array.h"
#include "..\io\xml.h"
#include "..\other\registeredType.h"

template <typename From, typename To>
struct RemapValue
{
public:
	void clear() { entries.clear(); }
	bool load_from_xml(IO::XML::Node const* _node);

	bool is_valid() const { return !entries.is_empty(); }

	To remap(From const& _value) const;
	To get_last_to() const;

	void add(From const& _from, To const& _to);

private:
	struct Entry
	{
		From from = zero<From>();
		To to = zero<To>();

		inline static int compare(void const* _a, void const* _b)
		{
			Entry const& a = *(plain_cast<Entry>(_a));
			Entry const& b = *(plain_cast<Entry>(_b));
			if (a.from < b.from) return A_BEFORE_B;
			if (a.from > b.from) return B_BEFORE_A;
			return A_AS_B;
		}
	};

	Array<Entry> entries;
};

#include "remapValue.inl"
