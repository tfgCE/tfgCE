#include "messageSet.h"

#include "library.h"

using namespace TeaForGodEmperor;

//

REGISTER_FOR_FAST_CAST(MessageSet);
LIBRARY_STORED_DEFINE_TYPE(MessageSet, messageSet);

MessageSet::MessageSet(Framework::Library * _library, Framework::LibraryName const & _name)
: base(_library, _name)
{
}

bool MessageSet::load_from_xml(IO::XML::Node const * _node, Framework::LibraryLoadingContext & _lc)
{
	bool result = base::load_from_xml(_node, _lc);
	
	Random::Generator rg;

	for_every(node, _node->children_named(TXT("message")))
	{
		Message m;
		m.id = node->get_name_attribute(TXT("id"));
		error_loading_xml_on_assert(m.id.is_valid(), result, node, TXT("no \"id\" provided"));
		m.fixedOrder.load_from_xml(node, TXT("fixedOrder"));
		m.randomOrder = rg.get_int();
		m.title.load_from_xml_child(node, TXT("title"), _lc, m.id.is_valid() ? String::printf(TXT("%S (title)"), m.id.to_char()).to_char() : String::printf(TXT("title #%i"), messages.get_size()).to_char(), false);
		m.message.load_from_xml(node, _lc, m.id.is_valid() ? String::printf(TXT("%S (message)"), m.id.to_char()).to_char() : String::printf(TXT("message #%i"), messages.get_size()).to_char(), false);
		for_every(n, node->children_named(TXT("highlight")))
		{
			Message::HighlightWidget hw;
			hw.screen = n->get_name_attribute(TXT("screen"));
			hw.widget = n->get_name_attribute(TXT("widget"));
			error_loading_xml_on_assert(hw.widget.is_valid(), result, n, TXT("no \"widget\" provided"));
			m.highlightWidgets.push_back(hw);
		}
		messages.push_back(m);
	}

	return result;
}

MessageSet::Message const * MessageSet::find(Name const & _id) const
{
	for_every(message, messages)
	{
		if (message->id == _id)
		{
			return message;
		}
	}
	return nullptr;
}
