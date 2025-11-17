#pragma once

#include "..\types\name.h"
#include "..\types\string.h"

namespace IO
{
	namespace XML
	{
		class Attribute;
		class Node;
		
		struct AllAttributes
		{
			struct Iterator
			{
				Iterator(AllAttributes const* _owner, Attribute const * _startAtAttribute);

				Iterator & operator ++ ();
				// post
				Iterator operator ++ (int);
				// comparison
				bool operator == (Iterator const & _other) const { return owner == _other.owner && attribute == _other.attribute; }
				bool operator != (Iterator const & _other) const { return owner != _other.owner || attribute != _other.attribute; }

				Attribute const & operator * ()  const { assert_slow(attribute); return *attribute; }
				Attribute const & operator -> () const { assert_slow(attribute); return *attribute; }
				
			private:
				AllAttributes const* owner;
				Attribute const * attribute; // nullptr if end
			};
		
			AllAttributes(Node const * _node);

			Iterator const begin() const { return attributeBegin; }
			Iterator const end() const { return attributeEnd; }

			Node const* get_node() const { return node; }

		private:
			Node const * node;
			Iterator attributeBegin;
			Iterator attributeEnd;
			
			friend struct Iterator;
		};

		struct AllChildren
		{
			struct Iterator
			{
				Iterator(AllChildren const* _owner, Node const * _startAtChildNode);

				Iterator & operator ++ ();
				// post
				Iterator operator ++ (int);
				// comparison
				bool operator == (Iterator const & _other) const { return owner == _other.owner && childNode == _other.childNode; }
				bool operator != (Iterator const & _other) const { return owner != _other.owner || childNode != _other.childNode; }

				Node const & operator * ()  const { assert_slow(childNode); return *childNode; }
				Node const & operator -> () const { assert_slow(childNode); return *childNode; }
				
			private:
				AllChildren const* owner;
				Node const * childNode; // nullptr if end
			};
		
			AllChildren(Node const * _node);

			Iterator const begin() const { return childBegin; }
			Iterator const end() const { return childEnd; }

			Node const* get_node() const { return node; }

		private:
			Node const * node;
			Iterator childBegin;
			Iterator childEnd;
			
			friend struct Iterator;
		};

		struct NamedChildren
		{
			struct Iterator
			{
				Iterator(NamedChildren const* _owner, Node const * _startAtChildNode);

				Iterator & operator ++ ();
				// post
				Iterator operator ++ (int);
				// comparison
				bool operator == (Iterator const & _other) const { return owner == _other.owner && childNode == _other.childNode; }
				bool operator != (Iterator const & _other) const { return owner != _other.owner || childNode != _other.childNode; }

				Node const & operator * ()  const { assert_slow(childNode); return *childNode; }
				Node const & operator -> () const { assert_slow(childNode); return *childNode; }
				
			private:
				NamedChildren const* owner;
				Node const * childNode; // nullptr if end
			};
		
			NamedChildren(Node const * _node, String const & _childName);

			Iterator const begin() const { return childBegin; }
			Iterator const end() const { return childEnd; }

			Node const* get_node() const { return node; }

		private:
			Node const * node;
			String childName;
			Iterator childBegin;
			Iterator childEnd;
			
			friend struct Iterator;
		};
		
	};

};
