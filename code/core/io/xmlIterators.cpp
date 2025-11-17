#include "xmlIterators.h"
#include "xmlNode.h"

using namespace IO::XML;

AllAttributes::AllAttributes(Node const * _node)
: node(_node)
, attributeBegin(this, _node->first_attribute())
, attributeEnd(this, nullptr)
{
}

AllAttributes::Iterator::Iterator(AllAttributes const * _owner, Attribute const * _startAtAttribute)
: owner(_owner)
, attribute(_startAtAttribute)
{
}

AllAttributes::Iterator & AllAttributes::Iterator::operator ++ ()
{
	attribute = attribute->next_attribute();
	return *this;
}

AllAttributes::Iterator AllAttributes::Iterator::operator ++ (int)
{
	an_assert(false, TXT("why are you using this?"));
	AllAttributes::Iterator copy = *this;
	operator ++();
	return copy;
}

//

AllChildren::AllChildren(Node const * _node)
: node(_node)
, childBegin(this, nullptr)
, childEnd(this, nullptr)
{
	Node const * node = _node->first_sub_node();
	while (node)
	{
		if (node->get_type() == IO::XML::NodeType::Node)
		{
			childBegin = Iterator(this, node);
			break;
		}
		node = node->next_sub_node();
	}
}

AllChildren::Iterator::Iterator(AllChildren const* _owner, Node const * _startAtChildNode)
: owner(_owner)
, childNode(_startAtChildNode)
{
}

AllChildren::Iterator & AllChildren::Iterator::operator ++ ()
{
	childNode = childNode->next_sub_node();
	while (childNode && childNode->get_type() != IO::XML::NodeType::Node)
	{
		childNode = childNode->next_sub_node();
	}
	return *this;
}

AllChildren::Iterator AllChildren::Iterator::operator ++ (int)
{
	an_assert(false, TXT("why are you using this?"));
	AllChildren::Iterator copy = *this;
	operator ++();
	return copy;
}

//

NamedChildren::NamedChildren(Node const * _node, String const & _childName)
: node(_node)
, childName(_childName)
, childBegin(this, _node->first_child_named(_childName))
, childEnd(this, nullptr)
{
}

NamedChildren::Iterator::Iterator(NamedChildren const* _owner, Node const * _startAtChildNode)
: owner(_owner)
, childNode(_startAtChildNode)
{
}

NamedChildren::Iterator & NamedChildren::Iterator::operator ++ ()
{
	childNode = childNode->next_sibling_named(owner->childName);
	return *this;
}

NamedChildren::Iterator NamedChildren::Iterator::operator ++ (int)
{
	an_assert(false, TXT("why are you using this?"));
	NamedChildren::Iterator copy = *this;
	operator ++();
	return copy;
}
