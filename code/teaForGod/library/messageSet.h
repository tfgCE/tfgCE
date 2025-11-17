#pragma once

#include "..\..\framework\library\libraryStored.h"
#include "..\..\framework\text\localisedString.h"

namespace TeaForGodEmperor
{
	class MessageSet
	: public Framework::LibraryStored
	{
		FAST_CAST_DECLARE(MessageSet);
		FAST_CAST_BASE(Framework::LibraryStored);
		FAST_CAST_END();
		LIBRARY_STORED_DECLARE_TYPE();

		typedef Framework::LibraryStored base;
	public:
		struct Message
		{
			Name id;
			Optional<int> fixedOrder; // the lower, the sooner
			int randomOrder = 0; // set when loading
			Framework::LocalisedString title;
			Framework::LocalisedString message;
			struct HighlightWidget
			{
				Name screen;
				Name widget;
			};
			ArrayStatic<HighlightWidget, 16> highlightWidgets;

			Message()
			{
				SET_EXTRA_DEBUG_INFO(highlightWidgets, TXT("MessageSet::Message.highlightWidgets"));
			}

			// for Array<Message*>
			inline static int compare_ptr(void const* _a, void const* _b)
			{
				Message const* a = *plain_cast<Message*>(_a);
				Message const* b = *plain_cast<Message*>(_b);
				if (a->fixedOrder.is_set() &&
					b->fixedOrder.is_set())
				{
					if (a->fixedOrder.get() < b->fixedOrder.get()) return A_BEFORE_B;
					if (b->fixedOrder.get() < a->fixedOrder.get()) return B_BEFORE_A;
					return A_AS_B;
				}
				if (a->fixedOrder.is_set()) return A_BEFORE_B;
				if (b->fixedOrder.is_set()) return B_BEFORE_A;
				if (a->randomOrder < b->randomOrder) return A_BEFORE_B;
				if (b->randomOrder < a->randomOrder) return B_BEFORE_A;
				return A_AS_B;
			}

		};

	public:
		MessageSet(Framework::Library * _library, Framework::LibraryName const & _name);

	public: // LibraryStored
		override_ bool load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc);

		Message const * find(Name const & _id) const;
		Array<Message> const & get_all() const { return messages; }

	private:
		Array<Message> messages;
	};
};
